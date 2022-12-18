#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "commander.h"
#include "datatype.h"
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <QString>
#include <QLabel>
#include <QProgressBar>
#include <QModelIndex>
#include <QCloseEvent>
#include <QButtonGroup>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void chooseDeviceID(QModelIndex index);
    void handleDeviceList(QStringList & devices,QStringList & names);
    void handlePackageInfoList(QVector<QStringList> &infolist);
    void handlePullAPKLevel(int value,int total);
    void handleParseLabelLevel(int value,int total);
    void handleCMDResult(QStringList result,int flags);
    void handleButtonListPackages();
    void handleGroupBtn(int id,bool flag);
    void handelRowSelected(QModelIndex index);
    void handleButtonUnfreeze();
    void handleButtonFreeze();
    void handleButtonUninstall();
    void handleButtonEnable();
signals:
    void doGetPackages(QString,QStringList);
    void doCleanCache();
    void doOperateCMD(QString,int);
protected:
    void closeEvent(QCloseEvent *event);
    void buttonOperate(QString op,QString msg,QString title,int flags);
private:
    bool m_winFlag;
    QString m_current_device_ID;
    QStringList m_device_ID_list,m_log;
    QModelIndex m_selected;
    QProgressBar *m_processBar;
    QLabel * m_procLabel;
    Commander * m_worker;
    Ui::MainWindow *ui;
    QThread m_workerThread;
    QStandardItemModel *m_model;
    QVector<QStringList> m_apkInfo;
    QButtonGroup *m_group,*m_group2;
};
#endif // MAINWINDOW_H
