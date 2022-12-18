#include "commander.h"
#include <QCoreApplication>
#include <QDebug>
Commander::Commander(QObject *parent) : QObject(parent)
{
    m_return_code =0;
    m_std_status = true;
    QString path = QCoreApplication::applicationDirPath();
    m_program_adb=path+"/platform-tools/adb.exe";
    m_program_aapt=path+"/aapt-windows/aapt.exe";
    m_process = new QProcess(this);
    m_dir = new QDir(path);
    if(!m_dir->cd("./cache"))
    {
        m_dir->mkdir("cache");
        m_dir->cd("./cache");
    }
    connect(m_process,SIGNAL(readyReadStandardError()),this,SLOT(readStandardError()));
    connect(m_process,SIGNAL(readyReadStandardOutput()),this,SLOT(readStandardOutput()));
    connect(m_process,SIGNAL(finished(int)),this,SLOT(readProcessFinish(int)));
    connect(m_process,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(readProcessError(QProcess::ProcessError)));
    m_process->setWorkingDirectory(m_dir->absoluteFilePath("../"));
}

Commander::~Commander()
{
    delete m_dir;
    m_process->terminate();
}

void Commander::listDevices()
{
    bool flag = false;
    QStringList args,devices;
    args<<"devices";
    QStringList retval = this->runCmd(m_program_adb,args);
    foreach(auto s, retval){
        if(s == "List of devices attached")
        {
            flag = true;
            continue;
        }
        if(flag)
        devices<<s.split("\t").at(0);
    }
    QStringList names = this->listDeviceNames(devices);
    emit deviceList(devices,names);
}

void Commander::listPackages(QString devid,QStringList flags)
{
    QStringList args;
    args<<"-s"<<devid<<"shell"<<"pm"<<"list"<<"packages"<<"-f";
    foreach (auto f, flags) {
        args<<f;
    }
    QStringList packages = this->runCmd(m_program_adb,args);
    QVector<QStringList> apkinfolist = pullAPK2Local(devid,packages);
    QVector<QStringList> output_apk_info;
    parseAPKLabel(apkinfolist,output_apk_info);
    emit doPackageInfoList(output_apk_info);

}

void Commander::handleOperateCMD(QString pkgname,int flags)
{
    QStringList args,result;
    switch (flags) {
        case InfoFlags::UNINSTALL:
        {
            args<<"shell"<<"pm"<<"uninstall"<<"--user"<<"0"<<pkgname;
            break;
        }
        case InfoFlags::FREEZE:
        {
            args<<"shell"<<"pm"<<"disable-user"<<pkgname;
            break;
        }
        case InfoFlags::UNFREEZE:
        {
            args<<"shell"<<"pm"<<"enable"<<pkgname;
            break;
        }
    }
    if(!args.empty())
    {
        result = this->runCmd(m_program_adb,args);
    }
    if(result.isEmpty())
    {
        result<<"failed";
    }
    emit doSendCMDResult(result,flags);
}

QVector<QStringList> Commander::pullAPK2Local(QString& deviceId,QStringList &packages)
{
    int i = 0;
    int total = packages.length();
    QVector<QStringList> package_info;
    foreach(auto s, packages)
    {
        QStringList args,temp;
        QString package_name,package_path,apk_folder,temp_path;
        QStringList package_cell = s.split("=");
        if(package_cell.length()>2)
        {
            package_name = package_cell.last();
            package_cell.removeLast();
            temp_path = package_cell.join("=");
        }else
        {
            package_name = package_cell.last();
            temp_path = package_cell.at(0);
        }
        bool flag=false;
        foreach(auto s, temp_path.split(":"))
        {
            if(s=="package")
            {
                flag = true;
                continue;
            }
            if(flag)
            {
                package_path = s;
            }
        }
        apk_folder = m_dir->absolutePath()+ "/" + package_name + ".apk";
        args<<"-s"<<deviceId<<"pull"<<package_path<<apk_folder;
        this->runCmd(m_program_adb,args);
        temp.append(package_name);
        temp.append(package_path);
        temp.append(apk_folder);
        package_info.append(temp);
        ++i;
        emit doPullAPKLevel(i,total);
    }
    return package_info;
}

void Commander::parseAPKLabel(QVector<QStringList> & apk_info_list,QVector<QStringList> & output_apk_info)
{
    QStringList args;
    int i=0;
    int total = apk_info_list.length();
    foreach (QStringList apk_info, apk_info_list) {
        args<<"d"<<"badging"<<apk_info.last();
        QStringList rt_temp = this->runCmd(m_program_aapt,args);
        QStringList label = getLabel(rt_temp);
        output_apk_info.append(label + apk_info);
        args.clear();
        i += 1;
        emit doParseLabel(i,total);
    }
}

void Commander::cleanCache()
{
    m_dir->setFilter(QDir::Files);
    QStringList names = m_dir->entryList(QDir::Files);
    for(int i=0;i<names.length();++i)
    {
        m_dir->remove(names[i]);
    }
}

QStringList Commander::getLabel(QStringList &info_list)
{
    QStringList labels;
    foreach (auto s, info_list) {
        int i = s.indexOf("application-label:");
        int j = s.indexOf("application-label-zh-CN:");
        if(i>=0 || j>=0){
            labels.append(s.split(":").last().remove("'"));
        }
    }
    int length = labels.length();
    if(0==length)
    {
        labels<<""<<"";
    }else if(1==length)
    {
        labels<<"";
    }
    return labels;
}

QStringList Commander::listDeviceNames(QStringList devids)
{
    QStringList args,retval;
    foreach (auto s, devids) {
        args<<"-s"<<s<<"shell"<<"getprop"<<"net.hostname";
        QStringList names = this->runCmd(m_program_adb,args);
        foreach (auto n, names) {
            retval<<n;
        }
        args.clear();
    }
    return retval;
}

QStringList Commander::runCmd(QString &program,QStringList &args)
{
    QStringList retval;
    m_process->start(program,args);
    m_process->waitForFinished();
    if(m_std_status && m_return_code==0){
        QStringList temp = m_stdout.split("\r\n");
        foreach(auto s,temp)
        {
            if(!s.isEmpty())
            {
                retval<<s;
            }
        }
        m_stdout.clear();
    }
    return retval;
}

void Commander::readStandardError()
{
    m_std_status = false;
}

void Commander::readStandardOutput()
{
    QString sdata(m_process->readAllStandardOutput());
    m_stdout.append(sdata);
    m_std_status = true;
}

void Commander::readProcessFinish(int code)
{
    m_return_code = code;
    m_std_status = true;
}

void Commander::readProcessError(QProcess::ProcessError error)
{
}
