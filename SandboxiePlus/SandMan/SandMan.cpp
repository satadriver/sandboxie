#include "stdafx.h"
#include "SandMan.h"
#include "../MiscHelpers/Common/Common.h"
#include "../MiscHelpers/Common/ExitDialog.h"
#include "../MiscHelpers/Common/SortFilterProxyModel.h"
#include "Views/SbieView.h"
#include "../MiscHelpers/Common/CheckableMessageBox.h"
#include <QWinEventNotifier>
#include "./Dialogs/MultiErrorDialog.h"
#include "../QSbieAPI/SbieUtils.h"
#include "../QSbieAPI/Sandboxie/BoxBorder.h"
#include "../QSbieAPI/Sandboxie/SbieTemplates.h"
#include "Windows/SettingsWindow.h"
#include "Windows/RecoveryWindow.h"
#include <QtConcurrent>
#include "../MiscHelpers/Common/SettingsWidgets.h"
#include "Windows/OptionsWindow.h"
#include <QProxyStyle>
#include "../MiscHelpers/Common/TreeItemModel.h"
#include "../MiscHelpers/Common/ListItemModel.h"
#include "Views/TraceView.h"
#include "Windows/SelectBoxWindow.h"
#include "../UGlobalHotkey/uglobalhotkeys.h"

CSbiePlusAPI* theAPI = NULL;

#if defined(Q_OS_WIN)
#include <wtypes.h>
#include <QAbstractNativeEventFilter>
#include <dbt.h>

class CNativeEventFilter : public QAbstractNativeEventFilter
{
public:
	virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result)
	{
		if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
		{
			MSG *msg = static_cast<MSG *>(message);

			//if(msg->message != 275 && msg->message != 1025)
			//	qDebug() << msg->message;

			if (msg->message == WM_NOTIFY)
			{
				//return true;
			}
			else if (msg->message == WM_DEVICECHANGE)
			{
				if (msg->wParam == DBT_DEVICEARRIVAL // Drive letter added
				 || msg->wParam == DBT_DEVICEREMOVECOMPLETE) // Drive letter removed
				{
					/*DEV_BROADCAST_HDR* deviceBroadcast = (DEV_BROADCAST_HDR*)msg->lParam;
					if (deviceBroadcast->dbch_devicetype == DBT_DEVTYP_VOLUME) {
					}*/
					if (theAPI)
						theAPI->UpdateDriveLetters();
				}
				/*else if ((msg->wParam & 0xFF80) == 0xAA00 && msg->lParam == 'xobs') 
				{
					UCHAR driveNumber = (UCHAR)(msg->wParam & 0x1F);
					if (driveNumber < 26) {		
					}
				}
				else if (msg->wParam == DBT_DEVNODES_CHANGED) // hardware changed
				{
				}*/
			}
			else if (msg->message == WM_DWMCOLORIZATIONCOLORCHANGED)
			{
				if (theGUI && theConf->GetInt("Options/UseDarkTheme", 2) == 2)
					theGUI->UpdateTheme();
			}
		}
		return false;
	}
};

HWND MainWndHandle = NULL;
#endif

CSandMan* theGUI = NULL;

extern QString g_PendingMessage;

#include <QStyledItemDelegate>
class CTrayBoxesItemDelegate : public QStyledItemDelegate
{
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyleOptionViewItem opt(option);
		if ((opt.state & QStyle::State_MouseOver) != 0)
			opt.state |= QStyle::State_Selected;
		else if ((opt.state & QStyle::State_HasFocus) != 0 && m_Hold)
			opt.state |= QStyle::State_Selected;
		opt.state &= ~QStyle::State_HasFocus;
		QStyledItemDelegate::paint(painter, opt, index);
	}
public:
	static bool m_Hold;
};

bool CTrayBoxesItemDelegate::m_Hold = false;

CSandMan::CSandMan(QWidget *parent)
	: QMainWindow(parent)
{
#if defined(Q_OS_WIN)
	MainWndHandle = (HWND)QWidget::winId();

	QApplication::instance()->installNativeEventFilter(new CNativeEventFilter);
#endif

	theGUI = this;

	QDesktopServices::setUrlHandler("http", this, "OpenUrl");
	QDesktopServices::setUrlHandler("https", this, "OpenUrl");
	QDesktopServices::setUrlHandler("sbie", this, "OpenUrl");

	m_ThemeUpdatePending = false;
	m_DefaultStyle = QApplication::style()->objectName();
	m_DefaultPalett = QApplication::palette();

	m_LanguageId = 1033; // lang en_us
	LoadLanguage();
	SetUITheme();

	m_bExit = false;

	theAPI = new CSbiePlusAPI(this);
	connect(theAPI, SIGNAL(StatusChanged()), this, SLOT(OnStatusChanged()));
	connect(theAPI, SIGNAL(BoxClosed(const QString&)), this, SLOT(OnBoxClosed(const QString&)));

	m_RequestManager = NULL;

	QString appTitle = tr("Sandboxie-Plus v%1").arg(GetVersion());

	this->setWindowTitle(appTitle);

	setAcceptDrops(true);

	m_pBoxBorder = new CBoxBorder(theAPI, this);

	m_SbieTemplates = new CSbieTemplates(theAPI, this);


	m_bConnectPending = false;
	m_bStopPending = false;

	QTreeViewEx::m_ResetColumns = tr("Reset Columns");
	CPanelView::m_CopyCell = tr("Copy Cell");
	CPanelView::m_CopyRow = tr("Copy Row");
	CPanelView::m_CopyPanel = tr("Copy Panel");

	CreateMenus();

	m_pMainWidget = new QWidget();
	m_pMainLayout = new QVBoxLayout(m_pMainWidget);
	m_pMainLayout->setMargin(2);
	m_pMainLayout->setSpacing(0);
	this->setCentralWidget(m_pMainWidget);

	CreateToolBar();

	m_pLogSplitter = new QSplitter();
	m_pLogSplitter->setOrientation(Qt::Vertical);
	m_pMainLayout->addWidget(m_pLogSplitter);

	m_pPanelSplitter = new QSplitter();
	m_pPanelSplitter->setOrientation(Qt::Horizontal);
	m_pLogSplitter->addWidget(m_pPanelSplitter);


	m_pBoxView = new CSbieView();
	m_pPanelSplitter->addWidget(m_pBoxView);

	connect(m_pBoxView->GetTree()->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(OnSelectionChanged()));

	//m_pPanelSplitter->addWidget();

	m_pLogTabs = new QTabWidget();
	m_pLogSplitter->addWidget(m_pLogTabs);

	// Message Log
	m_pMessageLog = new CPanelWidgetEx();

	//m_pMessageLog->GetView()->setItemDelegate(theGUI->GetItemDelegate());
	((QTreeWidgetEx*)m_pMessageLog->GetView())->setHeaderLabels(tr("Time|Message").split("|"));

	m_pMessageLog->GetMenu()->insertAction(m_pMessageLog->GetMenu()->actions()[0], m_pCleanUpMsgLog);
	m_pMessageLog->GetMenu()->insertSeparator(m_pMessageLog->GetMenu()->actions()[0]);

	m_pMessageLog->GetView()->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pMessageLog->GetView()->setSortingEnabled(false);

	m_pLogTabs->addTab(m_pMessageLog, tr("Sbie Messages"));
	//

	m_pTraceView = new CTraceView(this);

	m_pTraceView->GetMenu()->insertAction(m_pTraceView->GetMenu()->actions()[0], m_pCleanUpTrace);
	m_pTraceView->GetMenu()->insertSeparator(m_pTraceView->GetMenu()->actions()[0]);

	m_pLogTabs->addTab(m_pTraceView, tr("Trace Log"));

	m_pHotkeyManager = new UGlobalHotkeys(this);
	connect(m_pHotkeyManager, SIGNAL(activated(size_t)), SLOT(OnHotKey(size_t)));
	SetupHotKeys();

	for (int i = 0; i < eMaxColor; i++) {
		m_BoxIcons[i].Empty = QIcon(QString(":/Boxes/Empty%1").arg(i));
		m_BoxIcons[i].InUse= QIcon(QString(":/Boxes/Full%1").arg(i));

		//QImage Image(QString(":/Boxes/Empty%1").arg(i));
		//Image.invertPixels();
		//m_BoxIcons[i].Busy = QIcon(QPixmap::fromImage(Image));

		QPixmap base = QPixmap(QString(":/Boxes/Empty%1").arg(i));
		QPixmap overlay= QPixmap(":/Boxes/Busy");
		QPixmap result(base.width(), base.height());
		result.fill(Qt::transparent); // force alpha channel
		QPainter painter(&result);
		painter.drawPixmap(0, 0, base);
		painter.drawPixmap(0, 0, overlay);
		m_BoxIcons[i].Busy = QIcon(result);
	}

	// Tray
	m_pTrayIcon = new QSystemTrayIcon(GetTrayIcon(), this);
	m_pTrayIcon->setToolTip(GetTrayText());
	connect(m_pTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(OnSysTray(QSystemTrayIcon::ActivationReason)));
	m_bIconEmpty = true;
	m_bIconDisabled = false;
	m_bIconBusy = false;
	m_iDeletingContent = 0;

	m_pTrayMenu = new QMenu();
	QAction* pShowHide = m_pTrayMenu->addAction(GetIcon("IconFull", false), tr("Show/Hide"), this, SLOT(OnShowHide()));
	QFont f = pShowHide->font();
	f.setBold(true);
	pShowHide->setFont(f);
	m_pTrayMenu->addSeparator();

	m_pTrayList = new QWidgetAction(m_pTrayMenu);

	QWidget* pWidget = new CActionWidget();
    QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setMargin(0);
	pWidget->setLayout(pLayout);

	m_pTrayBoxes = new QTreeWidget();

	m_pTrayBoxes->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
	m_pTrayBoxes->setRootIsDecorated(false);
	//m_pTrayBoxes->setHeaderLabels(tr("         Sandbox").split("|"));
	m_pTrayBoxes->setHeaderHidden(true);
	m_pTrayBoxes->setSelectionMode(QAbstractItemView::NoSelection);
	//m_pTrayBoxes->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//m_pTrayBoxes->setStyleSheet("QTreeView::item:hover{background-color:#FFFF00;}");
	m_pTrayBoxes->setItemDelegate(new CTrayBoxesItemDelegate());

	pLayout->insertSpacing(0, 1);// 32);

	/*QFrame* vFrame = new QFrame;
	vFrame->setFixedWidth(1);
	vFrame->setFrameShape(QFrame::VLine);
	vFrame->setFrameShadow(QFrame::Raised);
	pLayout->addWidget(vFrame);*/
	
	pLayout->addWidget(m_pTrayBoxes);

    m_pTrayList->setDefaultWidget(pWidget);
	m_pTrayMenu->addAction(m_pTrayList);


	m_pTrayBoxes->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pTrayBoxes, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(OnBoxMenu(const QPoint &)));
	connect(m_pTrayBoxes, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(OnBoxDblClick(QTreeWidgetItem*)));
	//m_pBoxMenu

	m_pTraySeparator = m_pTrayMenu->addSeparator();
	m_pTrayMenu->addAction(m_pEmptyAll);
	m_pDisableForce2 = m_pTrayMenu->addAction(tr("Pause Forcing Programs"), this, SLOT(OnDisableForce2()));
	m_pDisableForce2->setCheckable(true);
	m_pTrayMenu->addSeparator();

	/*QWidgetAction* pBoxWidget = new QWidgetAction(m_pTrayMenu);

	QWidget* pWidget = new QWidget();
	pWidget->setMaximumHeight(200);
	QGridLayout* pLayout = new QGridLayout();
	pLayout->addWidget(pBar, 0, 0);
	pWidget->setLayout(pLayout);
	pBoxWidget->setDefaultWidget(pWidget);*/

	/*QLabel* pLabel = new QLabel("test");
	pLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	pLabel->setAlignment(Qt::AlignCenter);
	pBoxWidget->setDefaultWidget(pLabel);*/

	//m_pTrayMenu->addAction(pBoxWidget);
	//m_pTrayMenu->addSeparator();

	m_pTrayMenu->addAction(m_pExit);

	bool bAutoRun = QApplication::arguments().contains("-autorun");

	if(g_PendingMessage.isEmpty()){
	m_pTrayIcon->show(); // Note: qt bug; hide does not work if not showing first :/
	if(!bAutoRun && theConf->GetInt("Options/SysTrayIcon", 1) == 0)
		m_pTrayIcon->hide();
	}
	//

	LoadState();

	bool bAdvanced = theConf->GetBool("Options/AdvancedView", true);
	foreach(QAction * pAction, m_pViewMode->actions())
		pAction->setChecked(pAction->data().toBool() == bAdvanced);
	SetViewMode(bAdvanced);


	m_pKeepTerminated->setChecked(theConf->GetBool("Options/KeepTerminated"));
	m_pShowAllSessions->setChecked(theConf->GetBool("Options/ShowAllSessions"));

	m_pProgressDialog = new CProgressDialog("");
	m_pProgressDialog->setWindowModality(Qt::ApplicationModal);
	connect(m_pProgressDialog, SIGNAL(Cancel()), this, SLOT(OnCancelAsync()));
	m_pProgressModal = false;

	m_pPopUpWindow = new CPopUpWindow();

	bool bAlwaysOnTop = theConf->GetBool("Options/AlwaysOnTop", false);
	m_pWndTopMost->setChecked(bAlwaysOnTop);
	this->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);
	m_pPopUpWindow->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);
	m_pProgressDialog->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);


	//connect(theAPI, SIGNAL(LogMessage(const QString&, bool)), this, SLOT(OnLogMessage(const QString&, bool)));
	connect(theAPI, SIGNAL(LogSbieMessage(quint32, const QStringList&, quint32)), this, SLOT(OnLogSbieMessage(quint32, const QStringList&, quint32)));
	connect(theAPI, SIGNAL(NotAuthorized(bool, bool&)), this, SLOT(OnNotAuthorized(bool, bool&)), Qt::DirectConnection);
	connect(theAPI, SIGNAL(QueuedRequest(quint32, quint32, quint32, const QVariantMap&)), this, SLOT(OnQueuedRequest(quint32, quint32, quint32, const QVariantMap&)), Qt::QueuedConnection);
	connect(theAPI, SIGNAL(FileToRecover(const QString&, const QString&, const QString&, quint32)), this, SLOT(OnFileToRecover(const QString&, const QString&, const QString&, quint32)), Qt::QueuedConnection);
	connect(theAPI, SIGNAL(ConfigReloaded()), this, SLOT(OnIniReloaded()));

	m_uTimerID = startTimer(1000);

	if (!bAutoRun && g_PendingMessage.isEmpty())
		SafeShow(this);

	OnStatusChanged();
	if (CSbieUtils::IsRunning(CSbieUtils::eAll) || theConf->GetBool("Options/StartIfStopped", true))
	{
		SB_RESULT(void*) Status = ConnectSbie();
		HandleMaintenance(Status);
	}

	//qApp->setWindowIcon(GetIcon("IconEmptyDC", false));
}

CSandMan::~CSandMan()
{
	m_pPopUpWindow->close();
	delete m_pPopUpWindow;

	if(m_pEnableMonitoring->isChecked())
		theAPI->EnableMonitor(false);

	killTimer(m_uTimerID);

	m_pTrayIcon->hide();

	StoreState();

	theAPI = NULL;

	theGUI = NULL;
}

void CSandMan::LoadState()
{
    setWindowState(Qt::WindowNoState);
	restoreGeometry(theConf->GetBlob("MainWindow/Window_Geometry"));
	restoreState(theConf->GetBlob("MainWindow/Window_State"));
	//m_pBoxTree->restoreState(theConf->GetBlob("MainWindow/BoxTree_Columns"));
	m_pMessageLog->GetView()->header()->restoreState(theConf->GetBlob("MainWindow/LogList_Columns"));
	m_pLogSplitter->restoreState(theConf->GetBlob("MainWindow/Log_Splitter"));
	m_pPanelSplitter->restoreState(theConf->GetBlob("MainWindow/Panel_Splitter"));
	m_pLogTabs->setCurrentIndex(theConf->GetInt("MainWindow/LogTab", 0));
}

void CSandMan::StoreState()
{
	theConf->SetBlob("MainWindow/Window_Geometry", saveGeometry());
	theConf->SetBlob("MainWindow/Window_State", saveState());
	//theConf->SetBlob("MainWindow/BoxTree_Columns", m_pBoxTree->saveState());
	theConf->SetBlob("MainWindow/LogList_Columns", m_pMessageLog->GetView()->header()->saveState());
	theConf->SetBlob("MainWindow/Log_Splitter", m_pLogSplitter->saveState());
	theConf->SetBlob("MainWindow/Panel_Splitter", m_pPanelSplitter->saveState());
	theConf->SetValue("MainWindow/LogTab", m_pLogTabs->currentIndex());
}

QIcon CSandMan::GetIcon(const QString& Name, bool bAction)
{
	QString Path = QApplication::applicationDirPath() + "/Icons/" + Name + ".png";
	if(QFile::exists(Path))
		return QIcon(Path);
	return QIcon((bAction ? ":/Actions/" : ":/") + Name + ".png");
}

void CSandMan::CreateMenus()
{
	connect(menuBar(), SIGNAL(hovered(QAction*)), this, SLOT(OnMenuHover(QAction*)));

	m_pMenuFile = menuBar()->addMenu(tr("&Sandbox"));
		m_pNewBox = m_pMenuFile->addAction(CSandMan::GetIcon("NewBox"), tr("Create New Box"), this, SLOT(OnNewBox()));
		m_pNewGroup = m_pMenuFile->addAction(CSandMan::GetIcon("Group"), tr("Create Box Group"), this, SLOT(OnNewGroupe()));
		m_pMenuFile->addSeparator();
		m_pEmptyAll = m_pMenuFile->addAction(CSandMan::GetIcon("EmptyAll"), tr("Terminate All Processes"), this, SLOT(OnEmptyAll()));
		m_pWndFinder = m_pMenuFile->addAction(CSandMan::GetIcon("finder"), tr("Window Finder"), this, SLOT(OnWndFinder()));
		m_pDisableForce = m_pMenuFile->addAction(tr("Pause Forcing Programs"), this, SLOT(OnDisableForce()));
		m_pDisableForce->setCheckable(true);
		m_pMenuFile->addSeparator();
		m_pMaintenance = m_pMenuFile->addMenu(CSandMan::GetIcon("Maintenance"), tr("&Maintenance"));
			m_pConnect = m_pMaintenance->addAction(CSandMan::GetIcon("Connect"), tr("Connect"), this, SLOT(OnMaintenance()));
			m_pDisconnect = m_pMaintenance->addAction(CSandMan::GetIcon("Disconnect"), tr("Disconnect"), this, SLOT(OnMaintenance()));
			m_pMaintenance->addSeparator();
			m_pStopAll = m_pMaintenance->addAction(CSandMan::GetIcon("Stop"), tr("Stop All"), this, SLOT(OnMaintenance()));
			m_pMaintenanceItems = m_pMaintenance->addMenu(CSandMan::GetIcon("ManMaintenance"), tr("&Advanced"));
				m_pInstallDrv = m_pMaintenanceItems->addAction(tr("Install Driver"), this, SLOT(OnMaintenance()));
				m_pStartDrv = m_pMaintenanceItems->addAction(tr("Start Driver"), this, SLOT(OnMaintenance()));
				m_pStopDrv = m_pMaintenanceItems->addAction(tr("Stop Driver"), this, SLOT(OnMaintenance()));
				m_pUninstallDrv = m_pMaintenanceItems->addAction(tr("Uninstall Driver"), this, SLOT(OnMaintenance()));
				m_pMaintenanceItems->addSeparator();
				m_pInstallSvc = m_pMaintenanceItems->addAction(tr("Install Service"), this, SLOT(OnMaintenance()));
				m_pStartSvc = m_pMaintenanceItems->addAction(tr("Start Service"), this, SLOT(OnMaintenance()));
				m_pStopSvc = m_pMaintenanceItems->addAction(tr("Stop Service"), this, SLOT(OnMaintenance()));
				m_pUninstallSvc = m_pMaintenanceItems->addAction(tr("Uninstall Service"), this, SLOT(OnMaintenance()));
			m_pMaintenance->addSeparator();
			if(IsFullyPortable())
				m_pUninstallAll = m_pMaintenance->addAction(CSandMan::GetIcon("Uninstall"), tr("Uninstall All"), this, SLOT(OnMaintenance()));
			
				
		m_pMenuFile->addSeparator();
		m_pExit = m_pMenuFile->addAction(CSandMan::GetIcon("Exit"), tr("Exit"), this, SLOT(OnExit()));


	m_pMenuView = menuBar()->addMenu(tr("&View"));

		m_pViewMode = new QActionGroup(m_pMenuView);
		MakeAction(m_pViewMode, m_pMenuView, tr("Simple View"), false);
		MakeAction(m_pViewMode, m_pMenuView, tr("Advanced View"), true);
		connect(m_pViewMode, SIGNAL(triggered(QAction*)), this, SLOT(OnViewMode(QAction*)));

		m_pMenuView->addSeparator();
		m_pWndTopMost = m_pMenuView->addAction(tr("Always on Top"), this, SLOT(OnAlwaysTop()));
		m_pWndTopMost->setCheckable(true);

		m_iMenuViewPos = m_pMenuView->actions().count();
		m_pMenuView->addSeparator();

		m_pShowHidden = m_pMenuView->addAction(tr("Show Hidden Boxes"));
		m_pShowHidden->setCheckable(true);
		m_pShowAllSessions = m_pMenuView->addAction(tr("Show All Sessions"), this, SLOT(OnProcView()));
		m_pShowAllSessions->setCheckable(true);

		m_pMenuView->addSeparator();

		m_pCleanUpMenu = m_pMenuView->addMenu(CSandMan::GetIcon("Clean"), tr("Clean Up"));
			m_pCleanUpProcesses = m_pCleanUpMenu->addAction(tr("Cleanup Processes"), this, SLOT(OnCleanUp()));
			m_pCleanUpMenu->addSeparator();
			m_pCleanUpMsgLog = m_pCleanUpMenu->addAction(tr("Cleanup Message Log"), this, SLOT(OnCleanUp()));
			m_pCleanUpTrace = m_pCleanUpMenu->addAction(tr("Cleanup Trace Log"), this, SLOT(OnCleanUp()));

		m_pKeepTerminated = m_pMenuView->addAction(CSandMan::GetIcon("Keep"), tr("Keep terminated"), this, SLOT(OnProcView()));
		m_pKeepTerminated->setCheckable(true);


	m_pMenuOptions = menuBar()->addMenu(tr("&Options"));
		m_pMenuSettings = m_pMenuOptions->addAction(CSandMan::GetIcon("Settings"), tr("Global Settings"), this, SLOT(OnSettings()));
		m_pMenuResetMsgs = m_pMenuOptions->addAction(tr("Reset all hidden messages"), this, SLOT(OnResetMsgs()));
		m_pMenuResetGUI = m_pMenuOptions->addAction(tr("Reset all GUI options"), this, SLOT(OnResetGUI()));
		m_pMenuOptions->addSeparator();
		m_pEditIni = m_pMenuOptions->addAction(CSandMan::GetIcon("EditIni"), tr("Edit ini file"), this, SLOT(OnEditIni()));
		m_pReloadIni = m_pMenuOptions->addAction(CSandMan::GetIcon("ReloadIni"), tr("Reload ini file"), this, SLOT(OnReloadIni()));
		m_pMenuOptions->addSeparator();
		m_pEnableMonitoring = m_pMenuOptions->addAction(CSandMan::GetIcon("SetLogging"), tr("Trace Logging"), this, SLOT(OnSetMonitoring()));
		m_pEnableMonitoring->setCheckable(true);
		

	m_pMenuHelp = menuBar()->addMenu(tr("&Help"));
		//m_pMenuHelp->addAction(tr("Support Sandboxie-Plus on Patreon"), this, SLOT(OnHelp()));
		m_pSupport = m_pMenuHelp->addAction(tr("Support Sandboxie-Plus with a Donation"), this, SLOT(OnHelp()));
		m_pForum = m_pMenuHelp->addAction(tr("Visit Support Forum"), this, SLOT(OnHelp()));
		m_pManual = m_pMenuHelp->addAction(tr("Online Documentation"), this, SLOT(OnHelp()));
		m_pMenuHelp->addSeparator();
		m_pUpdate = m_pMenuHelp->addAction(tr("Check for Updates"), this, SLOT(CheckForUpdates()));
		m_pMenuHelp->addSeparator();
		m_pAboutQt = m_pMenuHelp->addAction(tr("About the Qt Framework"), this, SLOT(OnAbout()));
		m_pAbout = m_pMenuHelp->addAction(GetIcon("IconFull", false), tr("About Sandboxie-Plus"), this, SLOT(OnAbout()));
}

void CSandMan::CreateToolBar()
{
	m_pToolBar = new QToolBar();
	m_pMainLayout->insertWidget(0, m_pToolBar);

	m_pToolBar->addAction(m_pMenuSettings);
	m_pToolBar->addSeparator();

	//m_pToolBar->addAction(m_pMenuNew);
	//m_pToolBar->addAction(m_pMenuEmptyAll);
	//m_pToolBar->addSeparator();
	m_pToolBar->addAction(m_pKeepTerminated);
	//m_pToolBar->addAction(m_pCleanUp);

	m_pCleanUpButton = new QToolButton();
	m_pCleanUpButton->setIcon(CSandMan::GetIcon("Clean"));
	m_pCleanUpButton->setToolTip(tr("Cleanup"));
	m_pCleanUpButton->setPopupMode(QToolButton::MenuButtonPopup);
	m_pCleanUpButton->setMenu(m_pCleanUpMenu);
	//QObject::connect(m_pCleanUpButton, SIGNAL(triggered(QAction*)), , SLOT());
	QObject::connect(m_pCleanUpButton, SIGNAL(clicked(bool)), this, SLOT(OnCleanUp()));
	m_pToolBar->addWidget(m_pCleanUpButton);


	m_pToolBar->addSeparator();
	m_pToolBar->addAction(m_pEditIni);
	m_pToolBar->addSeparator();
	m_pToolBar->addAction(m_pEnableMonitoring);
	//m_pToolBar->addSeparator();
	

	if (!g_Certificate.isEmpty())
		return;

	QWidget* pSpacer = new QWidget();
	pSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_pToolBar->addWidget(pSpacer);

	//m_pToolBar->addAction(m_pMenuElevate);

	m_pToolBar->addSeparator();
	m_pToolBar->addWidget(new QLabel("        "));
	QLabel* pSupportLbl = new QLabel(tr("<a href=\"https://sandboxie-plus.com/go.php?to=patreon\">Support Sandboxie-Plus on Patreon</a>"));
	pSupportLbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
	connect(pSupportLbl, SIGNAL(linkActivated(const QString&)), this, SLOT(OnHelp()));
	m_pToolBar->addWidget(pSupportLbl);
	m_pToolBar->addWidget(new QLabel("        "));
}

void CSandMan::OnExit()
{
	m_bExit = true;
	close();
}

void CSandMan::closeEvent(QCloseEvent *e)
{
	if (!m_bExit)// && !theAPI->IsConnected())
	{
		QString OnClose = theConf->GetString("Options/OnClose", "ToTray");
		if (m_pTrayIcon->isVisible() && OnClose.compare("ToTray", Qt::CaseInsensitive) == 0)
		{
			StoreState();
			hide();

			if (theAPI->GetGlobalSettings()->GetBool("ForgetPassword", false))
				theAPI->ClearPassword();

			e->ignore();
			return;
		}
		else if(OnClose.compare("Prompt", Qt::CaseInsensitive) == 0)
		{
			CExitDialog ExitDialog(tr("Do you want to close Sandboxie Manager?"));
			if (!ExitDialog.exec())
			{
				e->ignore();
				return;
			}
		}
	}

	//if(theAPI->IsConnected())
	//	m_pBoxView->SaveUserConfig();

	if (IsFullyPortable() && theAPI->IsConnected())
	{
		int PortableStop = theConf->GetInt("Options/PortableStop", -1);
		if (PortableStop == -1)
		{
			bool State = false;
			auto Ret = CCheckableMessageBox::question(this, "Sandboxie-Plus", tr("Sandboxie-Plus was running in portable mode, now it has to clean up the created services. This will prompt for administrative privileges.\n\nDo you want to do the clean up?")
				, tr("Don't show this message again."), &State, QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel, QDialogButtonBox::Yes, QMessageBox::Question);

			if (Ret == QDialogButtonBox::Cancel)
			{
				e->ignore();
				return;
			}

			PortableStop = (Ret == QDialogButtonBox::Yes) ? 1 : 0;

			if (State)
				theConf->SetValue("Options/PortableStop", PortableStop);
		}

		if (PortableStop == 1) {
			SB_RESULT(void*) Status = StopSbie(true);
			// don't care for Status we quit here anyways
		}
	}

	QApplication::quit();
}

QIcon CSandMan::GetBoxIcon(int boxType, bool inUse, bool inBusy)
{
	EBoxColors color = eYellow;
	switch (boxType) {
	case CSandBoxPlus::eHardenedPlus:		color = eRed; break;
	case CSandBoxPlus::eHardened:			color = eOrang; break;
	case CSandBoxPlus::eDefaultPlus:		color = eBlue; break;
	case CSandBoxPlus::eDefault:			color = eYellow; break;
	case CSandBoxPlus::eAppBoxPlus:			color = eCyan; break;
	case CSandBoxPlus::eAppBox:				color = eGreen; break;
	case CSandBoxPlus::eInsecure:			color = eMagenta; break;
	}
	if (inBusy)
		return m_BoxIcons[color].Busy;
	if (inUse)
		return m_BoxIcons[color].InUse;
	return m_BoxIcons[color].Empty;
}

QString CSandMan::GetBoxDescription(int boxType)
{
	QString Info;

	switch (boxType) {
	case CSandBoxPlus::eHardenedPlus:
	case CSandBoxPlus::eHardened:
		Info = tr("This box provides enhanced security isolation, it is suitable to test untrusted software.");
		break;
	case CSandBoxPlus::eDefaultPlus:
	case CSandBoxPlus::eDefault:
		Info = tr("This box provides standard isolation, it is suitable to run your software to enhance security.");	
		break;
	case CSandBoxPlus::eAppBoxPlus:
	case CSandBoxPlus::eAppBox:
		Info = tr("This box does not enforce isolation, it is intended to be used as an application compartment for software virtualization only.");
		break;
	}
	
	if(boxType == CSandBoxPlus::eHardenedPlus || boxType == CSandBoxPlus::eDefaultPlus || boxType == CSandBoxPlus::eAppBoxPlus)
		Info.append(tr("\n\nThis box prevents access to all user data locations, except explicitly granted in the Resource Access options."));

	return Info;
}

bool CSandMan::IsFullyPortable()
{
	QString SbiePath = theAPI->GetSbiePath();
	QString IniPath = theAPI->GetIniPath();
	if (IniPath.indexOf(SbiePath, 0, Qt::CaseInsensitive) == 0)
		return true;
	return false;
}

void CSandMan::OnMessage(const QString& MsgData)
{
	QStringList Messages = MsgData.split("\n");
	QString Message = Messages[0];
	if (Message == "ShowWnd")
	{
		if (!isVisible())
			show();
		setWindowState(Qt::WindowActive);
		SetForegroundWindow(MainWndHandle);
	}
	else if (Message.left(4) == "Run:")
	{
		QString BoxName = "DefaultBox";
		QString CmdLine = Message.mid(4);

		if (CmdLine.contains("\\start.exe", Qt::CaseInsensitive)) {
			int pos = CmdLine.indexOf("/box:", 0, Qt::CaseInsensitive);
			int pos2 = CmdLine.indexOf(" ", pos);
			if (pos != -1 && pos2 != -1) {
				BoxName = CmdLine.mid(pos + 5, pos2 - (pos + 5));
				CmdLine = CmdLine.mid(pos2 + 1);
			}
		}

		QString WrkDir;
		for (int i = 1; i < Messages.length(); i++) {
			if (Messages[i].left(5) == "From:") {
				WrkDir = Messages[i].mid(5);
				break;
			}
		}

		if (theConf->GetBool("Options/RunInDefaultBox", false) && (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier) == 0) {
			theAPI->RunStart("DefaultBox", CmdLine, false, WrkDir);
		}
		else
			RunSandboxed(QStringList(CmdLine), BoxName, WrkDir);
	}
	else if (Message.left(3) == "Op:")
	{
		QString Op = Message.mid(3);

		SB_RESULT(void*) Status;
		if (Op == "Connect")
			Status = ConnectSbie();
		else if (Op == "Disconnect")
			Status = DisconnectSbie();
		else if (Op == "Shutdown")
			Status = StopSbie();
		else if (Op == "EmptyAll")
			Status = theAPI->TerminateAll();
		else
			Status = SB_ERR(SB_Message, QVariantList () << (tr("Unknown operation '%1' requested via command line").arg(Op)));

		HandleMaintenance(Status);
	}
}

void CSandMan::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

bool CSandMan::RunSandboxed(const QStringList& Commands, const QString& BoxName, const QString& WrkDir)
{
	CSelectBoxWindow* pSelectBoxWindow = new CSelectBoxWindow(Commands, BoxName, WrkDir);
	//pSelectBoxWindow->show();
	return SafeExec(pSelectBoxWindow) == 1;
}

void CSandMan::dropEvent(QDropEvent* e)
{
	QStringList Commands;
	foreach(const QUrl & url, e->mimeData()->urls()) {
		if (url.isLocalFile())
			Commands.append(url.toLocalFile().replace("/", "\\"));
	}

	RunSandboxed(Commands, "DefaultBox");
}

QIcon CSandMan::GetTrayIcon(bool isConnected)
{
	bool bClassic = (theConf->GetInt("Options/SysTrayIcon", 1) == 2);

	QString IconFile;
	if (isConnected) {
		if (m_bIconEmpty)
			IconFile = "IconEmpty";
		else
			IconFile = "IconFull";
	} else 
		IconFile = "IconOff";
	if (bClassic) IconFile += "C";

	QSize size = QSize(16, 16);
	QPixmap result(size);
	result.fill(Qt::transparent); // force alpha channel
	QPainter painter(&result);
	QPixmap base = GetIcon(IconFile, false).pixmap(size);
	QPixmap overlay;

	if (m_bIconBusy) {
		IconFile = "IconBusy";
		if (bClassic) { // classic has a different icon instead of an overlay
			IconFile += "C";
			base = GetIcon(IconFile, false).pixmap(size);
		}
		else
			overlay = GetIcon(IconFile, false).pixmap(size);
	}

	painter.drawPixmap(0, 0, base);
	if(!overlay.isNull()) painter.drawPixmap(0, 0, overlay);

	if (m_bIconDisabled) {
		IconFile = "IconDFP";
		if (bClassic) IconFile += "C";
		overlay = GetIcon(IconFile, false).pixmap(size);
		painter.drawPixmap(0, 0, overlay);
	}

	return QIcon(result);
}

QString CSandMan::GetTrayText(bool isConnected)
{
	QString Text = "Sandboxie-Plus";

	if(!isConnected)
		Text +=  tr(" - Driver/Service NOT Running!");
	else if(m_iDeletingContent)
		Text += tr(" - Deleting Sandbox Content");

	return Text;
}

void CSandMan::timerEvent(QTimerEvent* pEvent)
{
	if (pEvent->timerId() != m_uTimerID)
		return;

	bool bForceProcessDisabled = false;
	bool bIconBusy = false;
	bool bConnected = false;

	if (theAPI->IsConnected())
	{
		SB_STATUS Status = theAPI->ReloadBoxes();

		theAPI->UpdateProcesses(m_pKeepTerminated->isChecked(), m_pShowAllSessions->isChecked());

		bForceProcessDisabled = theAPI->AreForceProcessDisabled();
		m_pDisableForce->setChecked(bForceProcessDisabled);
		m_pDisableForce2->setChecked(bForceProcessDisabled);


		bool bIsMonitoring = theAPI->IsMonitoring();
		m_pEnableMonitoring->setChecked(bIsMonitoring);
		if (!bIsMonitoring) // don't disable the view as logn as there are entries shown
			bIsMonitoring = !theAPI->GetTrace().isEmpty();
		m_pTraceView->setEnabled(bIsMonitoring);

		QMap<quint32, CBoxedProcessPtr> Processes = theAPI->GetAllProcesses();
		int ActiveProcesses = 0;
		if (m_pKeepTerminated->isChecked()) {
			foreach(const CBoxedProcessPtr & Process, Processes) {
				if (!Process->IsTerminated())
					ActiveProcesses++;
			}
		}
		else 
			ActiveProcesses = Processes.count();

		if (theAPI->IsBusy() || m_iDeletingContent > 0)
			bIconBusy = true;

		if (m_bIconEmpty != (ActiveProcesses == 0)  || m_bIconBusy != bIconBusy || m_bIconDisabled != bForceProcessDisabled)
		{
			m_bIconEmpty = (ActiveProcesses == 0);
			m_bIconBusy = bIconBusy;
			m_bIconDisabled = bForceProcessDisabled;

			m_pTrayIcon->setIcon(GetTrayIcon());
			m_pTrayIcon->setToolTip(GetTrayText());
		}
	}

	if (!isVisible() || windowState().testFlag(Qt::WindowMinimized))
		return;

	theAPI->UpdateWindowMap();

	m_pBoxView->Refresh();
	m_pTraceView->Refresh();

	OnSelectionChanged();

	int iCheckUpdates = theConf->GetInt("Options/CheckForUpdates", 2);
	if (iCheckUpdates != 0)
	{
		time_t NextUpdateCheck = theConf->GetUInt64("Options/NextCheckForUpdates", 0);
		if (NextUpdateCheck == 0)
			theConf->SetValue("Options/NextCheckForUpdates", QDateTime::currentDateTime().addDays(7).toTime_t());
		else if(QDateTime::currentDateTime().toTime_t() >= NextUpdateCheck)
		{
			if (iCheckUpdates == 2)
			{
				bool bCheck = false;
				iCheckUpdates = CCheckableMessageBox::question(this, "Sandboxie-Plus", tr("Do you want to check if there is a new version of Sandboxie-Plus?")
					, tr("Don't show this message again."), &bCheck, QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes, QMessageBox::Information) == QDialogButtonBox::Ok ? 1 : 0;

				if (bCheck)
					theConf->SetValue("Options/CheckForUpdates", iCheckUpdates);
			}

			if (iCheckUpdates == 0)
				theConf->SetValue("Options/NextCheckForUpdates", QDateTime::currentDateTime().addDays(7).toTime_t());
			else
			{
				theConf->SetValue("Options/NextCheckForUpdates", QDateTime::currentDateTime().addDays(1).toTime_t());
				
				CheckForUpdates(false);
			}
		}
	}

	if (!m_pUpdateProgress.isNull() && m_RequestManager != NULL) {
		if (m_pUpdateProgress->IsCanceled()) {
			m_pUpdateProgress->Finish(SB_OK);
			m_pUpdateProgress.clear();

			m_RequestManager->AbortAll();
		}
	}

	if (!m_MissingTemplates.isEmpty())
	{
		if (m_MissingTemplates[0] == "") {
			m_MissingTemplates.clear();
			return;
		}

		int CleanupTemplates = theConf->GetInt("Options/AutoCleanupTemplates", -1);
		if (CleanupTemplates == -1)
		{
			bool State = false;
			CleanupTemplates = CCheckableMessageBox::question(this, "Sandboxie-Plus", tr("Some compatibility templates (%1) are missing, probably deleted, do you want to remove them from all boxes?")
				.arg(m_MissingTemplates.join(", "))
				, tr("Don't show this message again."), &State, QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes, QMessageBox::Information) == QDialogButtonBox::Yes ? 1 : 0;

			if (State)
				theConf->SetValue("Options/AutoCleanupTemplates", CleanupTemplates);
		}

		if (CleanupTemplates)
		{
			foreach(const QString& Template, m_MissingTemplates)
			{
				theAPI->GetGlobalSettings()->DelValue("Template", Template);
				foreach(const CSandBoxPtr& pBox, theAPI->GetAllBoxes())
					pBox->DelValue("Template", Template);
			}

			OnLogMessage(tr("Cleaned up removed templates..."));
		}
		m_MissingTemplates.clear();

		m_MissingTemplates.append("");
	}
}

SB_STATUS CSandMan::DeleteBoxContent(const CSandBoxPtr& pBox, EDelMode Mode, bool DeleteShapshots)
{
	SB_STATUS Ret = SB_OK;
	m_iDeletingContent++;

	if (Mode != eAuto) {
		Ret = pBox->TerminateAll();
		if (Ret.IsError())
			goto finish;
	}

	if (Mode != eForDelete) {
		foreach(const QString & Value, pBox->GetTextList("OnBoxDelete", true, false, true)) {
			QString Value2 = pBox->Expand(Value);
			CSbieProgressPtr pProgress = CSbieUtils::RunCommand(Value2, true);
			if (!pProgress.isNull()) {
				AddAsyncOp(pProgress, true, tr("Executing OnBoxDelete: %1").arg(Value2));
				if (pProgress->IsCanceled()) {
					Ret = CSbieStatus(SB_Canceled);
					goto finish;
				}
			}
		}
	}
	
	{
		SB_PROGRESS Status;
		if (Mode != eForDelete && !DeleteShapshots && pBox->HasSnapshots()) { // in auto delete mdoe always return to last snapshot
			QString Current;
			QString Default = pBox->GetDefaultSnapshot(&Current);
			Status = pBox->SelectSnapshot(Mode == eAuto ? Current : Default);
		}
		else // if there are no snapshots just use the normal cleaning procedure
			Status = pBox->CleanBox();

		Ret = Status;
		if (Status.GetStatus() == OP_ASYNC)
			Ret = AddAsyncOp(Status.GetValue(), true, tr("Auto Deleting %1 Content").arg(pBox->GetName()));
	}

finish:
	m_iDeletingContent--;
	return Ret;
}

void CSandMan::OnBoxClosed(const QString& BoxName)
{
	CSandBoxPtr pBox = theAPI->GetBoxByName(BoxName);
	if (!pBox)
		return;

	if (!pBox->GetBool("NeverDelete", false) && pBox->GetBool("AutoDelete", false) && !pBox->IsEmpty())
	{
		bool DeleteShapshots = false;
		// if this box auto deletes first show the recovry dialog with the option to abort deletion
		if(!theGUI->OpenRecovery(pBox, DeleteShapshots, true)) // unless no files are found than continue silently
			return;

		if(theConf->GetBool("Options/AutoBoxOpsNotify", false))
			OnLogMessage(tr("Auto deleting content of %1").arg(BoxName), true);

		if (theConf->GetBool("Options/UseAsyncBoxOps", false))
		{
			auto pBoxEx = pBox.objectCast<CSandBoxPlus>();
			SB_STATUS Status = pBoxEx->DeleteContentAsync(DeleteShapshots);
			CheckResults(QList<SB_STATUS>() << Status);
		}
		else
			DeleteBoxContent(pBox, eAuto, DeleteShapshots);
	}
}

void CSandMan::OnSelectionChanged()
{
	//QList<CBoxedProcessPtr>	Processes = m_pBoxView->GetSelectedProcesses();
	/*if (Processes.isEmpty())
	{
		QList<CSandBoxPtr>Boxes = m_pBoxView->GetSelectedBoxes();
		foreach(const CSandBoxPtr& pBox, Boxes)
			Processes.append(pBox->GetProcessList().values());
	}*/

	//QSet<quint64> Pids;
	//foreach(const CBoxedProcessPtr& pProcess, Processes)
	//	Pids.insert(pProcess->GetProcessId());
}

void CSandMan::OnStatusChanged()
{
	bool isConnected = theAPI->IsConnected();

	QString appTitle = tr("Sandboxie-Plus v%1").arg(GetVersion());
	if (isConnected)
	{
		QString SbiePath = theAPI->GetSbiePath();
		OnLogMessage(tr("Installation Directory: %1").arg(SbiePath));
		OnLogMessage(tr("Sandboxie-Plus Version: %1 (%2)").arg(GetVersion()).arg(theAPI->GetVersion()));
		OnLogMessage(tr("Loaded Config: %1").arg(theAPI->GetIniPath()));
		OnLogMessage(tr("Data Directory: %1").arg(QString(theConf->GetConfigDir()).replace("/","\\")));

		//statusBar()->showMessage(tr("Driver version: %1").arg(theAPI->GetVersion()));

		//appTitle.append(tr("   -   Driver: v%1").arg(theAPI->GetVersion()));
		if (IsFullyPortable())
		{
			appTitle.append(tr("   -   Portable"));

			QString BoxPath = QDir::cleanPath(QApplication::applicationDirPath() + "/../Sandbox").replace("/", "\\");

			int PortableRootDir = theConf->GetInt("Options/PortableRootDir", 2);
			if (PortableRootDir == 2)
			{
				bool State = false;
				PortableRootDir = CCheckableMessageBox::question(this, "Sandboxie-Plus", 
					tr("Sandboxie-Plus was started in portable mode, do you want to put the Sandbox folder into its parent directory?\nYes will choose: %1\nNo will choose: %2")
					.arg(BoxPath)
					.arg("C:\\Sandbox") // todo resolve os drive properly
					, tr("Don't show this message again."), &State, QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes, QMessageBox::Information) == QDialogButtonBox::Yes ? 1 : 0;

				if (State)
					theConf->SetValue("Options/PortableRootDir", PortableRootDir);
			}

			if (PortableRootDir)
				theAPI->GetGlobalSettings()->SetText("FileRootPath", BoxPath + "\\%SANDBOX%");
		}

		if (theConf->GetBool("Options/AutoRunSoftCompat", true))
		{
			if (m_SbieTemplates->RunCheck())
			{
				CSettingsWindow* pSettingsWindow = new CSettingsWindow();
				//connect(pSettingsWindow, SIGNAL(OptionsChanged()), this, SLOT(UpdateSettings()));
				pSettingsWindow->showCompat();
			}
		}

		if (SbiePath.compare(QApplication::applicationDirPath().replace("/", "\\"), Qt::CaseInsensitive) == 0)
		{
			if (theAPI->GetUserSettings()->GetText("SbieCtrl_AutoStartAgent").isEmpty())
				theAPI->GetUserSettings()->SetText("SbieCtrl_AutoStartAgent", "SandMan.exe");

			QString cmd = CSbieUtils::GetContextMenuStartCmd();
			if (!cmd.isEmpty() && !cmd.contains("SandMan.exe", Qt::CaseInsensitive))
				CSettingsWindow__AddContextMenu();
		}

		m_pBoxView->Clear();

		OnIniReloaded();

		if (theConf->GetBool("Options/WatchIni", true))
			theAPI->WatchIni(true);

		if (!theAPI->ReloadCert().IsError()) {
			CSettingsWindow::LoadCertificate();
			UpdateCertState();

			if ((g_CertInfo.expired || g_CertInfo.about_to_expire) && !theConf->GetBool("Options/NoSupportCheck", false)) 
			{
				CSettingsWindow* pSettingsWindow = new CSettingsWindow();
				//connect(pSettingsWindow, SIGNAL(OptionsChanged()), this, SLOT(UpdateSettings()));
				pSettingsWindow->showSupport();
			}
		}
		else {
			g_Certificate.clear();
			g_CertInfo.State = 0;

			QString CertPath = QCoreApplication::applicationDirPath() + "\\Certificate.dat";
			if(QFile::exists(CertPath)) // always delete invalid certificates
				WindowsMoveFile(CertPath.replace("/", "\\"), "");
		}
		
		g_FeatureFlags = theAPI->GetFeatureFlags();

		SB_STATUS Status = theAPI->ReloadBoxes();

		if (!Status.IsError() && !theAPI->GetAllBoxes().contains("defaultbox")) {
			OnLogMessage(tr("Default sandbox not found; creating: %1").arg("DefaultBox"));
			theAPI->CreateBox("DefaultBox");
		}
	}
	else
	{
		appTitle.append(tr("   -   NOT connected").arg(theAPI->GetVersion()));

		m_pBoxView->Clear();

		theAPI->WatchIni(false);
	}

	m_pSupport->setVisible(g_Certificate.isEmpty());

	this->setWindowTitle(appTitle);

	m_pTrayIcon->setIcon(GetTrayIcon(isConnected));
	m_pTrayIcon->setToolTip(GetTrayText(isConnected));
	m_bIconEmpty = true;
	m_bIconDisabled = false;
	m_bIconBusy = false;

	m_pNewBox->setEnabled(isConnected);
	m_pNewGroup->setEnabled(isConnected);
	m_pEmptyAll->setEnabled(isConnected);
	m_pDisableForce->setEnabled(isConnected);
	m_pDisableForce2->setEnabled(isConnected);

	//m_pCleanUpMenu->setEnabled(isConnected);
	//m_pCleanUpButton->setEnabled(isConnected);
	//m_pKeepTerminated->setEnabled(isConnected);

	m_pEditIni->setEnabled(isConnected);
	m_pReloadIni->setEnabled(isConnected);
	m_pEnableMonitoring->setEnabled(isConnected);
}

void CSandMan::OnMenuHover(QAction* action)
{
	//if (!menuBar()->actions().contains(action))
	//	return; // ignore sub menus


	if (menuBar()->actions().at(0) == action)
	{
		bool bConnected = theAPI->IsConnected();
		m_pConnect->setEnabled(!bConnected);
		m_pDisconnect->setEnabled(bConnected);

		m_pMaintenanceItems->setEnabled(!bConnected);

		bool DrvInstalled = CSbieUtils::IsInstalled(CSbieUtils::eDriver);
		bool DrvLoaded = CSbieUtils::IsRunning(CSbieUtils::eDriver);
		m_pInstallDrv->setEnabled(!DrvInstalled);
		m_pStartDrv->setEnabled(!DrvLoaded);
		m_pStopDrv->setEnabled(DrvLoaded);
		m_pUninstallDrv->setEnabled(DrvInstalled);

		bool SvcInstalled = CSbieUtils::IsInstalled(CSbieUtils::eService);
		bool SvcStarted = CSbieUtils::IsRunning(CSbieUtils::eService);
		m_pInstallSvc->setEnabled(!SvcInstalled);
		m_pStartSvc->setEnabled(!SvcStarted && DrvInstalled);
		m_pStopSvc->setEnabled(SvcStarted);
		m_pUninstallSvc->setEnabled(SvcInstalled);

		//m_pMenuStopAll - always enabled
	}
}

#define HK_PANIC 1

void CSandMan::SetupHotKeys()
{
	m_pHotkeyManager->unregisterAllHotkeys();

	if (theConf->GetBool("Options/EnablePanicKey", false))
		m_pHotkeyManager->registerHotkey(theConf->GetString("Options/PanicKeySequence", "Shift+Pause"), HK_PANIC);
}

void CSandMan::OnHotKey(size_t id)
{
	switch (id)
	{
	case HK_PANIC: 
		theAPI->TerminateAll();
		break;
	}
}

void CSandMan::OnLogMessage(const QString& Message, bool bNotify)
{
	QTreeWidgetItem* pItem = new QTreeWidgetItem(); // Time|Message
	pItem->setText(0, QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
	pItem->setText(1, Message);
	m_pMessageLog->GetTree()->addTopLevelItem(pItem);

	m_pMessageLog->GetView()->verticalScrollBar()->setValue(m_pMessageLog->GetView()->verticalScrollBar()->maximum());

	if (bNotify) {
		statusBar()->showMessage(Message);
		m_pTrayIcon->showMessage("Sandboxie-Plus", Message);
	}
}

void CSandMan::OnLogSbieMessage(quint32 MsgCode, const QStringList& MsgData, quint32 ProcessId)
{
	if ((MsgCode & 0xFFFF) == 2198) // file migration progress
	{
		m_pPopUpWindow->ShowProgress(MsgCode, MsgData, ProcessId);
		return;
	}

	if ((MsgCode & 0xFFFF) == 1411) // removed/missing template
	{
		if(MsgData.size() >= 3 && !m_MissingTemplates.contains(MsgData[2]))
			m_MissingTemplates.append(MsgData[2]);
	}

	if ((MsgCode & 0xFFFF) == 6004) // certificat error
	{
		static quint64 iLastCertWarning = 0;
		if (iLastCertWarning + 60 < QDateTime::currentDateTime().toTime_t()) { // reset after 60 seconds
			iLastCertWarning = QDateTime::currentDateTime().toTime_t();
			
			QString Message;
			if (!MsgData[2].isEmpty())
				Message = tr("The program %1 started in box %2 will be terminated in 5 minutes because the box was configured to use features exclusively available to project supporters.").arg(MsgData[2]).arg(MsgData[1]);
			else 
				Message = tr("The box %1 is configured to use features exclusively available to project supporters, these presets will be ignored.").arg(MsgData[1]);
			Message.append(tr("<br /><a href=\"https://sandboxie-plus.com/go.php?to=sbie-get-cert\">Become a project supporter</a>, and receive a <a href=\"https://sandboxie-plus.com/go.php?to=sbie-cert\">supporter certificate</a>"));

			QMessageBox msgBox(this);
			msgBox.setTextFormat(Qt::RichText);
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setWindowTitle("Sandboxie-Plus");
			msgBox.setText(Message);
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.exec();
			/*msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			if (msgBox.exec() == QDialogButtonBox::Yes) {
				OpenUrl(QUrl("https://sandboxie-plus.com/go.php?to=sbie-get-cert"));
			}*/
			
			//bCertWarning = false;
		}
		// return;
	}

	QString Message = MsgCode != 0 ? theAPI->GetSbieMsgStr(MsgCode, m_LanguageId) : (MsgData.size() > 0 ? MsgData[0] : QString());

	for (int i = 1; i < MsgData.size(); i++)
		Message = Message.arg(MsgData[i]);

	if (ProcessId != 4) // if it's not from the driver, add the pid
	{
		CBoxedProcessPtr pProcess = theAPI->GetProcessById(ProcessId);
		if(pProcess.isNull())
			Message.prepend(tr("PID %1: ").arg(ProcessId));
		else
			Message.prepend(tr("%1 (%2): ").arg(pProcess->GetProcessName()).arg(ProcessId));
	}

	OnLogMessage(Message);

	if ((MsgCode & 0xFFFF) == 6004) // certificat error
		return; // dont pop that one up

	if(MsgCode != 0 && theConf->GetBool("Options/ShowNotifications", true))
		m_pPopUpWindow->AddLogMessage(Message, MsgCode, MsgData, ProcessId);
}

bool CSandMan::CheckCertificate() 
{
	if ((g_FeatureFlags & CSbieAPI::eSbieFeatureCert) != 0)
		return true;

	//if ((g_FeatureFlags & CSbieAPI::eSbieFeatureCert) == 0) {
	//	OnLogMessage(tr("The supporter certificate is expired"));
	//	return false;
	//}

	QMessageBox msgBox(this);
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setWindowTitle("Sandboxie-Plus");
	msgBox.setText(tr("The selected feature set is only available to project supporters. Processes started in a box with this feature set enabled without a supporter certificate will be terminated after 5 minutes.<br />"
		"<a href=\"https://sandboxie-plus.com/go.php?to=sbie-get-cert\">Become a project supporter</a>, and receive a <a href=\"https://sandboxie-plus.com/go.php?to=sbie-cert\">supporter certificate</a>"));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
	/*msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	if (msgBox.exec() == QDialogButtonBox::Yes) {
		OpenUrl(QUrl("https://sandboxie-plus.com/go.php?to=sbie-get-cert"));
	}*/

	return false;
}

void CSandMan::OnQueuedRequest(quint32 ClientPid, quint32 ClientTid, quint32 RequestId, const QVariantMap& Data)
{
	m_pPopUpWindow->AddUserPrompt(RequestId, Data, ClientPid);
}

void CSandMan::OnFileToRecover(const QString& BoxName, const QString& FilePath, const QString& BoxPath, quint32 ProcessId)
{
	CSandBoxPtr pBox = theAPI->GetBoxByName(BoxName);
	if (!pBox.isNull() && pBox.objectCast<CSandBoxPlus>()->IsRecoverySuspended())
		return;

	if (theConf->GetBool("Options/InstantRecovery", true))
	{
		CRecoveryWindow* pWnd = ShowRecovery(pBox, false);

		if (!theConf->GetBool("Options/AlwaysOnTop", false)) {
			SetWindowPos((HWND)pWnd->winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			QTimer::singleShot(100, this, [pWnd]() {
				SetWindowPos((HWND)pWnd->winId(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				});
		}

		pWnd->AddFile(FilePath, BoxPath);
	}
	else
		m_pPopUpWindow->AddFileToRecover(FilePath, BoxPath, pBox, ProcessId);
}

bool CSandMan::OpenRecovery(const CSandBoxPtr& pBox, bool& DeleteShapshots, bool bCloseEmpty)
{
	auto pBoxEx = pBox.objectCast<CSandBoxPlus>();
	if (pBoxEx->m_pRecoveryWnd != NULL) {
		pBoxEx->m_pRecoveryWnd->close();
		// todo: resuse window?
	}

	CRecoveryWindow* pRecoveryWindow = new CRecoveryWindow(pBox, this);
	if (pRecoveryWindow->FindFiles() == 0 && bCloseEmpty) {
		delete pRecoveryWindow;
	}
	else if (pRecoveryWindow->exec() != 1)
		return false;
	DeleteShapshots = pRecoveryWindow->IsDeleteShapshots();
	return true;
}

CRecoveryWindow* CSandMan::ShowRecovery(const CSandBoxPtr& pBox, bool bFind)
{
	auto pBoxEx = pBox.objectCast<CSandBoxPlus>();
	if (pBoxEx->m_pRecoveryWnd == NULL) {
		pBoxEx->m_pRecoveryWnd = new CRecoveryWindow(pBox);
		connect(pBoxEx->m_pRecoveryWnd, &CRecoveryWindow::Closed, [pBoxEx]() {
			pBoxEx->m_pRecoveryWnd = NULL;
		});
		pBoxEx->m_pRecoveryWnd->show();
	}
	else {
		pBoxEx->m_pRecoveryWnd->setWindowState((pBoxEx->m_pRecoveryWnd->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		SetForegroundWindow((HWND)pBoxEx->m_pRecoveryWnd->winId());
	}
	if(bFind)
		pBoxEx->m_pRecoveryWnd->FindFiles();
	return pBoxEx->m_pRecoveryWnd;
}

SB_PROGRESS CSandMan::RecoverFiles(const QList<QPair<QString, QString>>& FileList, int Action)
{
	CSbieProgressPtr pProgress = CSbieProgressPtr(new CSbieProgress());
	QtConcurrent::run(CSandMan::RecoverFilesAsync, pProgress, FileList, Action);
	return SB_PROGRESS(OP_ASYNC, pProgress);
}

void CSandMan::RecoverFilesAsync(const CSbieProgressPtr& pProgress, const QList<QPair<QString, QString>>& FileList, int Action)
{
	SB_STATUS Status = SB_OK;

	int OverwriteOnExist = -1;

	QStringList Unrecovered;
	for (QList<QPair<QString, QString>>::const_iterator I = FileList.begin(); I != FileList.end(); ++I)
	{
		QString BoxPath = I->first;
		QString RecoveryPath = I->second;
		QString FileName = BoxPath.mid(BoxPath.lastIndexOf("\\") + 1);
		QString RecoveryFolder = RecoveryPath.left(RecoveryPath.lastIndexOf("\\") + 1);

		pProgress->ShowMessage(tr("Recovering file %1 to %2").arg(FileName).arg(RecoveryFolder));

		QDir().mkpath(RecoveryFolder);
		if (QFile::exists(RecoveryPath)) 
		{
			int Overwrite = OverwriteOnExist;
			if (Overwrite == -1)
			{
				bool forAll = false;
				int retVal = 0;
				QMetaObject::invokeMethod(theGUI, "ShowQuestion", Qt::BlockingQueuedConnection, // show this question using the GUI thread
					Q_RETURN_ARG(int, retVal),
					Q_ARG(QString, tr("The file %1 already exists, do you want to overwrite it?").arg(RecoveryPath)),
					Q_ARG(QString, tr("Do this for all files!")),
					Q_ARG(bool*, &forAll),
					Q_ARG(int, QDialogButtonBox::Yes | QDialogButtonBox::No),
					Q_ARG(int, QDialogButtonBox::No)
				);

				Overwrite = retVal == QDialogButtonBox::Yes ? 1 : 0;
				if (forAll)
					OverwriteOnExist = Overwrite;
			}
			if (Overwrite == 1)
				QFile::remove(RecoveryPath);
		}

		if (!QFile::rename(BoxPath, RecoveryPath))
			Unrecovered.append(BoxPath);
	}

	if (!Unrecovered.isEmpty())
		Status = SB_ERR(SB_Message, QVariantList () << (tr("Failed to recover some files: \n") + Unrecovered.join("\n")));
	else if(FileList.count() == 1 && Action != 0)
	{
		std::wstring path = FileList.first().second.toStdWString();
		switch (Action)
		{
		case 1: // open
			ShellExecute(NULL, NULL, path.c_str(), NULL, NULL, SW_SHOWNORMAL);
			break;
		case 2: // explore
			ShellExecute(NULL, NULL, L"explorer.exe", (L"/select,\"" + path + L"\"").c_str(), NULL, SW_SHOWNORMAL);
			break;
		}
	}


	pProgress->Finish(Status);
}

int CSandMan::ShowQuestion(const QString& question, const QString& checkBoxText, bool* checkBoxSetting, int buttons, int defaultButton)
{
	return CCheckableMessageBox::question(this, "Sandboxie-Plus", question, checkBoxText, checkBoxSetting, (QDialogButtonBox::StandardButtons)buttons, (QDialogButtonBox::StandardButton)defaultButton, QMessageBox::Question);
}

void CSandMan::OnNotAuthorized(bool bLoginRequired, bool& bRetry)
{
	if (!bLoginRequired)
	{
		QMessageBox::warning(this, "Sandboxie-Plus", tr("Only Administrators can change the config."));
		return;
	}

	static bool LoginOpen = false;
	if (LoginOpen)
		return;
	LoginOpen = true;
	for (;;)
	{
		QString Value = QInputDialog::getText(this, "Sandboxie-Plus", tr("Please enter the configuration password."), QLineEdit::Password);
		if (Value.isEmpty())
			break;
		SB_STATUS Status = theAPI->UnlockConfig(Value);
		if (!Status.IsError()) {
			bRetry = true;
			break;
		}
		QMessageBox::warning(this, "Sandboxie-Plus", tr("Login Failed: %1").arg(FormatError(Status)));
	}
	LoginOpen = false;
}

void CSandMan::OnBoxMenu(const QPoint & point)
{
	QPoint pos = ((QWidget*)m_pTrayBoxes->parent())->mapFromParent(point);
	QTreeWidgetItem* pItem = m_pTrayBoxes->itemAt(pos);
	if (!pItem)
		return;
	m_pTrayBoxes->setCurrentItem(pItem);

	CTrayBoxesItemDelegate::m_Hold = true;
	m_pBoxView->PopUpMenu(pItem->data(0, Qt::UserRole).toString());
	CTrayBoxesItemDelegate::m_Hold = false;

	//m_pBoxMenu->popup(QCursor::pos());	
}

void CSandMan::OnBoxDblClick(QTreeWidgetItem* pItem)
{
	m_pBoxView->ShowOptions(pItem->data(0, Qt::UserRole).toString());
}

void CSandMan::OnNewBox()
{
	m_pBoxView->AddNewBox();
}

void CSandMan::OnNewGroupe()
{
	m_pBoxView->AddNewGroup();
}

void CSandMan::OnEmptyAll()
{
 	if (theConf->GetInt("Options/WarnTerminateAll", -1) == -1)
	{
		bool State = false;
		if(CCheckableMessageBox::question(this, "Sandboxie-Plus", tr("Do you want to terminate all processes in all sandboxes?")
			, tr("Terminate all without asking"), &State, QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes, QMessageBox::Information) != QDialogButtonBox::Yes)
			return;

		if (State)
			theConf->SetValue("Options/WarnTerminateAll", 1);
	}

	theAPI->TerminateAll();
}

void CSandMan::OnDisableForce()
{
	bool Status = m_pDisableForce->isChecked();
	int Seconds = 0;
	if (Status)
	{
		int LastValue = theAPI->GetGlobalSettings()->GetNum("ForceDisableSeconds", 60);

		bool bOK = false;
		Seconds = QInputDialog::getInt(this, "Sandboxie-Plus", tr("Please enter the duration, in seconds, for disabling Forced Programs rules."), LastValue, 0, INT_MAX, 1, &bOK);
		if (!bOK)
			return;
	}
	theAPI->DisableForceProcess(Status, Seconds);
}

void CSandMan::OnDisableForce2()
{
	bool Status = m_pDisableForce2->isChecked();
	theAPI->DisableForceProcess(Status);
}

SB_RESULT(void*) CSandMan::ConnectSbie()
{
	SB_RESULT(void*) Status;
	if (!CSbieUtils::IsRunning(CSbieUtils::eAll)) 
	{
		if (!CSbieUtils::IsInstalled(CSbieUtils::eAll))
		{
			int PortableStart = theConf->GetInt("Options/PortableStart", -1);
			if (PortableStart == -1)
			{
				bool State = false;
				PortableStart = CCheckableMessageBox::question(this, "Sandboxie-Plus", tr("Sandboxie-Plus was started in portable mode and it needs to create necessary services. This will prompt for administrative privileges.")
					, tr("Don't show this message again."), &State, QDialogButtonBox::Ok | QDialogButtonBox::Cancel, QDialogButtonBox::Ok, QMessageBox::Information) == QDialogButtonBox::Ok ? 1 : 0;

				if (State)
					theConf->SetValue("Options/PortableStart", PortableStart);
			}

			if(!PortableStart)
				return SB_OK;
		}

		Status = CSbieUtils::Start(CSbieUtils::eAll);
	}

	if (Status.GetStatus() == OP_ASYNC) {
		m_bConnectPending = true;
		return Status;
	}
	if (Status.IsError())
		return Status;

	return ConnectSbieImpl();
}

SB_STATUS CSandMan::ConnectSbieImpl()
{
	SB_STATUS Status = theAPI->Connect(g_PendingMessage.isEmpty(), theConf->GetBool("Options/UseInteractiveQueue", true));

	if (Status.GetStatus() == 0xC0000038L /*STATUS_DEVICE_ALREADY_ATTACHED*/) {
		OnLogMessage(tr("CAUTION: Another agent (probably SbieCtrl.exe) is already managing this Sandboxie session, please close it first and reconnect to take over."));
		return SB_OK;
	}

	if (!g_PendingMessage.isEmpty()) {
		OnMessage(g_PendingMessage);
		PostQuitMessage(0);
	}

	return Status;
}

SB_STATUS CSandMan::DisconnectSbie()
{
	return theAPI->Disconnect();
}

SB_RESULT(void*) CSandMan::StopSbie(bool andRemove)
{
	SB_RESULT(void*) Status;

	if (theAPI->IsConnected()) {
		Status = theAPI->TerminateAll();
		theAPI->Disconnect();
	}
	if (!Status.IsError()) {
		if(andRemove)
			Status = CSbieUtils::Uninstall(CSbieUtils::eAll); // it stops it first of course
		else
			Status = CSbieUtils::Stop(CSbieUtils::eAll);
		if (Status.GetStatus() == OP_ASYNC)
			m_bStopPending = true;
	}

	return Status;
}

void CSandMan::OnMaintenance()
{
	SB_RESULT(void*) Status;
	if (sender() == m_pConnect)
		Status = ConnectSbie();
	else if (sender() == m_pDisconnect)
		Status = DisconnectSbie();
	else if (sender() == m_pStopAll)
		Status = StopSbie();

	// advanced
	else if (sender() == m_pInstallDrv)
		Status = CSbieUtils::Install(CSbieUtils::eDriver);
	else if (sender() == m_pStartDrv)
		Status = CSbieUtils::Start(CSbieUtils::eDriver);
	else if (sender() == m_pStopDrv)
		Status = CSbieUtils::Stop(CSbieUtils::eDriver);
	else if (sender() == m_pUninstallDrv)
		Status = CSbieUtils::Uninstall(CSbieUtils::eDriver);

	else if (sender() == m_pInstallSvc)
		Status = CSbieUtils::Install(CSbieUtils::eService);
	else if (sender() == m_pStartSvc)
		Status = CSbieUtils::Start(CSbieUtils::eService);
	else if (sender() == m_pStopSvc)
		Status = CSbieUtils::Stop(CSbieUtils::eService);
	else if (sender() == m_pUninstallSvc)
		Status = CSbieUtils::Uninstall(CSbieUtils::eService);

	// uninstall	
	else if (sender() == m_pUninstallAll)
		Status = StopSbie(true);

	HandleMaintenance(Status);
}

void CSandMan::HandleMaintenance(SB_RESULT(void*) Status)
{
	if (Status.GetStatus() == OP_ASYNC) {

		HANDLE hProcess = Status.GetValue();
		QWinEventNotifier* processFinishedNotifier = new QWinEventNotifier(hProcess);
		processFinishedNotifier->setEnabled(true);
		connect(processFinishedNotifier, &QWinEventNotifier::activated, this, [processFinishedNotifier, this, hProcess]() {
			processFinishedNotifier->setEnabled(false);
			processFinishedNotifier->deleteLater();
			
			DWORD dwStatus = 0;
			GetExitCodeProcess(hProcess, & dwStatus);

			if (dwStatus != 0)
			{
				if(m_bStopPending)
					QMessageBox::warning(this, tr("Sandboxie-Plus - Error"), tr("Failed to stop all Sandboxie components"));
				else if(m_bConnectPending)
					QMessageBox::warning(this, tr("Sandboxie-Plus - Error"), tr("Failed to start required Sandboxie components"));

				OnLogMessage(tr("Maintenance operation failed (%1)").arg((quint32)dwStatus));
				CheckResults(QList<SB_STATUS>() << SB_ERR(dwStatus));
			}
			else
			{
				OnLogMessage(tr("Maintenance operation Successful"));
				if (m_bConnectPending) {

					QTimer::singleShot(1000, [this]() {
						SB_STATUS Status = this->ConnectSbieImpl();
						CheckResults(QList<SB_STATUS>() << Status);
					});
				}
			}
			m_pProgressDialog->hide();
			//statusBar()->showMessage(tr("Maintenance operation completed"), 3000);
			m_bConnectPending = false;
			m_bStopPending = false;

			CloseHandle(hProcess);
		});

		//statusBar()->showMessage(tr("Executing maintenance operation, please wait..."));
		m_pProgressDialog->OnStatusMessage(tr("Executing maintenance operation, please wait..."));
		SafeShow(m_pProgressDialog);

		return;
	}

	CheckResults(QList<SB_STATUS>() << Status);
}

void CSandMan::OnViewMode(QAction* pAction)
{
	bool bAdvanced = pAction->data().toBool();
	theConf->SetValue("Options/AdvancedView", bAdvanced);
	SetViewMode(bAdvanced);
}

void CSandMan::OnAlwaysTop()
{
	StoreState();
	bool bAlwaysOnTop = m_pWndTopMost->isChecked();
	theConf->SetValue("Options/AlwaysOnTop", bAlwaysOnTop);
	this->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);
	this->show(); // why is this needed?
	m_pPopUpWindow->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);
	m_pProgressDialog->setWindowFlag(Qt::WindowStaysOnTopHint, bAlwaysOnTop);
	LoadState();
}

void CSandMan::SetViewMode(bool bAdvanced)
{
	if (bAdvanced)
	{
		for (int i = m_iMenuViewPos; i < m_pMenuView->actions().count(); i++)
			m_pMenuView->actions().at(i)->setVisible(true);

		if (m_pMenuHelp->actions().first() != m_pSupport) {
			m_pMenuHelp->insertAction(m_pMenuHelp->actions().first(), m_pSupport);
			menuBar()->removeAction(m_pSupport);
		}

		m_pToolBar->show();
		m_pLogTabs->show();
		if (theConf->GetBool("Options/NoStatusBar", false))
			statusBar()->hide();
		else {
			statusBar()->show();
			//if (theConf->GetBool("Options/NoSizeGrip", false))
			//	statusBar()->setSizeGripEnabled(false);
		}
	}
	else
	{
		for (int i = m_iMenuViewPos; i < m_pMenuView->actions().count(); i++)
			m_pMenuView->actions().at(i)->setVisible(false);

		m_pMenuHelp->removeAction(m_pSupport);
		menuBar()->addAction(m_pSupport);

		m_pToolBar->hide();
		m_pLogTabs->hide();
		statusBar()->hide();
	}
}

void CSandMan::OnCleanUp()
{
	if (sender() == m_pCleanUpMsgLog || sender() == m_pCleanUpButton)
		m_pMessageLog->GetTree()->clear();
	
	if (sender() == m_pCleanUpTrace || sender() == m_pCleanUpButton)
		m_pTraceView->Clear();
	
	if (sender() == m_pCleanUpProcesses || sender() == m_pCleanUpButton)
		theAPI->UpdateProcesses(false, m_pShowAllSessions->isChecked());
}

void CSandMan::OnProcView()
{
	theConf->SetValue("Options/KeepTerminated", m_pKeepTerminated->isChecked());
	theConf->SetValue("Options/ShowAllSessions", m_pShowAllSessions->isChecked());
}

void CSandMan::OnSettings()
{
	static CSettingsWindow* pSettingsWindow = NULL;
	if (pSettingsWindow == NULL)
	{
		pSettingsWindow = new CSettingsWindow();
		connect(pSettingsWindow, SIGNAL(OptionsChanged()), this, SLOT(UpdateSettings()));
		connect(pSettingsWindow, &CSettingsWindow::Closed, [this]() {
			pSettingsWindow = NULL;
		});
		SafeShow(pSettingsWindow);
	}
}

void CSandMan::UpdateSettings()
{
	SetUITheme();

	//m_pBoxView->UpdateRunMenu();

	SetupHotKeys();

	if (theConf->GetInt("Options/SysTrayIcon", 1))
		m_pTrayIcon->show();
	else
		m_pTrayIcon->hide();
}

void CSandMan::OnResetMsgs()
{
	auto Ret = QMessageBox("Sandboxie-Plus", tr("Do you also want to reset hidden message boxes (yes), or only all log messages (no)?"),
		QMessageBox::Question, QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape, this).exec();
	if (Ret == QMessageBox::Cancel)
		return;

	if (Ret == QMessageBox::Yes)
	{
		theConf->SetValue("Options/PortableStop", -1);
		theConf->SetValue("Options/PortableStart", -1);
		theConf->SetValue("Options/PortableRootDir", -1);

		theConf->SetValue("Options/CheckForUpdates", 2);

		theConf->SetValue("Options/NoEditInfo", true);

		theConf->SetValue("Options/BoxedExplorerInfo", true);
		theConf->SetValue("Options/ExplorerInfo", true);

		theConf->SetValue("Options/OpenUrlsSandboxed", 2);

		theConf->SetValue("Options/AutoCleanupTemplates", -1);
		theConf->SetValue("Options/WarnTerminateAll", -1);
		theConf->SetValue("Options/WarnTerminate", -1);
	}

	theAPI->GetUserSettings()->UpdateTextList("SbieCtrl_HideMessage", QStringList(), true);
	m_pPopUpWindow->ReloadHiddenMessages();
}

void CSandMan::OnResetGUI()
{
	hide();

	theConf->DelValue("ErrorWindow/Window_Geometry");
	theConf->DelValue("MainWindow/Window_Geometry");
	theConf->DelValue("MainWindow/Window_State");
	theConf->DelValue("MainWindow/BoxTree_Columns");
	theConf->DelValue("MainWindow/LogList_Columns");
	theConf->DelValue("MainWindow/Log_Splitter");
	theConf->DelValue("MainWindow/Panel_Splitter");
	theConf->DelValue("MainWindow/BoxTree_Columns");
	theConf->DelValue("MainWindow/TraceLog_Columns");
	theConf->DelValue("FileBrowserWindow/Window_Geometry");
	theConf->DelValue("FileBrowserWindow/FileTree_Columns");
	theConf->DelValue("NewBoxWindow/Window_Geometry");
	theConf->DelValue("OptionsWindow/Window_Geometry");
	theConf->DelValue("OptionsWindow/Run_Columns");
	theConf->DelValue("OptionsWindow/AutoRun_Columns");
	theConf->DelValue("OptionsWindow/Groups_Columns");
	theConf->DelValue("OptionsWindow/Forced_Columns");
	theConf->DelValue("OptionsWindow/Stop_Columns");
	theConf->DelValue("OptionsWindow/Start_Columns");
	theConf->DelValue("OptionsWindow/INet_Columns");
	theConf->DelValue("OptionsWindow/NetFw_Columns");
	theConf->DelValue("OptionsWindow/Access_Columns");
	theConf->DelValue("OptionsWindow/Recovery_Columns");
	theConf->DelValue("OptionsWindow/Templates_Columns");
	theConf->DelValue("PopUpWindow/Window_Geometry");
	theConf->DelValue("RecoveryWindow/Window_Geometry");
	theConf->DelValue("RecoveryWindow/TreeView_Columns");
	theConf->DelValue("SelectBoxWindow/Window_Geometry");
	theConf->DelValue("SettingsWindow/Window_Geometry");
	theConf->DelValue("SnapshotsWindow/Window_Geometry");

	LoadState();

	SafeShow(this);
}

void CSandMan::OnEditIni()
{
	if (theConf->GetBool("Options/NoEditInfo", true))
	{
		bool State = false;
		CCheckableMessageBox::question(this, "Sandboxie-Plus", 
			theConf->GetBool("Options/WatchIni", true)
			? tr("The changes will be applied automatically whenever the file gets saved.")
			: tr("The changes will be applied automatically as soon as the editor is closed.")
			, tr("Don't show this message again."), &State, QDialogButtonBox::Ok, QDialogButtonBox::Ok, QMessageBox::Information);

		if (State)
			theConf->SetValue("Options/NoEditInfo", false);
	}

	wstring Editor = theConf->GetString("Options/Editor", "notepad.exe").toStdWString();
	wstring IniPath = theAPI->GetIniPath().toStdWString();

	SHELLEXECUTEINFO si = { 0 };
	si.cbSize = sizeof(SHELLEXECUTEINFO);
	si.fMask = SEE_MASK_NOCLOSEPROCESS;
	si.hwnd = NULL;
	si.lpVerb = L"runas";
	si.lpFile = Editor.c_str();
	si.lpParameters = IniPath.c_str();
	si.lpDirectory = NULL;
	si.nShow = SW_SHOW;
	si.hInstApp = NULL;
	ShellExecuteEx(&si);
	//WaitForSingleObject(si.hProcess, INFINITE);
	//CloseHandle(si.hProcess);

	if (theConf->GetBool("Options/WatchIni", true))
		return; // if the ini is watched don't double reload
	
	QWinEventNotifier* processFinishedNotifier = new QWinEventNotifier(si.hProcess);
	processFinishedNotifier->setEnabled(true);
	connect(processFinishedNotifier, &QWinEventNotifier::activated, this, [processFinishedNotifier, this, si]() {
		processFinishedNotifier->setEnabled(false);
		processFinishedNotifier->deleteLater();
		this->OnReloadIni();
		CloseHandle(si.hProcess);
	});
}

void CSandMan::OnReloadIni()
{
	theAPI->ReloadConfig();
}

void CSandMan::OnIniReloaded()
{
	m_pBoxView->ReloadUserConfig();
	m_pPopUpWindow->ReloadHiddenMessages();
}

void CSandMan::OnSetMonitoring()
{
	theAPI->EnableMonitor(m_pEnableMonitoring->isChecked());

	if(m_pEnableMonitoring->isChecked() && !m_pToolBar->isVisible())
		m_pLogTabs->show();

	//m_pTraceView->setEnabled(m_pEnableMonitoring->isChecked());
}

SB_STATUS CSandMan::AddAsyncOp(const CSbieProgressPtr& pProgress, bool bWait, const QString& InitialMsg)
{
	m_pAsyncProgress.insert(pProgress.data(), pProgress);
	connect(pProgress.data(), SIGNAL(Message(const QString&)), this, SLOT(OnAsyncMessage(const QString&)));
	connect(pProgress.data(), SIGNAL(Progress(int)), this, SLOT(OnAsyncProgress(int)));
	connect(pProgress.data(), SIGNAL(Finished()), this, SLOT(OnAsyncFinished()));

	m_pProgressDialog->OnStatusMessage(InitialMsg);
	if (bWait) {
		m_pProgressModal = true;
		m_pProgressDialog->exec(); // safe exec breaks the closing
		m_pProgressModal = false;
	}
	else
		SafeShow(m_pProgressDialog);

	if (pProgress->IsFinished()) // Note: since the operation runs asynchronously, it may have already finished, so we need to test for that
		OnAsyncFinished(pProgress.data());

	if (pProgress->IsCanceled())
		return CSbieStatus(SB_Canceled);
	return SB_OK;
}

void CSandMan::OnAsyncFinished()
{
	OnAsyncFinished(qobject_cast<CSbieProgress*>(sender()));
}

void CSandMan::OnAsyncFinished(CSbieProgress* pSender)
{
	CSbieProgressPtr pProgress = m_pAsyncProgress.take(pSender);
	if (pProgress.isNull())
		return;
	disconnect(pProgress.data() , SIGNAL(Finished()), this, SLOT(OnAsyncFinished()));

	SB_STATUS Status = pProgress->GetStatus();
	if(Status.IsError())
		CSandMan::CheckResults(QList<SB_STATUS>() << Status);

	if (m_pAsyncProgress.isEmpty()) {
		if(m_pProgressModal)
			m_pProgressDialog->close();
		else
			m_pProgressDialog->hide();
	}
}

void CSandMan::OnAsyncMessage(const QString& Text)
{
	m_pProgressDialog->OnStatusMessage(Text);
}

void CSandMan::OnAsyncProgress(int Progress)
{
	m_pProgressDialog->OnProgressMessage("", Progress);
}

void CSandMan::OnCancelAsync()
{
	foreach(const CSbieProgressPtr& pProgress, m_pAsyncProgress)
		pProgress->Cancel();
}

QString CSandMan::FormatError(const SB_STATUS& Error)
{
	//QString Text = Error.GetText();
	//if (!Text.isEmpty())
	//	return Text;

	QString Message;
	switch (Error.GetMsgCode())
	{
	case SB_Generic:		return tr("Error Status: 0x%1 (%2)").arg((quint32)Error.GetStatus(), 8, 16, QChar('0')).arg(
		(Error.GetArgs().isEmpty() || Error.GetArgs().first().toString().isEmpty()) ? tr("Unknown") : Error.GetArgs().first().toString().trimmed());
	case SB_Message:		Message = "%1"; break;
	case SB_NeedAdmin:		Message = tr("Administrator rights are required for this operation."); break;
	case SB_ExecFail:		Message = tr("Failed to execute: %1"); break;
	case SB_DriverFail:		Message = tr("Failed to connect to the driver"); break;
	case SB_ServiceFail:	Message = tr("Failed to communicate with Sandboxie Service: %1"); break;
	case SB_Incompatible:	Message = tr("An incompatible Sandboxie %1 was found. Compatible versions: %2"); break;
	case SB_PathFail:		Message = tr("Can't find Sandboxie installation path."); break;
	case SB_FailedCopyConf:	Message = tr("Failed to copy configuration from sandbox %1: %2"); break;
	case SB_AlreadyExists:  Message = tr("A sandbox of the name %1 already exists"); break;
	case SB_DeleteFailed:	Message = tr("Failed to delete sandbox %1: %2"); break;
	case SB_NameLenLimit:	Message = tr("The sandbox name can not be longer than 32 characters."); break;
	case SB_BadNameDev:		Message = tr("The sandbox name can not be a device name."); break;
	case SB_BadNameChar:	Message = tr("The sandbox name can contain only letters, digits and underscores which are displayed as spaces."); break;
	case SB_FailedKillAll:	Message = tr("Failed to terminate all processes"); break;
	case SB_DeleteProtect:	Message = tr("Delete protection is enabled for the sandbox"); break;
	case SB_DeleteNotEmpty:	Message = tr("All sandbox processes must be stopped before the box content can be deleted"); break;
	case SB_DeleteError:	Message = tr("Error deleting sandbox folder: %1"); break;
	//case SB_RemNotEmpty:	Message = tr("A sandbox must be emptied before it can be renamed."); break;
	case SB_DelNotEmpty:	Message = tr("A sandbox must be emptied before it can be deleted."); break;
	case SB_FailedMoveDir:	Message = tr("Failed to move directory '%1' to '%2'"); break;
	case SB_SnapIsRunning:	Message = tr("This Snapshot operation can not be performed while processes are still running in the box."); break;
	case SB_SnapMkDirFail:	Message = tr("Failed to create directory for new snapshot"); break;
	case SB_SnapCopyDatFail:Message = tr("Failed to copy box data files"); break;
	case SB_SnapNotFound:	Message = tr("Snapshot not found"); break;
	case SB_SnapMergeFail:	Message = tr("Error merging snapshot directories '%1' with '%2', the snapshot has not been fully merged."); break;
	case SB_SnapRmDirFail:	Message = tr("Failed to remove old snapshot directory '%1'"); break;
	case SB_SnapIsShared:	Message = tr("Can't remove a snapshot that is shared by multiple later snapshots"); break;
	case SB_SnapDelDatFail:	Message = tr("Failed to remove old box data files"); break;
	case SB_NotAuthorized:	Message = tr("You are not authorized to update configuration in section '%1'"); break;
	case SB_ConfigFailed:	Message = tr("Failed to set configuration setting %1 in section %2: %3"); break;
	case SB_SnapIsEmpty:	Message = tr("Can not create snapshot of an empty sandbox"); break;
	case SB_NameExists:		Message = tr("A sandbox with that name already exists"); break;
	case SB_PasswordBad:	Message = tr("The config password must not be longer than 64 characters"); break;
	case SB_Canceled:		Message = tr("The operation was canceled by the user"); break;
	default:				return tr("Unknown Error Status: 0x%1").arg((quint32)Error.GetStatus(), 8, 16, QChar('0'));
	}

	foreach(const QVariant& Arg, Error.GetArgs())
		Message = Message.arg(Arg.toString()); // todo: make quint32 hex and so on

	return Message;
}

void CSandMan::CheckResults(QList<SB_STATUS> Results)
{
	QStringList Errors;
	for (QList<SB_STATUS>::iterator I = Results.begin(); I != Results.end(); ++I) {
		if (I->IsError() && I->GetStatus() != OP_CANCELED)
			Errors.append(FormatError(*I));
	}

	if (Errors.count() == 1)
		QMessageBox::warning(theGUI, tr("Sandboxie-Plus - Error"), Errors.first());
	else if (Errors.count() > 1) {
		CMultiErrorDialog Dialog(tr("Operation failed for %1 item(s).").arg(Errors.size()), Errors, theGUI);
		Dialog.exec();
	}
}

void CSandMan::OnShowHide()
{
	if (isVisible()) {
		StoreState();
		hide();
	} else
		show();
}

void CSandMan::OnSysTray(QSystemTrayIcon::ActivationReason Reason)
{
	static bool TriggerSet = false;
	static bool NullifyTrigger = false;
	switch(Reason)
	{
		case QSystemTrayIcon::Context:
		{
			QMap<QString, CSandBoxPtr> Boxes = theAPI->GetAllBoxes();

			int iSysTrayFilter = theConf->GetInt("Options/SysTrayFilter", 0);

			bool bAdded = false;
			if (m_pTrayBoxes->topLevelItemCount() == 0)
				bAdded = true; // triger size refresh

			QMap<QString, QTreeWidgetItem*> OldBoxes;
			for(int i = 0; i < m_pTrayBoxes->topLevelItemCount(); ++i) 
			{
				QTreeWidgetItem* pItem = m_pTrayBoxes->topLevelItem(i);
				QString Name = pItem->data(0, Qt::UserRole).toString();
				OldBoxes.insert(Name,pItem);
			}
			
			foreach(const CSandBoxPtr & pBox, Boxes) 
			{
				if (!pBox->IsEnabled())
					continue;

				CSandBoxPlus* pBoxEx = qobject_cast<CSandBoxPlus*>(pBox.data());

				if (iSysTrayFilter == 2) { // pinned only
					if (!pBox->GetBool("PinToTray", false))
						continue;
				}
				else if (iSysTrayFilter == 1) { // active + pinned
					if (pBoxEx->GetActiveProcessCount() == 0 && !pBox->GetBool("PinToTray", false))
						continue;
				}

				QTreeWidgetItem* pItem = OldBoxes.take(pBox->GetName());
				if(!pItem)
				{
					pItem = new QTreeWidgetItem();
					pItem->setData(0, Qt::UserRole, pBox->GetName());
					pItem->setText(0, "  " + pBox->GetName().replace("_", " "));
					m_pTrayBoxes->addTopLevelItem(pItem);

					bAdded = true;
				}

				pItem->setData(0, Qt::DecorationRole, theGUI->GetBoxIcon(pBoxEx->GetType(), pBox->GetActiveProcessCount() != 0));
			}

			foreach(QTreeWidgetItem* pItem, OldBoxes)
				delete pItem;

			if (!OldBoxes.isEmpty() || bAdded) 
			{
				auto palette = m_pTrayBoxes->palette();
				palette.setColor(QPalette::Base, m_pTrayMenu->palette().color(QPalette::Window));
				m_pTrayBoxes->setPalette(palette);
				m_pTrayBoxes->setFrameShape(QFrame::NoFrame);

				//const int FrameWidth = m_pTrayBoxes->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
				int Height = 0; //m_pTrayBoxes->header()->height() + (2 * FrameWidth);

				for (QTreeWidgetItemIterator AllIterator(m_pTrayBoxes, QTreeWidgetItemIterator::All); *AllIterator; ++AllIterator)
					Height += m_pTrayBoxes->visualItemRect(*AllIterator).height();

				QRect scrRect = this->screen()->availableGeometry();
				int MaxHeight = scrRect.height() / 2;
				if (Height > MaxHeight) {
					Height = MaxHeight;
					if (Height < 64)
						Height = 64;
				}

				m_pTrayBoxes->setFixedHeight(Height);

				m_pTrayMenu->removeAction(m_pTrayList);
				m_pTrayMenu->insertAction(m_pTraySeparator, m_pTrayList);

				m_pTrayBoxes->setFocus();
			}

			m_pTrayMenu->popup(QCursor::pos());	
			break;
		}
		case QSystemTrayIcon::DoubleClick:
			if (isVisible())
			{
				if(TriggerSet)
					NullifyTrigger = true;
				
				StoreState();
				hide();
				
				if (theAPI->GetGlobalSettings()->GetBool("ForgetPassword", false))
					theAPI->ClearPassword();

				break;
			}
			show();
		case QSystemTrayIcon::Trigger:
			if (isVisible() && !TriggerSet)
			{
				TriggerSet = true;
				QTimer::singleShot(100, [this]() { 
					TriggerSet = false;
					if (NullifyTrigger) {
						NullifyTrigger = false;
						return;
					}
					this->setWindowState((this->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
					SetForegroundWindow(MainWndHandle);
				} );
			}
			m_pPopUpWindow->Poke();
			break;
	}
}

void CSandMan::OpenUrl(const QUrl& url)
{
	if (url.scheme() == "sbie") {
		QString path = url.path();
		if (path == "/cert")
			return UpdateCert();
		return OpenUrl("https://sandboxie-plus.com/sandboxie" + path);
	}

	int iSandboxed = theConf->GetInt("Options/OpenUrlsSandboxed", 2);

	if (iSandboxed == 2)
	{
		bool bCheck = false;
		QString Message = tr("Do you want to open %1 in a sandboxed (yes) or unsandboxed (no) Web browser?").arg(url.toString());
		QDialogButtonBox::StandardButton Ret = CCheckableMessageBox::question(this, "Sandboxie-Plus", Message , tr("Remember choice for later."), 
			&bCheck, QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel, QDialogButtonBox::Yes, QMessageBox::Question);
		if (Ret == QDialogButtonBox::Cancel) return;
		iSandboxed = Ret == QDialogButtonBox::Yes ? 1 : 0;
		if(bCheck) theConf->SetValue("Options/OpenUrlsSandboxed", iSandboxed);
	}

	if (iSandboxed) RunSandboxed(QStringList(url.toString()), "DefaultBox");
	else ShellExecute(MainWndHandle, NULL, url.toString().toStdWString().c_str(), NULL, NULL, SW_SHOWNORMAL);
}

QString CSandMan::GetVersion()
{
	QString Version = QString::number(VERSION_MJR) + "." + QString::number(VERSION_MIN) //.rightJustified(2, '0')
#if VERSION_REV > 0 || VERSION_MJR == 0
		+ "." + QString::number(VERSION_REV)
#endif
#if VERSION_UPD > 0
		+ QString('a' + VERSION_UPD - 1)
#endif
		;
	return Version;
}

void CSandMan::CheckForUpdates(bool bManual)
{
	if (!m_pUpdateProgress.isNull())
		return;

	m_pUpdateProgress = CSbieProgressPtr(new CSbieProgress());
	AddAsyncOp(m_pUpdateProgress);
	m_pUpdateProgress->ShowMessage(tr("Checking for updates..."));

	if (m_RequestManager == NULL) 
		m_RequestManager = new CNetworkAccessManager(30 * 1000, this);


	QUrlQuery Query;
	Query.addQueryItem("software", "sandboxie-plus");
	//QString Branche = theConf->GetString("Options/ReleaseBranche");
	//if (!Branche.isEmpty())
	//	Query.addQueryItem("branche", Branche);
	//Query.addQueryItem("version", GetVersion());
	Query.addQueryItem("version", QString::number(VERSION_MJR) + "." + QString::number(VERSION_MIN) + "." + QString::number(VERSION_REV) + "." + QString::number(VERSION_UPD));
	Query.addQueryItem("system", "windows-" + QSysInfo::kernelVersion() + "-" + QSysInfo::currentCpuArchitecture());
	Query.addQueryItem("language", QString::number(m_LanguageId));

	QString UpdateKey = GetArguments(g_Certificate, L'\n', L':').value("updatekey");
	if (UpdateKey.isEmpty())
		UpdateKey = theAPI->GetGlobalSettings()->GetText("UpdateKey"); // theConf->GetString("Options/UpdateKey");
	if (!UpdateKey.isEmpty())
		Query.addQueryItem("update_key", UpdateKey);
	Query.addQueryItem("auto", bManual ? "0" : "1");

	QUrl Url("https://sandboxie-plus.com/update.php");
	Url.setQuery(Query);

	QNetworkRequest Request = QNetworkRequest(Url);
	Request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	//Request.setRawHeader("Accept-Encoding", "gzip");
	QNetworkReply* pReply = m_RequestManager->get(Request);
	pReply->setProperty("manual", bManual);
	connect(pReply, SIGNAL(finished()), this, SLOT(OnUpdateCheck()));
}

void CSandMan::OnUpdateCheck()
{
	if (m_pUpdateProgress.isNull())
		return;

	QNetworkReply* pReply = qobject_cast<QNetworkReply*>(sender());
	QByteArray Reply = pReply->readAll();
	bool bManual = pReply->property("manual").toBool();
	pReply->deleteLater();

	m_pUpdateProgress->Finish(SB_OK);
	m_pUpdateProgress.clear();

	QVariantMap Data = QJsonDocument::fromJson(Reply).toVariant().toMap();
	if (Data.isEmpty() || Data["error"].toBool())
	{
		QString Error = Data.isEmpty() ? tr("server not reachable") : Data["errorMsg"].toString();
		OnLogMessage(tr("Failed to check for updates, error: %1").arg(Error), !bManual);
		if (bManual)
			QMessageBox::critical(this, "Sandboxie-Plus", tr("Failed to check for updates, error: %1").arg(Error));
		return;
	}

	bool bNothing = true;

	QStringList IgnoredUpdates = theConf->GetStringList("Options/IgnoredUpdates");

	QString UserMsg = Data["userMsg"].toString();
	if (!UserMsg.isEmpty())
	{
		QString MsgHash = QCryptographicHash::hash(Data["userMsg"].toByteArray(), QCryptographicHash::Md5).toHex().left(8);
		if (!IgnoredUpdates.contains(MsgHash))
		{
			QString FullMessage = UserMsg;
			QString InfoUrl = Data["infoUrl"].toString();
			if (!InfoUrl.isEmpty())
				FullMessage += tr("<p>Do you want to go to the <a href=\"%1\">info page</a>?</p>").arg(InfoUrl);

			CCheckableMessageBox mb(this);
			mb.setWindowTitle("Sandboxie-Plus");
			QIcon ico(QLatin1String(":/SandMan.png"));
			mb.setIconPixmap(ico.pixmap(64, 64));
			//mb.setTextFormat(Qt::RichText);
			mb.setText(UserMsg);
			mb.setCheckBoxText(tr("Don't show this announcement in the future."));
			
			if (!InfoUrl.isEmpty()) {
				mb.setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
				mb.setDefaultButton(QDialogButtonBox::Yes);
			}
			else
				mb.setStandardButtons(QDialogButtonBox::Ok);

			mb.exec();

			if (mb.isChecked())
				theConf->SetValue("Options/IgnoredUpdates", IgnoredUpdates << MsgHash);

			if (mb.clickedStandardButton() == QDialogButtonBox::Yes)
			{
				QDesktopServices::openUrl(InfoUrl);
			}

			bNothing = false;
		}
	}

	QString VersionStr = Data["version"].toString();
	if (!VersionStr.isEmpty()) //&& VersionStr != GetVersion())
	{
		UCHAR myVersion[4] = { VERSION_UPD, VERSION_REV, VERSION_MIN, VERSION_MJR }; // ntohl
		ULONG MyVersion = *(ULONG*)&myVersion;

		ULONG Version = 0;
		QStringList Nums = VersionStr.split(".");
		for (int i = 0, Bits = 24; i < Nums.count() && Bits >= 0; i++, Bits -= 8)
			Version |= (Nums[i].toInt() & 0xFF) << Bits;

		if (Version > MyVersion)
		if (bManual || !IgnoredUpdates.contains(VersionStr)) // when checked manually always show result
		{
			bNothing = false;
			//QDateTime Updated = QDateTime::fromTime_t(Data["updated"].toULongLong());

			QString UpdateMsg = Data["updateMsg"].toString();
			QString UpdateUrl = Data["updateUrl"].toString();

			QString DownloadUrl = Data["downloadUrl"].toString();
			//	'sha256'
			//	'signature'

			QString FullMessage = UpdateMsg.isEmpty() ? tr("<p>There is a new version of Sandboxie-Plus available.<br /><font color='red'>New version:</font> <b>%1</b></p>").arg(VersionStr) : UpdateMsg;
			if (!DownloadUrl.isEmpty())
				FullMessage += tr("<p>Do you want to download the latest version?</p>");
			else if (!UpdateUrl.isEmpty())
				FullMessage += tr("<p>Do you want to go to the <a href=\"%1\">download page</a>?</p>").arg(UpdateUrl);

			CCheckableMessageBox mb(this);
			mb.setWindowTitle("Sandboxie-Plus");
			QIcon ico(QLatin1String(":/SandMan.png"));
			mb.setIconPixmap(ico.pixmap(64, 64));
			//mb.setTextFormat(Qt::RichText);
			mb.setText(FullMessage);
			mb.setCheckBoxText(tr("Don't show this message anymore."));
			mb.setCheckBoxVisible(!bManual);

			if (!UpdateUrl.isEmpty() || !DownloadUrl.isEmpty()) {
				mb.setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
				mb.setDefaultButton(QDialogButtonBox::Yes);
			}
			else
				mb.setStandardButtons(QDialogButtonBox::Ok);

			mb.exec();

			if (mb.isChecked())
				theConf->SetValue("Options/IgnoredUpdates", IgnoredUpdates << VersionStr);

			if (mb.clickedStandardButton() == QDialogButtonBox::Yes)
			{
				if (!DownloadUrl.isEmpty())
				{
					QNetworkRequest Request = QNetworkRequest(DownloadUrl);
					Request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
					//Request.setRawHeader("Accept-Encoding", "gzip");
					QNetworkReply* pReply = m_RequestManager->get(Request);
					connect(pReply, SIGNAL(finished()), this, SLOT(OnUpdateDownload()));
					connect(pReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(OnUpdateProgress(qint64, qint64)));

					m_pUpdateProgress = CSbieProgressPtr(new CSbieProgress());
					AddAsyncOp(m_pUpdateProgress);
					m_pUpdateProgress->ShowMessage(tr("Downloading new version..."));
				}
				else
					QDesktopServices::openUrl(UpdateUrl);
			}
		}
	}

	if (bNothing) 
	{
		theConf->SetValue("Options/NextCheckForUpdates", QDateTime::currentDateTime().addDays(7).toTime_t());

		if (bManual)
			QMessageBox::information(this, "Sandboxie-Plus", tr("No new updates found, your Sandboxie-Plus is up-to-date."));
	}
}

void CSandMan::OnUpdateProgress(qint64 bytes, qint64 bytesTotal)
{
	if (bytesTotal != 0 && !m_pUpdateProgress.isNull())
		m_pUpdateProgress->Progress(100 * bytes / bytesTotal);
}

void CSandMan::OnUpdateDownload()
{
	if (m_pUpdateProgress.isNull())
		return;

	QString TempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	if (TempDir.right(1) != "/")
		TempDir += "/";

	m_pUpdateProgress->Progress(-1);

	QNetworkReply* pReply = qobject_cast<QNetworkReply*>(sender());
	quint64 Size = pReply->bytesAvailable();
	QString Name = pReply->request().url().fileName();
	if (Name.isEmpty() || Name.right(4).compare(".exe", Qt::CaseInsensitive) != 0)
		Name = "Sandboxie-Plus-Install.exe";

	QString FilePath = TempDir + Name;

	QFile File(FilePath);
	if (File.open(QFile::WriteOnly)) {
		while (pReply->bytesAvailable() > 0)
			File.write(pReply->read(4096));
		File.close();
	}

	pReply->deleteLater();

	m_pUpdateProgress->Finish(SB_OK);
	m_pUpdateProgress.clear();

	if (File.size() != Size) {
		QMessageBox::critical(this, "Sandboxie-Plus", tr("Failed to download update from: %1").arg(pReply->request().url().toString()));
		return;
	}

	QString Message = tr("<p>New Sandboxie-Plus has been downloaded to the following location:</p><p><a href=\"%2\">%1</a></p><p>Do you want to begin the installation? If any programs are running sandboxed, they will be terminated.</p>")
		.arg(FilePath).arg("File:///" + TempDir);
	if (QMessageBox("Sandboxie-Plus", Message, QMessageBox::Information, QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape, QMessageBox::NoButton, this).exec() == QMessageBox::Yes)
		QProcess::startDetached(FilePath);
}

void CSandMan::OnHelp()
{
	if (sender() == m_pSupport)
		QDesktopServices::openUrl(QUrl("https://sandboxie-plus.com/go.php?to=donate"));
	else if (sender() == m_pForum)
		QDesktopServices::openUrl(QUrl("https://sandboxie-plus.com/go.php?to=sbie-forum"));
	else if (sender() == m_pManual)
		QDesktopServices::openUrl(QUrl("https://sandboxie-plus.com/go.php?to=sbie-docs"));
	else
		QDesktopServices::openUrl(QUrl("https://sandboxie-plus.com/go.php?to=patreon"));
}

void CSandMan::OnAbout()
{
	if (sender() == m_pAbout)
	{
		QString AboutCaption = tr(
			"<h3>About Sandboxie-Plus</h3>"
			"<p>Version %1</p>"
			"<p>Copyright (c) 2020-2022 by DavidXanatos</p>"
		).arg(GetVersion());

		QString CertInfo;
		if (!g_Certificate.isEmpty()) {
			CertInfo = tr("This copy of Sandboxie+ is certified for: %1").arg(GetArguments(g_Certificate, L'\n', L':').value("name"));
		} else {
			CertInfo = tr("Sandboxie+ is free for personal and non-commercial use.");
		}

		QString AboutText = tr(
			"Sandboxie-Plus is an open source continuation of Sandboxie.<br />"
			"Visit <a href=\"https://sandboxie-plus.com\">sandboxie-plus.com</a> for more information.<br />"
			"<br />"
			"%3<br />"
			"<br />"
			"Driver version: %1<br />"
			"Features: %2<br />"
			"<br />"
			"Icons from <a href=\"https://icons8.com\">icons8.com</a>"
		).arg(theAPI->GetVersion()).arg(theAPI->GetFeatureStr()).arg(CertInfo);

		QMessageBox *msgBox = new QMessageBox(this);
		msgBox->setAttribute(Qt::WA_DeleteOnClose);
		msgBox->setWindowTitle(tr("About Sandboxie-Plus"));
		msgBox->setText(AboutCaption);
		msgBox->setInformativeText(AboutText);

		QIcon ico(QLatin1String(":/SandMan.png"));
		msgBox->setIconPixmap(ico.pixmap(128, 128));

		SafeExec(msgBox);
	}
	else if (sender() == m_pAboutQt)
		QMessageBox::aboutQt(this);
}

void CSandMan::UpdateCertState()
{
	g_CertInfo.State = theAPI->GetCertState();

	g_CertInfo.about_to_expire = g_CertInfo.expirers_in_sec && g_CertInfo.expirers_in_sec < (60*60*24*30);
	if (g_CertInfo.outdated)
		OnLogMessage(tr("The supporter certificate is not valid for this build, please get an updated certificate"));
			// outdated always implicates it is no longer valid
	else if (g_CertInfo.expired) // may be still valid for the current and older builds
		OnLogMessage(tr("The supporter certificate has expired%1, please get an updated certificate")
			.arg(g_CertInfo.valid ? tr(", but it remains valid for the current build") : ""));
	else if(g_CertInfo.about_to_expire)
		OnLogMessage(tr("The supporter certificate will expire in %1 days, please get an updated certificate").arg(g_CertInfo.expirers_in_sec / (60*60*24)));

	emit CertUpdated();
}

void CSandMan::UpdateCert()
{
	QString UpdateKey; // for now only patreons can update the cert automatically
	if(GetArguments(g_Certificate, L'\n', L':').value("type").indexOf("PATREON") == 0)
		UpdateKey = GetArguments(g_Certificate, L'\n', L':').value("updatekey");
	if (UpdateKey.isEmpty()) {
		OpenUrl("https://sandboxie-plus.com/go.php?to=sbie-get-cert");
		return;
	}

	if (!m_pUpdateProgress.isNull())
		return;

	m_pUpdateProgress = CSbieProgressPtr(new CSbieProgress());
	AddAsyncOp(m_pUpdateProgress);
	m_pUpdateProgress->ShowMessage(tr("Checking for certificate..."));

	if (m_RequestManager == NULL) 
		m_RequestManager = new CNetworkAccessManager(30 * 1000, this);


	QUrlQuery Query;
	Query.addQueryItem("UpdateKey", UpdateKey);

	QUrl Url("https://sandboxie-plus.com/get_cert.php");
	Url.setQuery(Query);

	QNetworkRequest Request = QNetworkRequest(Url);
	Request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	//Request.setRawHeader("Accept-Encoding", "gzip");
	QNetworkReply* pReply = m_RequestManager->get(Request);
	connect(pReply, SIGNAL(finished()), this, SLOT(OnCertCheck()));
}

void CSandMan::OnCertCheck()
{
	if (m_pUpdateProgress.isNull())
		return;

	QNetworkReply* pReply = qobject_cast<QNetworkReply*>(sender());
	QByteArray Reply = pReply->readAll();
	int Code = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	pReply->deleteLater();

	m_pUpdateProgress->Finish(SB_OK);
	m_pUpdateProgress.clear();

	if (Code > 299 || Code < 200) {
		QMessageBox::critical(this, "Sandboxie-Plus", tr("No certificate found on server!"));
		return;
	}

	if (Reply.replace("\r\n","\n").compare(g_Certificate.replace("\r\n","\n"), Qt::CaseInsensitive) == 0){
		QMessageBox::information(this, "Sandboxie-Plus", tr("There is no updated certificate available."));
		return;
	}

	QString CertPath = theAPI->GetSbiePath() + "\\Certificate.dat";
	QString TempPath = QDir::tempPath() + "/Sbie+Certificate.dat";
	QFile CertFile(TempPath);
	if (CertFile.open(QFile::WriteOnly)) {
		CertFile.write(Reply);
		CertFile.close();
	}

	WindowsMoveFile(TempPath.replace("/", "\\"), CertPath.replace("/", "\\"));

	if (!theAPI->ReloadCert().IsError()) {
		CSettingsWindow::LoadCertificate();
		UpdateCertState();
	}
	else { // this should not happen
		g_Certificate.clear();
		g_CertInfo.State = 0;
	}
}

void CSandMan::SetUITheme()
{
	m_ThemeUpdatePending = false;

	bool bDark;
	int iDark = theConf->GetInt("Options/UseDarkTheme", 2);
	if (iDark == 2) {
		QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
		bDark = (settings.value("AppsUseLightTheme") == 0);
	} else
		bDark = (iDark == 1);

	if (bDark)
	{
		QApplication::setStyle(QStyleFactory::create("Fusion"));
		QPalette palette;
		palette.setColor(QPalette::Window, QColor(53, 53, 53));
		palette.setColor(QPalette::WindowText, Qt::white);
		palette.setColor(QPalette::Base, QColor(25, 25, 25));
		palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
		palette.setColor(QPalette::ToolTipBase, Qt::white);
		palette.setColor(QPalette::ToolTipText, Qt::white);
		palette.setColor(QPalette::Text, Qt::white);
		palette.setColor(QPalette::Button, QColor(53, 53, 53));
		palette.setColor(QPalette::ButtonText, Qt::white);
		palette.setColor(QPalette::BrightText, Qt::red);
		palette.setColor(QPalette::Link, QColor(218, 130, 42));
		palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
		palette.setColor(QPalette::HighlightedText, Qt::black);
		palette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGray);
		palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
		palette.setColor(QPalette::Disabled, QPalette::Light, Qt::black);
		palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
		QApplication::setPalette(palette);
	}
	else
	{
		QApplication::setStyle(QStyleFactory::create(m_DefaultStyle));
		QApplication::setPalette(m_DefaultPalett);
	}

	m_DarkTheme = bDark;
	CTreeItemModel::SetDarkMode(bDark);
	CListItemModel::SetDarkMode(bDark);
	CPopUpWindow::SetDarkMode(bDark);
	CPanelView::SetDarkMode(bDark);
	CFinder::SetDarkMode(bDark);
}

void CSandMan::UpdateTheme()
{
	if (!m_ThemeUpdatePending)
	{
		m_ThemeUpdatePending = true;
		QTimer::singleShot(500, this, SLOT(SetUITheme()));
	}
}

void CSandMan::LoadLanguage()
{
	m_LanguageId = 0;

	QString Lang = theConf->GetString("Options/UiLanguage");
	if(Lang.isEmpty())
		Lang = QLocale::system().name();

	if (Lang.compare("native", Qt::CaseInsensitive) != 0) {
		if (!Lang.isEmpty())
			m_LanguageId = LocaleNameToLCID(Lang.toStdWString().c_str(), 0);

		LoadLanguage(Lang, "sandman", 0);
		LoadLanguage(Lang, "qt", 1);
	}

	if (!m_LanguageId) 
		m_LanguageId = 1033; // default to English
}

void CSandMan::LoadLanguage(const QString& Lang, const QString& Module, int Index)
{
	qApp->removeTranslator(&m_Translator[Index]);

	if (!Lang.isEmpty())
	{
		QString LangAux = Lang; // Short version as fallback
		LangAux.truncate(LangAux.lastIndexOf('_'));

		QString LangDir = QApplication::applicationDirPath() + "/translations/";

		QString LangPath = LangDir + Module + "_";
		bool bAux = false;
		if (QFile::exists(LangPath + Lang + ".qm") || (bAux = QFile::exists(LangPath + LangAux + ".qm")))
		{
			if(m_Translator[Index].load(LangPath + (bAux ? LangAux : Lang) + ".qm", LangDir))
				qApp->installTranslator(&m_Translator[Index]);
		}
	}
}


// Make sure that QPlatformTheme strings won't be marked as vanished in all .ts files, even after running lupdate

static const char* platform_strings[] = {
QT_TRANSLATE_NOOP("QPlatformTheme", "OK"),
QT_TRANSLATE_NOOP("QPlatformTheme", "Apply"),
QT_TRANSLATE_NOOP("QPlatformTheme", "Cancel"),
QT_TRANSLATE_NOOP("QPlatformTheme", "&Yes"),
QT_TRANSLATE_NOOP("QPlatformTheme", "&No"),
};

// Make sure that CSandBox strings won't be marked as vanished in all .ts files, even after running lupdate

static const char* CSandBox_strings[] = {
QT_TRANSLATE_NOOP("CSandBox", "Waiting for folder: %1"),
QT_TRANSLATE_NOOP("CSandBox", "Deleting folder: %1"),
QT_TRANSLATE_NOOP("CSandBox", "Merging folders: %1 &gt;&gt; %2"),
QT_TRANSLATE_NOOP("CSandBox", "Finishing Snapshot Merge..."),
};

//////////////////////////////////////////////////////////////////////////////////////////
// WinSpy based window finder
//

#include <windows.h>
#include "Helpers/FindTool.h"


typedef enum DEVICE_SCALE_FACTOR {
    DEVICE_SCALE_FACTOR_INVALID	= 0,
    SCALE_100_PERCENT	= 100,
    SCALE_120_PERCENT	= 120,
    SCALE_125_PERCENT	= 125,
    SCALE_140_PERCENT	= 140,
    SCALE_150_PERCENT	= 150,
    SCALE_160_PERCENT	= 160,
    SCALE_175_PERCENT	= 175,
    SCALE_180_PERCENT	= 180,
    SCALE_200_PERCENT	= 200,
    SCALE_225_PERCENT	= 225,
    SCALE_250_PERCENT	= 250,
    SCALE_300_PERCENT	= 300,
    SCALE_350_PERCENT	= 350,
    SCALE_400_PERCENT	= 400,
    SCALE_450_PERCENT	= 450,
    SCALE_500_PERCENT	= 500
} 	DEVICE_SCALE_FACTOR;

typedef HRESULT (CALLBACK *P_GetScaleFactorForMonitor)(HMONITOR, DEVICE_SCALE_FACTOR*);

UINT GetMonitorScaling(HWND hwnd)
{
    static HINSTANCE shcore = LoadLibrary(L"Shcore.dll");
    if (shcore != nullptr)
    {
        if (auto getScaleFactorForMonitor =
                P_GetScaleFactorForMonitor(GetProcAddress(shcore, "GetScaleFactorForMonitor")))
        {
			HMONITOR monitor =
                MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

            DEVICE_SCALE_FACTOR Scale;

            getScaleFactorForMonitor(monitor, &Scale);

            return Scale;
        }
    }
    return 100;
}


#define IDD_FINDER_TOOL                 111
#define ID_FINDER_TARGET                112
#define ID_FINDER_EXPLAIN               113
#define ID_FINDER_RESULT                114

struct SFinderWndData {
	int Scale;
	HFONT hFont;
};

#define DS(x) ((x) * WndData.Scale / 100)

UINT CALLBACK FindProc(HWND hwndTool, UINT uCode, HWND hwnd)
{
	ULONG pid;
	if (uCode == WFN_END)
		GetWindowThreadProcessId(hwnd, &pid);
	else
		pid = 0;

	hwndTool = GetParent(hwndTool);

	SFinderWndData &WndData = *(SFinderWndData*)GetWindowLongPtr(hwndTool, 0);

	if (pid && pid != GetCurrentProcessId())
	{
		RECT rc;
		GetWindowRect(hwndTool, &rc);
		if (rc.bottom - rc.top <= DS(150)) 
			SetWindowPos(hwndTool, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top + DS(70), SWP_SHOWWINDOW | SWP_NOMOVE);

		CBoxedProcessPtr pProcess = theAPI->GetProcessById(pid);
		if (!pProcess.isNull()) 
		{
			wstring result = CSandMan::tr("The selected window is running as part of program %1 in sandbox %2").arg(pProcess->GetProcessName()).arg(pProcess->GetBoxName()).toStdWString();

			SetWindowText(GetDlgItem(hwndTool, ID_FINDER_RESULT), result.c_str());
			//::ShowWindow(GetDlgItem(hwndTool, ID_FINDER_YES_BOXED), SW_SHOW);
		}
		else
		{
			wstring result = CSandMan::tr("The selected window is not running as part of any sandboxed program.").toStdWString();

			SetWindowText(GetDlgItem(hwndTool, ID_FINDER_RESULT), result.c_str());
			//::ShowWindow(GetDlgItem(hwndTool, ID_FINDER_NOT_BOXED), SW_SHOW);
		}
		::ShowWindow(GetDlgItem(hwndTool, ID_FINDER_RESULT), SW_SHOW);
	}
	else
	{
		RECT rc;
		GetWindowRect(hwndTool, &rc);
		if (rc.bottom - rc.top > DS(150))
			SetWindowPos(hwndTool, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top - DS(70), SWP_SHOWWINDOW | SWP_NOMOVE);

		//::ShowWindow(GetDlgItem(hwndTool, ID_FINDER_YES_BOXED), SW_HIDE);
		//::ShowWindow(GetDlgItem(hwndTool, ID_FINDER_NOT_BOXED), SW_HIDE);
		::ShowWindow(GetDlgItem(hwndTool, ID_FINDER_RESULT), SW_HIDE);
	}

	return 0;
}

// hwnd:    All window processes are passed the handle of the window
//         that they belong to in hwnd.
// msg:     Current message (e.g., WM_*) from the OS.
// wParam:  First message parameter, note that these are more or less
//          integers, but they are really just "data chunks" that
//          you are expected to memcpy as raw data to float, etc.
// lParam:  Second message parameter, same deal as above.
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
			SFinderWndData &WndData = *(SFinderWndData*)createStruct->lpCreateParams;
			SetWindowLongPtr(hwnd, 0, (LONG_PTR)&WndData);

			wstring info = CSandMan::tr("Drag the Finder Tool over a window to select it, then release the mouse to check if the window is sandboxed.").toStdWString();

			CreateWindow(L"Static", L"", SS_BITMAP | SS_NOTIFY | WS_VISIBLE | WS_CHILD, DS(10), DS(10), DS(32), DS(32), hwnd, (HMENU)ID_FINDER_TARGET, NULL, NULL);
			CreateWindow(L"Static", info.c_str(), WS_VISIBLE | WS_CHILD, DS(60), DS(10), DS(180), DS(85), hwnd, (HMENU)ID_FINDER_EXPLAIN, NULL, NULL);
			CreateWindow(L"Static", L"", WS_CHILD, DS(60), DS(100), DS(180), DS(50), hwnd, (HMENU)ID_FINDER_RESULT, NULL, NULL);

			WndData.hFont = CreateFont(DS(13), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
			if (WndData.hFont) {
				SendMessage(GetDlgItem(hwnd, ID_FINDER_EXPLAIN), WM_SETFONT, (WPARAM)WndData.hFont, TRUE);
				SendMessage(GetDlgItem(hwnd, ID_FINDER_RESULT), WM_SETFONT, (WPARAM)WndData.hFont, TRUE);
			}

			MakeFinderTool(GetDlgItem(hwnd, ID_FINDER_TARGET), FindProc);

			break;
		}

		case WM_CLOSE:
			SFinderWndData &WndData = *(SFinderWndData*)GetWindowLongPtr(hwnd, 0);

			if (WndData.hFont) DeleteObject(WndData.hFont);

			//DestroyWindow(hwnd);
			PostQuitMessage(0);
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

DWORD WINAPI FinderThreadFunc(LPVOID lpParam)
{
	MSG  msg;
	WNDCLASS mainWindowClass = { 0 };

	HINSTANCE hInstance = NULL;

	// You can set the main window name to anything, but
	// typically you should prefix custom window classes
	// with something that makes it unique.
	mainWindowClass.lpszClassName = TEXT("SBp.WndFinder");

	mainWindowClass.hInstance = hInstance;
	mainWindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	mainWindowClass.lpfnWndProc = WndProc;
	mainWindowClass.hCursor = LoadCursor(0, IDC_ARROW);

	mainWindowClass.cbWndExtra = sizeof(void*); // SFinderWndData

	RegisterClass(&mainWindowClass);

	// Notes:
	// - The classname identifies the TYPE of the window. Not a C type.
	//   This is a (TCHAR*) ID that Windows uses internally.
	// - The window name is really just the window text, this is
	//   commonly used for captions, including the title
	//   bar of the window itself.
	// - parentHandle is considered the "owner" of this
	//   window. MessageBoxes can use HWND_MESSAGE to
	//   free them of any window.
	// - menuHandle: hMenu specifies the child-window identifier,
	//               an integer value used by a dialog box
	//               control to notify its parent about events.
	//               The application determines the child-window
	//               identifier; it must be unique for all
	//               child windows with the same parent window.

	SFinderWndData WndData;
	WndData.Scale = GetMonitorScaling(MainWndHandle);

	HWND hwnd = CreateWindow(mainWindowClass.lpszClassName, CSandMan::tr("Sandboxie-Plus - Window Finder").toStdWString().c_str()
		, WS_SYSMENU | WS_CAPTION | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, DS(275), DS(135), NULL, 0, hInstance, &WndData);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void CSandMan::OnWndFinder()
{
	m_pWndFinder->setEnabled(false);

	HANDLE hThread = CreateThread(NULL, 0, FinderThreadFunc, NULL, 0, NULL);

	QWinEventNotifier* finishedNotifier = new QWinEventNotifier(hThread);
	finishedNotifier->setEnabled(true);
	connect(finishedNotifier, &QWinEventNotifier::activated, this, [finishedNotifier, this, hThread]() {
		CloseHandle(hThread);

		m_pWndFinder->setEnabled(true);

		finishedNotifier->setEnabled(false);
		finishedNotifier->deleteLater();
	});
}
