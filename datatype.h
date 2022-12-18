#ifndef DATATYPE_H
#define DATATYPE_H
#include <QMetaType>
#include <QModelIndex>
#include <QString>
enum PackageType
{
    SYS_PACK=0,
    TRD_PACK=1,
    PACK_INFO=2,
    PACK_LABEL=3
};
enum MsgType
{
    PULL_SYS_APK,
    PULL_TRD_APK,
    PARSE_LABEL,
    FINISH
};
enum InfoFlags
{
    UNINSTALL=0,
    FREEZE=1,
    UNFREEZE=2
};
Q_DECLARE_METATYPE(MsgType);
#endif // DATATYPE_H
