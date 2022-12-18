#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>
#include <QMessageBox>
#include <QMap>
#include <QDateTime>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qRegisterMetaType<MsgType>("MsgType");
    qRegisterMetaType<QStringList>("QStringList&");
    qRegisterMetaType<QVector<QStringList>>("QVector<QStringList>&");

    m_current_device_ID = "";
    m_worker = new Commander();
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));

    m_group = new QButtonGroup(this);
    m_group2 = new QButtonGroup(this);

    m_group->addButton(ui->radioButton_en,0);
    m_group->addButton(ui->radioButton_dis,1);
    connect(m_group,SIGNAL(buttonToggled(int,bool)),this,SLOT(handleGroupBtn(int,bool)));

    m_group2->addButton(ui->radioButton_sys,2);
    m_group2->addButton(ui->radioButton_thd,3);
    connect(m_group2,SIGNAL(buttonToggled(int,bool)),this,SLOT(handleGroupBtn(int,bool)));

    connect(ui->pushButton_listdevice,SIGNAL(clicked()),m_worker,SLOT(listDevices()));
    connect(ui->pushButton_listdevice,SIGNAL(clicked()),this,SLOT(handleButtonEnable()));
    connect(m_worker, SIGNAL(deviceList(QStringList&,QStringList&)), this, SLOT(handleDeviceList(QStringList&,QStringList&)));
    connect(ui->listWidget,SIGNAL(clicked(QModelIndex)),this,SLOT(chooseDeviceID(QModelIndex)));
    connect(ui->pushButton_listpackage,SIGNAL(clicked()),this,SLOT(handleButtonListPackages()));
    connect(this, SIGNAL(doGetPackages(QString,QStringList)), m_worker, SLOT(listPackages(QString,QStringList)));
    connect(m_worker, SIGNAL(doPullAPKLevel(int,int)), this, SLOT(handlePullAPKLevel(int,int)));
    connect(m_worker, SIGNAL(doParseLabel(int,int)), this, SLOT(handleParseLabelLevel(int,int)));
    connect(this, SIGNAL(doCleanCache()), m_worker, SLOT(cleanCache()));
    connect(m_worker, SIGNAL(doPackageInfoList(QVector<QStringList>&)), this, SLOT(handlePackageInfoList(QVector<QStringList>&)));
    connect(ui->treeView,SIGNAL(clicked(QModelIndex)),this,SLOT(handelRowSelected(QModelIndex)));

    connect(ui->pushButton_disable,SIGNAL(clicked()),this,SLOT(handleButtonFreeze()));
    connect(ui->pushButton_enable,SIGNAL(clicked()),this,SLOT(handleButtonUnfreeze()));
    connect(ui->pushButton_uninstall,SIGNAL(clicked()),this,SLOT(handleButtonUninstall()));
    connect(this, SIGNAL(doOperateCMD(QString,int)), m_worker, SLOT(handleOperateCMD(QString,int)));
    connect(m_worker, SIGNAL(doSendCMDResult(QStringList,int)), this, SLOT(handleCMDResult(QStringList,int)));

    ui->treeView->setIndentation(0);
    m_winFlag = true;
    m_processBar = new QProgressBar(this);
    m_procLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(m_procLabel);
    ui->statusbar->addPermanentWidget(m_processBar);
    m_model = new QStandardItemModel(this);
    ui->listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listWidget->setAlternatingRowColors(true);
    m_workerThread.start();
}

MainWindow::~MainWindow()
{
    delete m_worker;
    delete m_processBar;
    delete m_procLabel;
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_model;
    delete ui;
}

void MainWindow::handleButtonEnable()
{
    ui->pushButton_listpackage->setEnabled(false);
}

void MainWindow::handelRowSelected(QModelIndex index)
{
        ui->pushButton_uninstall->setEnabled(true);
        if(ui->radioButton_dis->isChecked())
        {
            ui->pushButton_enable->setEnabled(true);
        }else {
            ui->pushButton_disable->setEnabled(true);
        }
}

void MainWindow::handleGroupBtn(int id,bool flag)
{
    m_model->clear();
    ui->pushButton_disable->setEnabled(false);
    ui->pushButton_enable->setEnabled(false);
    ui->pushButton_uninstall->setEnabled(false);
    ui->pushButton_listpackage->setEnabled(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(!m_winFlag){
        QMessageBox::information(this,"提示","程序正在运行,请稍等","确定","取消",0,1);
        event->ignore();
    }
}

void MainWindow::chooseDeviceID(QModelIndex index)
{
    m_current_device_ID = m_device_ID_list.at(index.row());
    ui->pushButton_listpackage->setEnabled(true);
}

void MainWindow::handleButtonListPackages()
{
    QStringList args;
    QString status = ui->radioButton_dis->isChecked() ? "-d" : "-e";
    QString type = ui->radioButton_thd->isChecked() ? "-3" : "-s";
    args<<type<<status;
    emit doGetPackages(m_current_device_ID,args);
    ui->pushButton_listpackage->setEnabled(false);
    ui->groupBox_3->setDisabled(true);
    ui->groupBox_4->setDisabled(true);
}

void MainWindow::handleButtonUnfreeze()
{
    buttonOperate("解禁","","解禁应用",InfoFlags::UNFREEZE);
}
void MainWindow::handleButtonFreeze()
{
    buttonOperate("禁用","这是系统应用,确定要禁用?","禁用应用",InfoFlags::FREEZE);
}
void MainWindow::handleButtonUninstall()
{
    buttonOperate("卸载","这是系统应用,确定要卸载?","卸载应用",InfoFlags::UNINSTALL);
}

void MainWindow::buttonOperate(QString op,QString msg,QString title,int flags)
{
    int col=-1,w=0;
    QString package_name;
    QModelIndexList indexlist = ui->treeView->selectionModel()->selectedIndexes();
    foreach (QModelIndex index, indexlist) {
        QMap<int,QVariant> data = ui->treeView->model()->itemData(index);
        QString packge_info = data.first().toString();
        col = index.column();
        if(0==col)
        {
            m_selected = index;
            m_log.append(op);
            continue;
        }
        if(3==col)
        {
            package_name = data.first().toString();
        }
        m_log.append(packge_info);
    }
    if(!m_log.isEmpty())
    {
        m_log.pop_back();
    }
    bool checked = ui->radioButton_sys->isChecked();
    if(checked && !msg.isEmpty())
    {
        w = QMessageBox::warning(this,title,msg,"确定","取消",0,1);
    }
    if(w<=0 && !package_name.isEmpty())
    {
        emit doOperateCMD(package_name,flags);
    }
}

void MainWindow::handleCMDResult(QStringList result,int flags)
{
    QString status;
    QDateTime datetime(QDateTime::currentDateTime());
    QString qStr = datetime.toString("yyyy-MM-dd hh:mm::ss ddd");
    switch (flags) {
        case InfoFlags::UNINSTALL:
        {
            status = "已卸载";
             m_log[0] = qStr + "  < 卸载 > ";
            break;
        }
        case InfoFlags::FREEZE:
        {
            status = "已禁用";
            m_log[0] = qStr + "  < 禁用 > ";
            break;
        }
        case InfoFlags::UNFREEZE:
        {
            status = "已解禁";
            m_log[0] = qStr + "  < 解禁 > ";
            break;
        }
    }
    if(result.first()!="failed")
    {
        ui->treeView->model()->setData(m_selected,QString(status));
    }else
    {
        result.first() = "失败";
    }
    QStringList temp;
    temp = m_log+result;
    QString log = temp.join("  ");
    ui->textEdit->append(log);
    m_log.clear();
}

void MainWindow::handleDeviceList(QStringList & devices, QStringList &names)
{
    m_device_ID_list = devices;
    ui->listWidget->clear();
    ui->listWidget->addItems(names);
    ui->pushButton_uninstall->setEnabled(false);
    ui->pushButton_enable->setEnabled(false);
    ui->pushButton_disable->setEnabled(false);
}

void MainWindow::handlePullAPKLevel(int value,int total)
{
    m_procLabel->setText("拉取应用程序包");
    m_processBar->setRange(0,total);
    m_processBar->setValue(value+1);
    m_winFlag = false;
}

void MainWindow::handleParseLabelLevel(int value,int total)
{
    m_procLabel->setText("解析应用程序名");
    m_processBar->setRange(0,total);
    m_processBar->setValue(value+1);
    m_winFlag = false;
    if(value+1 >= total)
    {
        m_procLabel->setText("完成");
        m_processBar->setValue(0);
        emit doCleanCache();
        m_winFlag = true;
        ui->pushButton_listpackage->setEnabled(true);
    }
}

void MainWindow::handlePackageInfoList(QVector<QStringList> &infolist)
{
    int i = 0;
    QString status = ui->radioButton_dis->isChecked() ? "已禁用" : "已安装";
    m_model->clear();
    m_model->setColumnCount(5);
    m_model->setHeaderData(0,Qt::Horizontal,"应用状态");
    m_model->setHeaderData(1,Qt::Horizontal,"应用名称1");
    m_model->setHeaderData(2,Qt::Horizontal,"应用名称2");
    m_model->setHeaderData(3,Qt::Horizontal,"应用包名称");
    m_model->setHeaderData(4,Qt::Horizontal,"应用安装路径");
    foreach (QStringList pkginfo, infolist) {
        QString name1 = pkginfo.at(0);
        QString name2 = pkginfo.at(1);
        QString pkg_name = pkginfo.at(2);
        QString pkg_path = pkginfo.at(3);
        m_model->setItem(i,0,new QStandardItem(status));
        m_model->setItem(i,1,new QStandardItem(name1));
        m_model->setItem(i,2,new QStandardItem(name2));
        m_model->setItem(i,3,new QStandardItem(pkg_name));
        m_model->setItem(i,4,new QStandardItem(pkg_path));
        i +=1;
    }
    ui->treeView->setModel(m_model);
    m_winFlag = true;
    ui->groupBox_3->setDisabled(false);
    ui->groupBox_4->setDisabled(false);
}

