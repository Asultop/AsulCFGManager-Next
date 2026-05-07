#include "GlobalSettings.h"
#include "../SystemKit/AsulApplication.h"
#include <QStyleHints>
#include <ElaTheme.h>
#include "GlobalFunc.h"
#include <ElaPushButton.h>
#include <ElaLineEdit.h>
#include <ElaText.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfiledevice.h>
#include <qstandardpaths.h>


Q_SINGLETON_CREATE_CPP(GlobalSettings);
GlobalSettings::GlobalSettings(QObject *parent)
    : QObject{parent}{}

void GlobalSettings::init()
{
    QFile programJson(":/PropertySetting/Program_Property.json");
    if(programJson.open(QIODevice::ReadOnly)){
        QByteArray data = programJson.readAll();
        programJson.close();
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
        jsonObj = jsonDoc.object();
    }


    this->setCharactersPerMinute(120);
    this->setGLoc(new QTemporaryDir());
    this->setEnableThemeColorSyncWithSystem(false);
    this->setEnableThemeModeSyncWithSystem(false);
    this->setOpenFolderAfterDeploy(true);
    this->setExecuteProgramAfterDeploy(true);
    this->setSupportedLang(QStringList{"zh_CN","en_US"});
    this->setCustomSourceURL("https://github.com/Asultop/ManagerNextPackageSource/raw/refs/heads/main/manifest.json");


    foreach(QString lang,this->getSupportedLang()){
        translators[lang] = new QTranslator(this);
        if(translators[lang]->load(QString(":/translations/AsulKit_%1.qm").arg(lang))){
            //ToDo: Need Do Something But Not Now; 
        }
    }

    this->setRegisterSettings(new QSettings(getProgramOrganization(),getProgramName(),this));

    // 读取上次选择的平台
    QString lastPlatform = this->getRegisterSettings()->value("LastSelectedPlatform", "Steam").toString();
    this->setLastSelectedPlatform(lastPlatform);

    Qt::ColorScheme scheme = qApp->styleHints()->colorScheme();
    if(scheme == Qt::ColorScheme::Dark){
        eTheme->setThemeMode(ElaThemeType::Dark);
    }else eTheme->setThemeMode(ElaThemeType::Light);

    connect(qApp->styleHints(),&QStyleHints::colorSchemeChanged,[=](){
        if(!this->getEnableThemeModeSyncWithSystem()) return;

        Qt::ColorScheme scheme = qApp->styleHints()->colorScheme();
        if(scheme == Qt::ColorScheme::Dark){
            eTheme->setThemeMode(ElaThemeType::Dark);
        }else eTheme->setThemeMode(ElaThemeType::Light);
        GlobalFunc::updateThemeUI();
    });
    connect(aApp,&AsulApplication::themeChanged,[=](){
        GlobalFunc::updateThemeUI();
    });

    this->getAllPath();

    // qDebug() << this->getSteamPath();
    // qDebug() << this->getSteamUserPath();
    // qDebug() << this->getSteamConfPath();
    // qDebug() << this->getCFGPath();
    // qDebug() << this->getWMPVPPath();
    // qDebug() << this->getEEEEEPath();

   }
void GlobalSettings::destroy(){

    delete this->getGLoc();
}

QString GlobalSettings::getProgramName()
{
    return jsonObj["programName"].toString();
}

QString GlobalSettings::getProgramVersion()
{
    return jsonObj["programVersion"].toString();
}

QString GlobalSettings::getProgramDescription()
{
    return jsonObj["programDescription"].toString();
}

QString GlobalSettings::getProgramAuthor()
{
    return jsonObj["programAuthor"].toString();
}

QString GlobalSettings::getProgramLicense()
{
    return jsonObj["programLicense"].toString();
}

QString GlobalSettings::getProgramRepository()
{
    return jsonObj["programRepository"].toString();
}

QString GlobalSettings::getProgramOrganization()
{
    return jsonObj["programOrganization"].toString();
}

void GlobalSettings::getAllPath()
{

    QCoreApplication::processEvents(QEventLoop::AllEvents,5);
    QString regPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\steam\\Shell\\Open\\Command";
    QSettings *reg=new QSettings (regPath,QSettings::NativeFormat);
    QString steamPath = reg->value(".").toString();
    steamPath =QString(steamPath.split("\"")[1])+QString("\\..\\");
    //获取到cfg目录

    QString libraryFoldersFile = steamPath + "/steamapps/libraryfolders.vdf";
    if(steamPath.isEmpty()){
        qDebug() << "Steam Path is empty";
        return;
    }
    QString CFGPath = getPath(libraryFoldersFile);
    this->setCFGPath(CFGPath);
    this->setSteamPath(QString(steamPath).replace("\\","/").replace("steam.exe/..","").replace("//","/"));
    this->setSteamUserPath(QString(steamPath).replace("\\","/").replace("steam.exe/..","userdata"));
    this->setSteamConfPath(QString(steamPath).replace("\\","/").replace("steam.exe/..","config"));

    qDebug() << "CFGPath:" << CFGPath;
    qDebug() << "SteamUserPath:" << this->getSteamUserPath();
    qDebug() << "SteamConfPath:" << this->getSteamConfPath();
    qDebug() << "SteamPath:" << this->getSteamPath();
    
    //完美平台使用 计算机\HKEY_CLASSES_ROOT\wmpvp\shell\open\command 获取到wmpvp的路径
    // "C:\Program Files (x86)\perfectworldarena\完美世界竞技平台.exe" "%1"
    QString wmpvpPath = "HKEY_CLASSES_ROOT\\wmpvp\\shell\\open\\command";
    QSettings *wmpvpReg=new QSettings (wmpvpPath,QSettings::NativeFormat);
    QString wmpvpPath_ = wmpvpReg->value(".").toString();
    wmpvpPath_ = QString(wmpvpPath_.split("\"")[1]).replace("%1",steamPath);
    this->setWMPVPPath(wmpvpPath_.replace("\\","/"));
    qDebug() << "WMPVPPath:" << wmpvpPath_.replace("\\","/").replace("完美世界竞技平台.exe","");

    //5E平台路径 计算机\HKEY_CLASSES_ROOT\fe-play\shell\open\command 获取到5E平台的路径
    // "C:\Program Files (x86)\Counter-Strike Global Offensive\csgo.exe" "%1"
    QString eeeeePath = "HKEY_CLASSES_ROOT\\fe-play\\shell\\open\\command";
    QSettings *eeeeeReg=new QSettings (eeeeePath,QSettings::NativeFormat);
    QString eeeeePath_ = eeeeeReg->value(".").toString();
    eeeeePath_ = QString(eeeeePath_.split("\"")[1]).replace("%1",steamPath);
    this->setEEEEEPath(eeeeePath_.replace("\\","/"));
    qDebug() << "EEEEEPath:" << eeeeePath_.replace("\\","/").replace("5EClient.exe","");

    QString eeeeeLOP=QFileInfo(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first()).absolutePath()+"/5E对战平台/setting.json";
    /*
        {
	"csgo": {
		"path": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\game\\bin\\win64",
		"args": "-customLaunchOptionForSearch"
	}
}
    */
    this->setEEEEELaunchOptionFilePath(eeeeeLOP);
    qDebug() << "EEEEEELaunchOptionFilePath:" << this->getEEEEELaunchOptionFilePath();

    QString wmpvpLOP=QFileInfo(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first()).absolutePath()+"/Wmpvp/setting/game.json";
    /*
    {
        "csgocommand": "-commandU",
        "version": 3,
        "cs_check_result": {
            "steamlogined": true,
            "steammatch": true,
            "needcheck": false,
            "checkingprogress": "end",
            "anticheat": {
                "result": "success",
                "error": ""
            },
            "game": {
                "result": "success",
                "error": ""
            },
            "connect": {
                "result": "success",
                "error": ""
            },
            "checkpassed": true,
            "checkresult": "RESULT_SUCCESS"
        }
    }
    */
    this->setWmPvpLaunchOptionFilePath(wmpvpLOP);
    qDebug() << "WmPvpLaunchOptionFilePath:" << this->getWmPvpLaunchOptionFilePath();
    
    
}
QString GlobalSettings::getPath(QString vdfFile)
{
    QFile file(vdfFile);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString buffer(file.readAll());
    QString path;
    QStringList reader = buffer.split("\n");
    foreach(QString var,reader){
        if(var.contains("path")){
            path=var.split("\"")[3];
        }
        if(var.contains("730")) break;
    }

    //path -> steamapps -> common -> Counter-Strike Global Offensive ->game ->csgo -> cfg
    path=path.replace("\\\\","\\");
    path = path+"\\steamapps\\common\\Counter-Strike Global Offensive\\game\\csgo\\cfg";
    path.replace("\\","/");
    return path;
}