#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileIconProvider>
#include "ui_RecoveryWindow.h"
#include "SbiePlusAPI.h"
class CSimpleTreeModel;

class CRecoveryCounter : public QThread
{
	Q_OBJECT
public:
	CRecoveryCounter(const QString& BoxPath, QWidget* parent = Q_NULLPTR) : QThread(parent) {
		m_BoxPath = BoxPath;
		m_run = true;
		start(QThread::LowPriority);
	}
	~CRecoveryCounter() {
		m_run = false;
		wait(2000);
		terminate();
	}

signals:
	void		Count(quint32 fileCount, quint32 folderCount, quint64 totalSize);

protected:
	void		run();

	QString		m_BoxPath;
	bool		m_run;
};

class CRecoveryWindow : public QDialog
{
	Q_OBJECT

public:
	CRecoveryWindow(const CSandBoxPtr& pBox, QWidget *parent = Q_NULLPTR);
	~CRecoveryWindow();

	bool		IsDeleteShapshots() { return m_DeleteShapshots; }

	virtual void accept() {}
	virtual void reject() { this->close(); }

signals:
	void		Closed();

public slots:
	int			exec();

	int			FindFiles();
	void		AddFile(const QString& FilePath, const QString& BoxPath);

private slots:
	void		OnAddFolder();
	void		OnRecover();
	void		OnTargetChanged();
	void		OnDeleteAll();
	void		OnDeleteEverything();
	void		OnCloseUntil();
	void		OnCount(quint32 fileCount, quint32 folderCount, quint64 totalSize);

protected:
	void		closeEvent(QCloseEvent *e);

	int			FindFiles(const QString& Folder);
	int			FindBoxFiles(const QString& Folder);
	QPair<int, quint64> FindFiles(const QString& BoxedFolder, const QString& RealFolder, const QString& Name, const QString& ParentID = QString());

	void		RecoverFiles(bool bBrowse, QString RecoveryFolder = QString());

	CSandBoxPtr m_pBox;

	QMap<QVariant, QVariantMap> m_FileMap;
	QSet<QString> m_NewFiles;

	QStringList m_RecoveryFolders;

	CRecoveryCounter* m_pCounter;

	int m_LastTargetIndex;
	bool m_bTargetsChanged;
	bool m_bReloadPending;
	bool m_DeleteShapshots;

private:
	Ui::RecoveryWindow ui;
	QAction* m_pRemember;
	
	CSimpleTreeModel* m_pFileModel;
	QSortFilterProxyModel*	m_pSortProxy;

	QFileIconProvider m_IconProvider;
};
