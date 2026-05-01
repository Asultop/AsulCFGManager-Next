#include "T_Manager.h"
#include "../Global/GlobalSettings.h"
#include "../Global/GlobalFunc.h"
#include "ElaDef.h"
#include "ElaComboBox.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QThread>
#include <QDebug>
#include <QList>
#include <fstream>
#include "AFormParser/AFormParser.hpp"

#define gSettings GlobalSettings::getInstance()
#define QLOG_DEBUG qDebug
#define QLOG_WARN qWarning
#define QLOG_ERROR qCritical

T_Manager::T_Manager(QWidget *parent)
    : BaseScrollPage{parent}
{
    this->initWidget(tr("启动项"), tr("管理"), tr("@AsulTop"));

    // 读取上次选择的平台
    QString lastPlatform = gSettings->getLastSelectedPlatform();
    m_currentPlatform = getPlatformFromString(lastPlatform.isEmpty() ? "Steam" : lastPlatform);

    // 创建顶部组件
    setupTopWidget();

    // 创建内容布局
    QWidget* contentWidget = new QWidget(this);
    m_contentLayout = new QVBoxLayout(contentWidget);
    m_contentLayout->setContentsMargins(10, 10, 20, 20);
    m_contentLayout->setSpacing(10);

    QVBoxLayout* centerVLayout = new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(10, 20, 20, 20);
    centerVLayout->addWidget(contentWidget);
    centerVLayout->addStretch();

    // 根据当前平台设置UI
    switch(m_currentPlatform) {
    case Platform::Steam:
        setupSteamUI();
        break;
    case Platform::WMPVP:
        setupWMPVPUI();
        break;
    case Platform::EEEEE:
        setupEEEEEUI();
        break;
    }
}

void T_Manager::setupTopWidget()
{
    QWidget* topWidget = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);

    // 左侧：平台 Label
    ElaText* platformLabel = new ElaText(tr("平台"), this);
    platformLabel->setTextPixelSize(15);
    QFont font = platformLabel->font();
    font.setBold(true);
    platformLabel->setFont(font);

    // 右侧：平台选择 ComboBox
    m_platformComboBox = new ElaComboBox(this);
    m_platformComboBox->setFixedWidth(200);

    // 添加选项
    m_platformComboBox->addItem("Steam");
    bool wmpvpExists = QFile::exists(gSettings->getWmPvpLaunchOptionFilePath());
    bool eeeeeExists = QFile::exists(gSettings->getEEEEELaunchOptionFilePath());

    if(wmpvpExists) {
        m_platformComboBox->addItem(tr("完美对战平台"));
    }
    if(eeeeeExists) {
        m_platformComboBox->addItem("5EPlay");
    }

    // 设置当前选中
    switch(m_currentPlatform) {
    case Platform::Steam:
        m_platformComboBox->setCurrentText("Steam");
        break;
    case Platform::WMPVP:
        m_platformComboBox->setCurrentText(tr("完美对战平台"));
        break;
    case Platform::EEEEE:
        m_platformComboBox->setCurrentText("5EPlay");
        break;
    }

    topLayout->addWidget(platformLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_platformComboBox);

    this->addTopWidget(topWidget);

    // 连接平台切换信号
    connect(m_platformComboBox, &QComboBox::currentTextChanged, this, [this](const QString& text){
        Platform newPlatform = getPlatformFromString(text);
        if(newPlatform == m_currentPlatform) return;

        if(!GlobalFunc::askDialog(this, tr("切换平台"),
            tr("更改平台将重启对应平台并调整设置，是否继续？"))) {
            // 恢复原选择
            blockSignals(true);
            switch(m_currentPlatform) {
            case Platform::Steam:
                m_platformComboBox->setCurrentText("Steam");
                break;
            case Platform::WMPVP:
                m_platformComboBox->setCurrentText(tr("完美对战平台"));
                break;
            case Platform::EEEEE:
                m_platformComboBox->setCurrentText("5EPlay");
                break;
            }
            blockSignals(false);
            return;
        }
        blockSignals(false);

        // 保存新选择
        gSettings->setLastSelectedPlatform(text);
        gSettings->getRegisterSettings()->setValue("LastSelectedPlatform", text);
        m_currentPlatform = newPlatform;

        // 清除并重建UI
        clearCurrentUI();
        switch(m_currentPlatform) {
        case Platform::Steam:
            setupSteamUI();
            break;
        case Platform::WMPVP:
            setupWMPVPUI();
            break;
        case Platform::EEEEE:
            setupEEEEEUI();
            break;
        }
    });
}

void T_Manager::setupSteamUI()
{
    m_cfgSwitchMap.clear();

    // 用户选择区域
    m_userComboBox = new ElaComboBox(this);
    m_userComboBox->setFixedWidth(200);

    QString userConf = gSettings->getSteamConfPath() + "/loginusers.vdf";
    QList<SteamUserInfo> userList = F_SteamUserQuery::parseUsersFile(userConf);
    m_userInfoMap.clear();

    for(const auto& userInfo : userList) {
        QString personaName = userInfo.personaName;
        if(userInfo.mostRecent) {
            personaName = personaName + tr(" (最近登录)");
        }
        m_userComboBox->addItem(personaName);
        m_userInfoMap[personaName] = userInfo;
    }

    for(const auto& userInfo : userList) {
        if(userInfo.mostRecent) {
            m_userComboBox->setCurrentText(userInfo.personaName + tr(" (最近登录)"));
            break;
        }
    }

    if(m_userInfoMap.isEmpty()) {
        GlobalFunc::showErr(tr("账号"), tr("请先登录账号"));
        return;
    }

    ElaScrollPageArea* userArea = GlobalFunc::GenerateArea(this, ElaIconType::User,
        new ElaText(tr("用户"), this),
        new ElaText(tr("选择用户"), this),
        m_userComboBox, false);
    m_contentLayout->addWidget(userArea);

    // 扫描本地 CFGs
    scanLocalCFGs();

    // 应用按钮
    ElaPushButton* applyButton = new ElaPushButton(tr("应用"), this);
    applyButton->setFixedWidth(100);
    connect(applyButton, &ElaPushButton::clicked, this, [this](){
        if(!GlobalFunc::askDialog(this, tr("重启"),
            tr("这将重启 Steam 要继续吗"))) {
            return;
        }
        applySteamChanges();
        restartPlatform();
    });

    ElaScrollPageArea* applyArea = GlobalFunc::GenerateArea(this, ElaIconType::BadgeCheck,
        new ElaText(tr("应用更改"), this),
        new ElaText(tr("点击应用后重启 Steam"), this),
        applyButton, false);
    m_contentLayout->addWidget(applyArea);
}

void T_Manager::setupWMPVPUI()
{
    m_cfgSwitchMap.clear();

    // 读取当前启动参数
    QString settingPath = gSettings->getWmPvpLaunchOptionFilePath();
    QFile file(settingPath);
    if(!file.open(QIODevice::ReadOnly)) {
        GlobalFunc::showErr(tr("完美平台"), tr("无法读取启动配置文件"));
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if(!doc.isObject()) {
        GlobalFunc::showErr(tr("完美平台"), tr("配置文件格式错误"));
        return;
    }

    QString currentArgs;
    QJsonObject rootObj = doc.object();
    if(rootObj.contains("csgocommand")) {
        currentArgs = rootObj["csgocommand"].toString();
    }

    m_originalLaunchOptions = currentArgs;

    // 扫描本地 CFGs
    scanLocalCFGs();

    // 应用按钮
    ElaPushButton* applyButton = new ElaPushButton(tr("应用"), this);
    applyButton->setFixedWidth(100);
    connect(applyButton, &ElaPushButton::clicked, this, [this](){
        if(!GlobalFunc::askDialog(this, tr("重启"),
            tr("这将重启完美世界竞技平台 要继续吗"))) {
            return;
        }
        applyWMPVPChanges();
        restartPlatform();
    });

    ElaScrollPageArea* applyArea = GlobalFunc::GenerateArea(this, ElaIconType::BadgeCheck,
        new ElaText(tr("应用更改"), this),
        new ElaText(tr("点击应用后重启完美世界竞技平台"), this),
        applyButton, false);
    m_contentLayout->addWidget(applyArea);
}

void T_Manager::setupEEEEEUI()
{
    m_cfgSwitchMap.clear();

    // 读取当前启动参数
    QString settingPath = gSettings->getEEEEELaunchOptionFilePath();
    QFile file(settingPath);
    if(!file.open(QIODevice::ReadOnly)) {
        GlobalFunc::showErr(tr("5EPlay"), tr("无法读取启动配置文件"));
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if(!doc.isObject()) {
        GlobalFunc::showErr(tr("5EPlay"), tr("配置文件格式错误"));
        return;
    }

    QString currentArgs;
    QJsonObject rootObj = doc.object();
    if(rootObj.contains("csgo")) {
        QJsonObject csgoObj = rootObj["csgo"].toObject();
        if(csgoObj.contains("args")) {
            currentArgs = csgoObj["args"].toString();
        }
    }

    m_originalLaunchOptions = currentArgs;

    // 扫描本地 CFGs
    scanLocalCFGs();

    // 应用按钮
    ElaPushButton* applyButton = new ElaPushButton(tr("应用"), this);
    applyButton->setFixedWidth(100);
    connect(applyButton, &ElaPushButton::clicked, this, [this](){
        if(!GlobalFunc::askDialog(this, tr("重启"),
            tr("这将重启 5EPlay 要继续吗"))) {
            return;
        }
        applyEEEEEChanges();
        restartPlatform();
    });

    ElaScrollPageArea* applyArea = GlobalFunc::GenerateArea(this, ElaIconType::BadgeCheck,
        new ElaText(tr("应用更改"), this),
        new ElaText(tr("点击应用后重启 5EPlay"), this),
        applyButton, false);
    m_contentLayout->addWidget(applyArea);
}

void T_Manager::clearCurrentUI()
{
    QLayoutItem* child;
    while((child = m_contentLayout->takeAt(0)) != nullptr) {
        if(child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    m_cfgSwitchMap.clear();
}

void T_Manager::scanLocalCFGs()
{
    QDir cfgDir(gSettings->getCFGPath());
    if(!cfgDir.exists()) {
        return;
    }

    // 获取当前平台的启动参数列表
    QStringList currentArgsList;
    if(m_currentPlatform == Platform::Steam) {
        currentArgsList = getCurrentLaunchOptions();
    } else if(m_currentPlatform == Platform::WMPVP) {
        currentArgsList = m_originalLaunchOptions.split(" ", Qt::SkipEmptyParts);
    } else if(m_currentPlatform == Platform::EEEEE) {
        currentArgsList = m_originalLaunchOptions.split(" ", Qt::SkipEmptyParts);
    }

    for(const auto& subDir : cfgDir.entryList(QDir::Dirs)) {
        if(subDir == "." || subDir == "..") {
            continue;
        }
        QString configPath = cfgDir.absoluteFilePath(subDir) + "/config.asul";
        if(!QFile::exists(configPath)) {
            continue;
        }

        QString launchArgs = extractLaunchArgs(configPath);
        if(launchArgs.isEmpty()) {
            continue;
        }

        ElaToggleSwitch* toggle = new ElaToggleSwitch(this);

        // 检查是否已启用
        bool isEnabled = currentArgsList.contains(launchArgs);
        toggle->setIsToggled(isEnabled);

        m_cfgSwitchMap[subDir] = {launchArgs, toggle};

        // 创建 UI 区域
        QString displayName = subDir;
        ElaText* titleText = new ElaText(displayName, this);
        ElaText* subtitleText = new ElaText(launchArgs, this);
        subtitleText->setStyleSheet("color: gray;");

        ElaScrollPageArea* area = GlobalFunc::GenerateArea(this, ElaIconType::FileCode,
            titleText, subtitleText, toggle, true);
        m_contentLayout->addWidget(area);
    }
}

QString T_Manager::extractLaunchArgs(const QString& configAsulPath)
{
    QFile file(configAsulPath);
    if(!file.open(QIODevice::ReadOnly)) {
        return "";
    }
    QString content = file.readAll();
    file.close();

    AFormParser::ParseError err;
    auto doc = AFormParser::Document::from(content, &err);
    if(!doc) {
        return "";
    }
    return doc->metaValue("@LaunchArgs");
}

QString T_Manager::getLocalConfigVDFPath()
{
    // 获取当前选中用户的 ShortId
    QString currentText = m_userComboBox->currentText();
    SteamUserInfo userInfo = m_userInfoMap.value(currentText);
    QString shortId = userInfo.userShortId;

    QString basePath = gSettings->getSteamUserPath();
    return basePath + "/" + shortId + "/config/localconfig.vdf";
}

QStringList T_Manager::getCurrentLaunchOptions()
{
    if(m_currentPlatform != Platform::Steam) {
        return QStringList();
    }

    QString filePath = getLocalConfigVDFPath();
    QFileInfo fileInfo(filePath);
    if(!fileInfo.exists()) {
        return QStringList();
    }

    try {
        std::ifstream file(filePath.toStdString());
        tyti::vdf::Options options;
        bool parseOk = false;
        auto root = tyti::vdf::read(file, &parseOk, options);
        file.close();

        if(!parseOk) {
            qWarning() << "[T_Manager] localconfig.vdf 解析失败";
            return QStringList();
        }

        // 导航到 Software -> Valve -> Steam -> apps -> 730
        auto softwareIt = root.childs.find("Software");
        if(softwareIt == root.childs.end() || !softwareIt->second) {
            return QStringList();
        }
        auto valveIt = softwareIt->second->childs.find("Valve");
        if(valveIt == softwareIt->second->childs.end() || !valveIt->second) {
            return QStringList();
        }
        auto steamIt = valveIt->second->childs.find("Steam");
        if(steamIt == valveIt->second->childs.end() || !steamIt->second) {
            return QStringList();
        }
        auto appsIt = steamIt->second->childs.find("apps");
        if(appsIt == steamIt->second->childs.end() || !appsIt->second) {
            return QStringList();
        }
        auto app730It = appsIt->second->childs.find("730");
        if(app730It == appsIt->second->childs.end() || !app730It->second) {
            return QStringList();
        }

        QString launchOpts = QString::fromStdString(app730It->second->attribs["LaunchOptions"]);
        m_originalLaunchOptions = launchOpts;
        return launchOpts.split(" ", Qt::SkipEmptyParts);
    } catch(const std::exception& e) {
        qCritical() << "[T_Manager] VDF 解析异常:" << e.what();
        return QStringList();
    }
}

bool T_Manager::checkIfArgEnabled(const QString& arg)
{
    if(m_currentPlatform == Platform::Steam) {
        QStringList currentArgs = getCurrentLaunchOptions();
        return currentArgs.contains(arg);
    } else if(m_currentPlatform == Platform::WMPVP) {
        return m_originalLaunchOptions.contains(arg);
    } else if(m_currentPlatform == Platform::EEEEE) {
        return m_originalLaunchOptions.contains(arg);
    }
    return false;
}

void T_Manager::applySteamChanges()
{
    QString filePath = getLocalConfigVDFPath();
    QFileInfo fileInfo(filePath);
    if(!fileInfo.exists()) {
        GlobalFunc::showErr(tr("Steam"), tr("localconfig.vdf 文件不存在"));
        return;
    }

    try {
        std::ifstream file(filePath.toStdString());
        tyti::vdf::Options options;
        bool parseOk = false;
        auto root = tyti::vdf::read(file, &parseOk, options);
        file.close();

        if(!parseOk) {
            GlobalFunc::showErr(tr("Steam"), tr("localconfig.vdf 解析失败"));
            return;
        }

        // 导航到 Software -> Valve -> Steam -> apps -> 730
        auto ensureChild = [](tyti::vdf::object& node, const std::string& key) -> tyti::vdf::object* {
            auto it = node.childs.find(key);
            if(it == node.childs.end() || !it->second) {
                node.childs[key] = std::make_unique<tyti::vdf::object>();
                it = node.childs.find(key);
            }
            return it->second.get();
        };

        tyti::vdf::object* software = ensureChild(root, "Software");
        tyti::vdf::object* valve = ensureChild(*software, "Valve");
        tyti::vdf::object* steam = ensureChild(*valve, "Steam");
        tyti::vdf::object* apps = ensureChild(*steam, "apps");
        tyti::vdf::object* app730 = ensureChild(*apps, "730");

        // 构建新的启动参数列表
        QStringList newArgs;
        for(const auto& pair : m_cfgSwitchMap) {
            if(pair.second->getIsToggled()) {
                QString arg = pair.first;  // launchArgs
                // 将参数分割并添加到列表
                for(const QString& a : arg.split(" ", Qt::SkipEmptyParts)) {
                    if(!newArgs.contains(a)) {
                        newArgs.append(a);
                    }
                }
            }
        }

        // 设置新的启动参数
        app730->attribs["LaunchOptions"] = newArgs.join(" ").toStdString();

        // 写回文件
        std::ofstream outFile(filePath.toStdString());
        tyti::vdf::write(outFile, root);
        outFile.close();

        GlobalFunc::showSuccess(tr("Steam"), tr("启动参数已保存"));
    } catch(const std::exception& e) {
        GlobalFunc::showErr(tr("Steam"), tr("保存失败: ") + QString(e.what()));
    }
}

void T_Manager::applyWMPVPChanges()
{
    QString settingPath = gSettings->getWmPvpLaunchOptionFilePath();
    QFile file(settingPath);
    if(!file.open(QIODevice::ReadOnly)) {
        GlobalFunc::showErr(tr("完美平台"), tr("无法读取启动配置文件"));
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if(!doc.isObject()) {
        GlobalFunc::showErr(tr("完美平台"), tr("配置文件格式错误"));
        return;
    }

    QJsonObject rootObj = doc.object();

    // 构建新的启动参数
    QStringList newArgs;
    for(const auto& pair : m_cfgSwitchMap) {
        if(pair.second->getIsToggled()) {
            QString arg = pair.first;
            for(const QString& a : arg.split(" ", Qt::SkipEmptyParts)) {
                if(!newArgs.contains(a)) {
                    newArgs.append(a);
                }
            }
        }
    }

    rootObj["csgocommand"] = newArgs.join(" ");

    // 写回文件
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        GlobalFunc::showErr(tr("完美平台"), tr("无法写入配置文件"));
        return;
    }
    file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
    file.close();

    GlobalFunc::showSuccess(tr("完美平台"), tr("启动参数已保存"));
}

void T_Manager::applyEEEEEChanges()
{
    QString settingPath = gSettings->getEEEEELaunchOptionFilePath();
    QFile file(settingPath);
    if(!file.open(QIODevice::ReadOnly)) {
        GlobalFunc::showErr(tr("5EPlay"), tr("无法读取启动配置文件"));
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if(!doc.isObject()) {
        GlobalFunc::showErr(tr("5EPlay"), tr("配置文件格式错误"));
        return;
    }

    QJsonObject rootObj = doc.object();

    // 构建新的启动参数
    QStringList newArgs;
    for(const auto& pair : m_cfgSwitchMap) {
        if(pair.second->getIsToggled()) {
            QString arg = pair.first;
            for(const QString& a : arg.split(" ", Qt::SkipEmptyParts)) {
                if(!newArgs.contains(a)) {
                    newArgs.append(a);
                }
            }
        }
    }

    if(rootObj.contains("csgo")) {
        QJsonObject csgoObj = rootObj["csgo"].toObject();
        csgoObj["args"] = newArgs.join(" ");
        rootObj["csgo"] = csgoObj;
    } else {
        QJsonObject csgoObj;
        csgoObj["args"] = newArgs.join(" ");
        rootObj["csgo"] = csgoObj;
    }

    // 写回文件
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        GlobalFunc::showErr(tr("5EPlay"), tr("无法写入配置文件"));
        return;
    }
    file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
    file.close();

    GlobalFunc::showSuccess(tr("5EPlay"), tr("启动参数已保存"));
}

void T_Manager::restartPlatform()
{
    switch(m_currentPlatform) {
    case Platform::Steam: {
        // 终止 Steam
        QProcess::startDetached("taskkill", QStringList() << "/F" << "/IM" << "steam.exe");
        QThread::msleep(500);
        // 启动 Steam
        QString steamExe = gSettings->getSteamPath();
        if(!steamExe.endsWith("/") && !steamExe.endsWith("\\")) {
            steamExe += "/";
        }
        steamExe += "steam.exe";
        QProcess::startDetached(steamExe);
        break;
    }
    case Platform::WMPVP: {
        // 终止完美世界竞技平台
        QProcess::startDetached("taskkill", QStringList() << "/F" << "/IM" << "完美世界竞技平台.exe");
        QThread::msleep(500);
        // 启动完美世界竞技平台
        QString wmpvpExe = gSettings->getWMPVPPath();
        if(!wmpvpExe.endsWith("/") && !wmpvpExe.endsWith("\\")) {
            wmpvpExe += "/";
        }
        wmpvpExe += "完美世界竞技平台.exe";
        QProcess::startDetached(wmpvpExe);
        break;
    }
    case Platform::EEEEE: {
        // 终止 5EClient
        QProcess::startDetached("taskkill", QStringList() << "/F" << "/IM" << "5EClient.exe");
        QThread::msleep(500);
        // 启动 5EClient
        QString eeeeeExe = gSettings->getEEEEEPath();
        if(!eeeeeExe.endsWith("/") && !eeeeeExe.endsWith("\\")) {
            eeeeeExe += "/";
        }
        eeeeeExe += "5EClient.exe";
        QProcess::startDetached(eeeeeExe);
        break;
    }
    }
}

T_Manager::Platform T_Manager::getPlatformFromString(const QString& platformStr)
{
    if(platformStr == "Steam") {
        return Platform::Steam;
    } else if(platformStr == tr("完美对战平台") || platformStr == "完美世界竞技平台") {
        return Platform::WMPVP;
    } else if(platformStr == "5EPlay" || platformStr == "5E") {
        return Platform::EEEEE;
    }
    return Platform::Steam;
}

QString T_Manager::getPlatformName(Platform platform)
{
    switch(platform) {
    case Platform::Steam:
        return "Steam";
    case Platform::WMPVP:
        return tr("完美世界竞技平台");
    case Platform::EEEEE:
        return "5EPlay";
    }
    return "Steam";
}
