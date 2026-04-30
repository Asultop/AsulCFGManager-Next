#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <QObject>
#include "ElaProperty.h"
#include "ElaSingleton.h"
#include <QTemporaryDir>
#include <QTranslator>
#include <QSettings>
#include <QJsonArray>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <qobject.h>

#define gSets GlobalSettings::getInstance()
class GlobalSettings : public QObject
{
    Q_OBJECT
    Q_SINGLETON_CREATE_H(GlobalSettings);
    

    Q_PROPERTY_CREATE(QString, CFGPath);
    Q_PROPERTY_CREATE(QString, SteamUserPath);
    Q_PROPERTY_CREATE(QString, SteamConfPath);
    Q_PROPERTY_CREATE(QString, SteamPath);
    Q_PROPERTY_CREATE(QString, WMPVPPath);
    Q_PROPERTY_CREATE(QString, EEEEEPath);
    Q_PROPERTY_CREATE(QString, EEEEELaunchOptionFilePath);
    Q_PROPERTY_CREATE(QString, WmPvpLaunchOptionFilePath);
    Q_PROPERTY_CREATE(int,CharactersPerMinute);
    Q_PROPERTY_CREATE(QTemporaryDir*,GLoc);

    Q_PROPERTY_CREATE(bool,EnableThemeColorSyncWithSystem);
    Q_PROPERTY_CREATE(bool,EnableThemeModeSyncWithSystem);
    Q_PROPERTY_CREATE(QString,EnableDisplayMode);
    Q_PROPERTY_CREATE(QStringList,SupportedLang);

    Q_PROPERTY_CREATE(QSettings*,RegisterSettings);
public:
    QMap<QString,QTranslator*> translators;
public:
    explicit GlobalSettings(QObject *parent = nullptr);
    void init();
    void destroy();
    QString getProgramName();
    QString getProgramVersion();
    QString getProgramDescription();
    QString getProgramAuthor();
    QString getProgramLicense();
    QString getProgramRepository();
    QString getProgramOrganization();
    
    
private:
    QJsonObject jsonObj;
    QString getPath(QString vdfFile);
    QString getAllPath();
signals:
};

#endif // GLOBALSETTINGS_H
