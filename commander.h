#ifndef COMMANDER_H
#define COMMANDER_H

#include <QObject>
#include <QDir>
#include <QVector>
#include <QProcess>
#include <QString>
#include <QStringList>
#include "datatype.h"
class Commander : public QObject
{
    Q_OBJECT
public:
    explicit Commander(QObject *parent = nullptr);
    ~Commander();

public slots:
    void cleanCache();
    void listDevices();
    void listPackages(QString devid, QStringList flags);
    void handleOperateCMD(QString pkgname,int flags);
    void readStandardError();
    void readStandardOutput();
    void readProcessFinish(int code);
    void readProcessError(QProcess::ProcessError error);
signals:
    void deviceList(QStringList & devices,QStringList & names);
    void doPullAPKLevel(int,int);
    void doParseLabel(int,int);
    void doPackageInfoList(QVector<QStringList> &labels);
    void doSendCMDResult(QStringList,int);
private:
    QStringList runCmd(QString &program,QStringList &args);
    QStringList listDeviceNames(QStringList devids);
    QVector<QStringList> pullAPK2Local(QString& deviceId,QStringList &packages);
    void parseAPKLabel(QVector<QStringList> & apk_info_list,QVector<QStringList> & output_apk_info);
    QStringList getLabel(QStringList &info);
private:
    int m_return_code;
    bool m_std_status;
    QDir *m_dir;
    QString m_stdout;
    QProcess * m_process;
    QString m_program_adb,m_program_aapt;
};

#endif // COMMANDER_H
