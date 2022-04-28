#include "stdafx.h"
#include "SelectBoxWindow.h"
#include "SandMan.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../SbiePlusAPI.h"
#include "../Views/SbieView.h"

#if defined(Q_OS_WIN)
#include <wtypes.h>
#include <QAbstractNativeEventFilter>
#include <dbt.h>
#endif

QTreeWidgetItem* CSelectBoxWindow__GetBoxParent(const QMap<QString, QStringList>& Groups, QMap<QString, QTreeWidgetItem*>& GroupItems, QTreeWidget* treeBoxes, const QString& Name, int Depth = 0)
{
	if (Depth > 100)
		return NULL;
	for (auto I = Groups.constBegin(); I != Groups.constEnd(); ++I) {
		if (I->contains(Name)) {
			if (I.key().isEmpty())
				return NULL; // global group
			QTreeWidgetItem*& pParent = GroupItems[I.key()];
			if (!pParent) {
				pParent = new QTreeWidgetItem();
				pParent->setText(0, I.key());
				QFont fnt = pParent->font(0);
				fnt.setBold(true);
				pParent->setFont(0, fnt);
				if (QTreeWidgetItem* pParent2 = CSelectBoxWindow__GetBoxParent(Groups, GroupItems, treeBoxes, I.key(), ++Depth))
					pParent2->addChild(pParent);
				else
					treeBoxes->addTopLevelItem(pParent);
			}
			return pParent;
		}
	}
	return NULL;
}

double CSelectBoxWindow__GetBoxOrder(const QMap<QString, QStringList>& Groups, const QString& Name, double value = 0.0, int Depth = 0) 
{
	if (Depth > 100)
		return 1000000000;
	for (auto I = Groups.constBegin(); I != Groups.constEnd(); ++I) {
		int Pos = I->indexOf(Name);
		if (Pos != -1) {
			value = double(Pos) + value / 10.0;
			if (I.key().isEmpty())
				return value;
			return CSelectBoxWindow__GetBoxOrder(Groups, I.key(), value, ++Depth);
		}
	}
	return 1000000000;
}

CSelectBoxWindow::CSelectBoxWindow(const QStringList& Commands, const QString& BoxName, const QString& WrkDir, QWidget *parent)
	: QDialog(parent)
{
	m_Commands = Commands;
	m_WrkDir = WrkDir;

	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::CustomizeWindowHint;
	//flags &= ~Qt::WindowContextHelpButtonHint;
	//flags &= ~Qt::WindowSystemMenuHint;
	//flags &= ~Qt::WindowMinMaxButtonsHint;
	//flags |= Qt::WindowMinimizeButtonHint;
	//flags &= ~Qt::WindowCloseButtonHint;
	flags &= ~Qt::WindowContextHelpButtonHint;
	//flags &= ~Qt::WindowSystemMenuHint;
	setWindowFlags(flags);

	//setWindowState(Qt::WindowActive);
	SetForegroundWindow((HWND)QWidget::winId());

	bool bAlwaysOnTop = theConf->GetBool("Options/AlwaysOnTop", false);
	this->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);

	if (!bAlwaysOnTop) {
		HWND hWnd = (HWND)this->winId();
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		QTimer::singleShot(100, this, [hWnd]() {
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		});
	}

	ui.setupUi(this);
	this->setWindowTitle(tr("Sandboxie-Plus - Run Sandboxed"));


	connect(ui.radBoxed, SIGNAL(clicked(bool)), this, SLOT(OnBoxType()));
	connect(ui.radUnBoxed, SIGNAL(clicked(bool)), this, SLOT(OnBoxType()));

	connect(ui.buttonBox, SIGNAL(accepted()), SLOT(OnRun()));
	connect(ui.buttonBox, SIGNAL(rejected()), SLOT(reject()));

	connect(ui.treeBoxes, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(OnBoxDblClick(QTreeWidgetItem*)));

	QList<CSandBoxPtr> Boxes = theAPI->GetAllBoxes().values(); // map is sorted by key (box name)
	QMap<QString, QStringList> Groups = theGUI->GetBoxView()->GetGroups();

	if (theConf->GetBool("MainWindow/BoxTree_UseOrder", false)) {
		QMultiMap<double, CSandBoxPtr> Boxes2;
		foreach(const CSandBoxPtr &pBox, Boxes) {
			Boxes2.insertMulti(CSelectBoxWindow__GetBoxOrder(Groups, pBox->GetName()), pBox);
		}
		Boxes = Boxes2.values();
	}

	QMap<QString, QTreeWidgetItem*> GroupItems;
	foreach(const CSandBoxPtr &pBox, Boxes) 
	{
		if (!pBox->IsEnabled() || !pBox->GetBool("ShowForRunIn", true))
			continue;

		CSandBoxPlus* pBoxEx = qobject_cast<CSandBoxPlus*>(pBox.data());

		QTreeWidgetItem* pParent = CSelectBoxWindow__GetBoxParent(Groups, GroupItems, ui.treeBoxes, pBox->GetName());
		
		QTreeWidgetItem* pItem = new QTreeWidgetItem();
		pItem->setText(0, pBox->GetName().replace("_", " "));
		pItem->setData(0, Qt::UserRole, pBox->GetName());
		pItem->setData(0, Qt::DecorationRole, theGUI->GetBoxIcon(pBoxEx->GetType(), pBox->GetActiveProcessCount()));
		if (pParent)
			pParent->addChild(pItem);
		else
			ui.treeBoxes->addTopLevelItem(pItem);

		if (pBox->GetName().compare(BoxName, Qt::CaseInsensitive) == 0)
			ui.treeBoxes->setCurrentItem(pItem);
	}

	ui.treeBoxes->expandAll();

	//ui.treeBoxes->sortByColumn(0, Qt::AscendingOrder);

	//restoreGeometry(theConf->GetBlob("SelectBoxWindow/Window_Geometry"));
}

CSelectBoxWindow::~CSelectBoxWindow()
{
	//theConf->SetBlob("SelectBoxWindow/Window_Geometry", saveGeometry());
}

void CSelectBoxWindow::closeEvent(QCloseEvent *e)
{
	//emit Closed();
	this->deleteLater();
}

void CSelectBoxWindow::OnBoxType()
{
	ui.treeBoxes->setEnabled(!ui.radUnBoxed->isChecked());
}

void CSelectBoxWindow::OnBoxDblClick(QTreeWidgetItem*)
{
	OnRun();
}

void CSelectBoxWindow::OnRun()
{
	QTreeWidgetItem* pItem = ui.treeBoxes->currentItem();

	QString BoxName;
	if (ui.radUnBoxed->isChecked())
	{
		if (QMessageBox("Sandboxie-Plus", tr("Are you sure you want to run the program outside the sandbox?"), QMessageBox::Question, QMessageBox::Yes, QMessageBox::No | QMessageBox::Default | QMessageBox::Escape, QMessageBox::NoButton, this).exec() != QMessageBox::Yes)
			return;
		pItem = NULL;
	}
	else if (pItem == NULL) {
		QMessageBox("Sandboxie-Plus", tr("Please select a sandbox."), QMessageBox::Information, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this).exec();
		return;
	}
	else {
		BoxName = pItem->data(0, Qt::UserRole).toString();
	}


	//QList<SB_STATUS> Results;
	foreach(const QString & Command, m_Commands) {
		theAPI->RunStart(BoxName, Command, ui.chkAdmin->isChecked(), m_WrkDir);
	}
	//CSandMan::CheckResults(Results);

	setResult(1);
	close();
}