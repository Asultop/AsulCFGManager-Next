#include "T_Deploy.h"
#include "../Global/GlobalFunc.h"
#include "../Global/GlobalSettings.h"
#include "../SystemKit/AsulApplication.h"
#include "ElaDef.h"
#include "../../3rd/QsLog/QsLog.h"

#include "ElaComboBox.h"
#include "ElaContentDialog.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaDrawerArea.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"
#include <qboxlayout.h>
#include <qdesktopservices.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qdebug.h>
#include <qwidget.h>
#include <QFile>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include "VerifyFileSdk/VerifyFileSdk.h"
#include "AFormParser/AFormParser.hpp"
#include "ElaProgressRing.h"
#include "AsulMultiDownloader.h"
#include "../ToolKit/ASteamSDK/ASteamUserQuery/F_SteamUserQuery.h"
#include "../ToolKit/LoadingProgress/LoadingProgressDialog.h"
#include <QThread>

#include "../COM_DeployPanelWindow/T_DeployPanel.h"
#include "../../3rd/ValveFileVDF-1.1.1/include/vdf_parser.hpp"
#include <fstream>


#define DBG(x) QLOG_DEBUG()<<"[Key] "<<#x<<" : "<<x;
#define _DBG(label,x) QLOG_DEBUG()<<"["<<label<<"] "<<#x<<" : "<<x;
#define gSettings GlobalSettings::getInstance()

using namespace QsLogging;

static bool copyFolder(const QString &fromDir, const QString &toDir, bool coverFileIfExist);

T_Deploy::T_Deploy(QWidget *parent)
    : BaseScrollPage{parent}
{
    this->initWidget(tr("CFG_Panel"),tr("安装"),tr("@AsulTop"));
    QVBoxLayout *centerVLayout=new QVBoxLayout(centralWidget);
    centerVLayout->setContentsMargins(10,20,20,20);
    // ElaText *subtitle=new ElaText(tr("在线商店暂未开放"),this);
    // centerVLayout->addWidget(GlobalFunc::GenerateArea(this,ElaIconType::GearCode,new ElaText("欢迎",this),subtitle,(QWidget*) nullptr));
   
    ElaComboBox * selectAccountComboBox=new ElaComboBox(this);
    selectAccountComboBox->setFixedWidth(200);
    centerVLayout->addWidget(GlobalFunc::GenerateArea(this,ElaIconType::User,new ElaText("选择账号",this),new ElaText("将同步到对应的账号中"),selectAccountComboBox));
    
    QString userConf=GlobalSettings::getInstance()->getSteamConfPath()+"/loginusers.vdf";
    // auto userList = F_SteamUserQuery::parseUsersFile(userConf);
    auto userList = F_SteamUserQuery::parseUsersFile(userConf);
    QMap<QString,SteamUserInfo> userInfoMap;
    for(const auto& userInfo : userList){
        QString personaName=userInfo.personaName;
        if(userInfo.mostRecent){
            personaName=personaName+tr(" (最近登录)");
        }
        selectAccountComboBox->addItem(personaName);
        userInfoMap[personaName]=userInfo;
    }
    for(const auto& userInfo : userList){
        if(userInfo.mostRecent){
            selectAccountComboBox->setCurrentText(userInfo.personaName + tr(" (最近登录)"));
            break;
        }
    }
    if(userInfoMap.isEmpty()){
        GlobalFunc::showErr(tr("账号"),tr("请先登录账号"));
        return;
    }
    
    // 在线商店下拉框
    ElaDrawerArea *drawerArea = new ElaDrawerArea(this);
    QWidget *drawerHeader = new QWidget(this);
    QHBoxLayout *drawerHeaderLayout = new QHBoxLayout(drawerHeader);
    ElaText *drawerIcon = new ElaText(this);
    drawerIcon->setTextPixelSize(15);
    drawerIcon->setElaIcon(ElaIconType::Shop);
    drawerIcon->setFixedSize(25, 25);
    ElaText *drawerText = new ElaText(tr("在线商店"), this);
    drawerText->setTextPixelSize(15);

    drawerHeaderLayout->addWidget(drawerIcon);
    drawerHeaderLayout->addWidget(drawerText);
    drawerHeaderLayout->addStretch();


    drawerArea->setDrawerHeader(drawerHeader);

    // 自定义源
    ElaLineEdit *sourceUrlEdit = new ElaLineEdit(this);
    sourceUrlEdit->setText(gSets->getCustomSourceURL());
    sourceUrlEdit->setFixedWidth(400);
    ElaPushButton *pullButton = new ElaPushButton(tr("拉取"), this);
    pullButton->setFixedWidth(80);
    ElaProgressRing *sourceProgressRing = new ElaProgressRing(this);
    sourceProgressRing->setFixedSize(36, 36);
    sourceProgressRing->setIsBusying(false);
    sourceProgressRing->setIsDisplayValue(false);
    sourceProgressRing->setMinimum(0);
    sourceProgressRing->setMaximum(100);
    sourceProgressRing->setValue(0);
    sourceProgressRing->setBusyingWidth(4);
    sourceProgressRing->hide();
    QHBoxLayout *sourceInputLayout = new QHBoxLayout();
    sourceInputLayout->addWidget(sourceUrlEdit);
    sourceInputLayout->addWidget(pullButton);
    sourceInputLayout->addWidget(sourceProgressRing);
    drawerArea->addDrawer(GlobalFunc::GenerateArea(this,
        ElaIconType::CloudArrowDown,
        new ElaText(tr("自定义源"), this),
        new ElaText(tr("从自定义源拉取配置包列表"), this),
        sourceInputLayout));

    connect(pullButton, &ElaPushButton::clicked, this, [this, sourceUrlEdit, pullButton, sourceProgressRing, drawerArea, userInfoMap, selectAccountComboBox](){
        QString srcUrl = sourceUrlEdit->text().trimmed();
        if(srcUrl.isEmpty()){
            GlobalFunc::showWarn(tr("自定义源"), tr("请输入源地址"));
            return;
        }
        QUrl qsrcUrl(srcUrl);
        if(!qsrcUrl.isValid()){
            GlobalFunc::showErr(tr("自定义源"), tr("无效的地址"));
            return;
        }

        pullButton->hide();
        sourceProgressRing->setIsBusying(true);
        sourceProgressRing->setIsDisplayValue(false);
        sourceProgressRing->show();

        QString manifestPath = gSets->getGLoc()->path() + "/manifest.json";

        AsulMultiDownloader *manifestDownloader = new AsulMultiDownloader(this);
        manifestDownloader->setMaxConcurrentDownloads(1);
        manifestDownloader->setAutoRetry(true);
        manifestDownloader->setMaxRetryCount(3);
        manifestDownloader->addDownload(qsrcUrl, manifestPath);

        connect(manifestDownloader, &AsulMultiDownloader::downloadProgress, this,
            [sourceProgressRing](const QString &taskId, qint64 received, qint64 total){
                Q_UNUSED(taskId);
                if(total > 0){
                    sourceProgressRing->setIsBusying(false);
                    sourceProgressRing->setRange(0, static_cast<int>(total));
                    sourceProgressRing->setValue(static_cast<int>(received));
                }
            });

        connect(manifestDownloader, &AsulMultiDownloader::downloadFinished, this,
            [this, manifestDownloader, manifestPath, pullButton, sourceProgressRing, drawerArea, userInfoMap, selectAccountComboBox]
            (const QString &taskId, const QString &savePath)
        {
            Q_UNUSED(taskId);
            Q_UNUSED(savePath);

            sourceProgressRing->hide();
            sourceProgressRing->setIsBusying(false);
            pullButton->show();

            QFile manifestFile(manifestPath);
            if(!manifestFile.open(QIODevice::ReadOnly)){
                GlobalFunc::showErr(tr("自定义源"), tr("无法读取 manifest 文件"));
                manifestDownloader->deleteLater();
                return;
            }
            QByteArray manifestData = manifestFile.readAll();
            manifestFile.close();
            QFile::remove(manifestPath);

            QJsonParseError parseErr;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(manifestData, &parseErr);
            if(parseErr.error != QJsonParseError::NoError || !jsonDoc.isArray()){
                GlobalFunc::showErr(tr("自定义源"), tr("manifest 格式错误: ") + parseErr.errorString());
                manifestDownloader->deleteLater();
                return;
            }

            QJsonArray entries = jsonDoc.array();
            if(entries.isEmpty()){
                GlobalFunc::showInfo(tr("自定义源"), tr("manifest 中没有条目"));
                manifestDownloader->deleteLater();
                return;
            }

            for(const auto &entry : entries){
                if(!entry.isObject()) continue;
                QJsonObject obj = entry.toObject();
                handleManifestEntry(obj, drawerArea, userInfoMap, selectAccountComboBox);
            }
            drawerArea->expand();
            GlobalFunc::showSuccess(tr("自定义源"), tr("已拉取 %1 个条目").arg(entries.size()));
            manifestDownloader->deleteLater();
        });

        connect(manifestDownloader, &AsulMultiDownloader::downloadFailed, this,
            [manifestDownloader, pullButton, sourceProgressRing](const QString &taskId, const QString &errorString){
                Q_UNUSED(taskId);
                sourceProgressRing->hide();
                sourceProgressRing->setIsBusying(false);
                pullButton->show();
                GlobalFunc::showErr(tr("自定义源"), tr("拉取失败: ") + errorString);
                manifestDownloader->deleteLater();
            });
    });

    // 自定义包 URL 输入
    ElaLineEdit *customUrlEdit = new ElaLineEdit(this);
    customUrlEdit->setPlaceholderText(tr("输入包下载地址..."));
    customUrlEdit->setFixedWidth(400);
    ElaPushButton *fetchButton = new ElaPushButton(tr("获取"), this);
    fetchButton->setFixedWidth(80);
    ElaProgressRing *progressRing = new ElaProgressRing(this);
    progressRing->setFixedSize(36, 36);
    progressRing->setIsBusying(false);
    progressRing->setIsDisplayValue(false);
    progressRing->setMinimum(0);
    progressRing->setMaximum(100);
    progressRing->setValue(0);
    progressRing->setBusyingWidth(4);
    progressRing->hide();
    QHBoxLayout *urlInputLayout = new QHBoxLayout();
    urlInputLayout->addWidget(customUrlEdit);
    urlInputLayout->addWidget(fetchButton);
    urlInputLayout->addWidget(progressRing);
    drawerArea->addDrawer(GlobalFunc::GenerateArea(this, ElaIconType::Globe, new ElaText(tr("自定义包Url"), this), new ElaText(tr("从网络地址获取配置包"), this), urlInputLayout));

    connect(fetchButton, &ElaPushButton::clicked, this, [this, customUrlEdit, drawerArea, userInfoMap, selectAccountComboBox, fetchButton, progressRing](){
        QString url = customUrlEdit->text().trimmed();
        if(url.isEmpty()){
            GlobalFunc::showWarn(tr("获取"), tr("请输入下载地址"));
            return;
        }
        QUrl qurl(url);
        if(!qurl.isValid()){
            GlobalFunc::showErr(tr("获取"), tr("无效的地址"));
            return;
        }

        QString downloadDir = gSets->getGLoc()->path() + "/Downloads";
        QDir().mkpath(downloadDir);
        QString fileName = qurl.fileName().isEmpty() ? "package.zip" : qurl.fileName();
        QString destPath = downloadDir + "/" + fileName;

        fetchButton->hide();
        progressRing->setIsBusying(true);
        progressRing->setIsDisplayValue(false);
        progressRing->show();

        AsulMultiDownloader *downloader = new AsulMultiDownloader(this);
        downloader->setMaxConcurrentDownloads(1);
        downloader->setAutoRetry(true);
        downloader->setMaxRetryCount(3);
        downloader->addDownload(qurl, destPath);

        connect(downloader, &AsulMultiDownloader::downloadProgress, this,
            [progressRing](const QString &taskId, qint64 received, qint64 total){
                Q_UNUSED(taskId);
                if(total > 0){
                    progressRing->setIsBusying(false);
                    progressRing->setRange(0, static_cast<int>(total));
                    progressRing->setValue(static_cast<int>(received));
                }
            });

        connect(downloader, &AsulMultiDownloader::downloadFinished, this,
            [this, downloader, destPath, url, drawerArea, userInfoMap, selectAccountComboBox, fetchButton, progressRing]
            (const QString &taskId, const QString &savePath)
        {
            Q_UNUSED(taskId);
            Q_UNUSED(savePath);

            // 下载完成，切换 ring 到 Busy 模式（解压阶段无法获取精确进度）
            progressRing->setIsBusying(true);
            progressRing->setIsDisplayValue(false);

            // 验证 zip 包签名
            QString rootCaPath(":/certs/RootCA-Cert/root-ca.crt.pem");
            VerifyFile::SimpleVerifyResult result = VerifyFile::VerifyFileSdk::verifyFileSimple(destPath, rootCaPath);

            if(!result.trusted){
                bool userAccept = GlobalFunc::askDialog(this, tr("签名：未受信任"),
                    tr("此包签名验证未通过，可能存在安全风险。\n\n是否继续？"));
                if(!userAccept){
                    QFile::remove(destPath);
                    progressRing->hide();
                    progressRing->setIsBusying(false);
                    fetchButton->show();
                    downloader->deleteLater();
                    return;
                }
            }else{
                GlobalFunc::showSuccess(tr("签名"), tr("受信任开发者：") + result.info.signer);
            }

            // 异步解压
            QString extracDir = gSettings->getGLoc()->path() + "/Downloads/" + QFileInfo(destPath).baseName() + "-extract";
            gFunc->UnzipFileAsync(destPath, extracDir, [this, destPath, extracDir, result, drawerArea, userInfoMap, selectAccountComboBox, fetchButton, progressRing, downloader](bool success){
                QFile::remove(destPath);
                // 解压完成，恢复 UI
                progressRing->hide();
                progressRing->setIsBusying(false);
                fetchButton->show();

                if(!success){
                    GlobalFunc::showErr(tr("获取"), tr("解压失败"));
                    QDir(extracDir).removeRecursively();
                    downloader->deleteLater();
                    return;
                }
                handleExtractedPackage(extracDir, tr("获取"), result, drawerArea, userInfoMap, selectAccountComboBox);
                GlobalFunc::showSuccess(tr("获取"), tr("包校验完成，已就绪"));
                downloader->deleteLater();
            });
        });

        connect(downloader, &AsulMultiDownloader::downloadFailed, this,
            [downloader, fetchButton, progressRing](const QString &taskId, const QString &errorString){
                Q_UNUSED(taskId);
                progressRing->hide();
                progressRing->setIsBusying(false);
                fetchButton->show();
                GlobalFunc::showErr(tr("下载"), tr("下载失败: ") + errorString);
                downloader->deleteLater();
            });
    });

    centerVLayout->addWidget(drawerArea);

    // 本地文件下拉框
    ElaDrawerArea *localFileDrawerArea = new ElaDrawerArea(this);
    QWidget *localFileDrawerHeader = new QWidget(this);
    QHBoxLayout *localFileDrawerHeaderLayout = new QHBoxLayout(localFileDrawerHeader);
    ElaText *localFileDrawerIcon = new ElaText(this);
    localFileDrawerIcon->setTextPixelSize(15);
    localFileDrawerIcon->setElaIcon(ElaIconType::FolderArrowDown);
    localFileDrawerIcon->setFixedSize(25, 25);
    ElaText *localFileDrawerText = new ElaText(tr("本地文件"), this);
    localFileDrawerText->setTextPixelSize(15);

    ElaPushButton *scanButton = new ElaPushButton(tr("扫描"), this);
    scanButton->setFixedWidth(100);
    ElaPushButton *importButton = new ElaPushButton(tr("导入"), this);
    importButton->setFixedWidth(100);

    QWidget * headerWidget=new QWidget(this);
    QHBoxLayout *headerLayout=new QHBoxLayout(headerWidget);
    headerLayout->addWidget(scanButton);
    headerLayout->addWidget(importButton);
    connect(scanButton,&ElaPushButton::clicked,this,[this,localFileDrawerArea,userInfoMap,selectAccountComboBox](){
        GlobalFunc::showInfo(tr("扫描"),tr("扫描本地文件"));
        QDir localCFGRootDir(gSets->getCFGPath());
        for(const auto &subDir: localCFGRootDir.entryList(QDir::Dirs)){
            qDebug() << "[T_Deploy] Scan subDir:" << subDir;
            if(subDir == "." || subDir == ".."){
                continue;
            }
            QString subDirPath=localCFGRootDir.absoluteFilePath(subDir);
            QFile subConfig=QDir(subDirPath).filePath("config.asul");
            if(!subConfig.exists()){
                continue;
            }
            if(!subConfig.open(QIODevice::ReadOnly)){
                continue;
            }
            QString docFileContent=subConfig.readAll();
            subConfig.close();


            AFormParser::ParseError err;
            auto doc = AFormParser::Document::from(docFileContent,&err);
            if(!doc){
                GlobalFunc::showErr(tr("扫描"),tr("解析失败")+err.message);
                qWarning()<<"解析失败："<<err.line<<"<"<<err.column<<">: "<<err.message;
                continue;
            }
            qDebug() << "[T_Deploy] check arguments ";
            QStringList argList={"Version","Name","Target","Author","Description"};
            for(auto arg:argList){
                if(doc->metaValue(arg).isEmpty()){
                    GlobalFunc::showErr(tr("扫描"),tr("缺少参数")+" "+arg);
                    qCritical()<< "[T_Deploy] Missing argument: "<<arg;
                    continue;
                }
            }
            qDebug() << "[T_Deploy] doc parsed successfully, forms count:" << doc->forms.size();

            if(m_scannedAreaMap.contains(subDirPath)){
                qDebug() << "[T_Deploy] Config already in drawer, replacing:" << subDirPath;
                ElaScrollPageArea *oldArea = m_scannedAreaMap[subDirPath];
                localFileDrawerArea->removeDrawer(oldArea);
                delete oldArea;
            }

            ElaPushButton *editButton = new ElaPushButton(tr("编辑"), this);
            editButton->setFixedWidth(100);
            ElaPushButton *deleteButton = new ElaPushButton(tr("删除"), this);
            deleteButton->setFixedWidth(100);
            QColor defaultRedColor(255,87,87);
            deleteButton->setDarkDefaultColor(defaultRedColor);
            deleteButton->setDarkHoverColor(gFunc->getLighterColor(defaultRedColor));
            deleteButton->setDarkPressColor(gFunc->getDarkerColor(defaultRedColor));
            deleteButton->setLightDefaultColor(defaultRedColor);
            deleteButton->setLightHoverColor(gFunc->getDarkerColor(defaultRedColor));
            deleteButton->setLightPressColor(gFunc->getLighterColor(defaultRedColor));
            QWidget *buttonWidget = new QWidget(this);
            QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
            buttonLayout->setContentsMargins(0, 0, 0, 0);
            buttonLayout->addStretch();
            buttonLayout->addWidget(editButton,1);
            buttonLayout->addWidget(deleteButton,1);

            ElaScrollPageArea *gArea;
            QString displayName = doc->metaValue("Name") + " (" + doc->metaValue("Target") + ")";
            if(!doc->metaValue("Picture").isEmpty() && QFile(subDirPath+"/"+doc->metaValue("Picture")).exists()){
                gArea = gFunc->GenerateArea(
                    this,
                    QString(subDirPath+"/"+doc->metaValue("Picture")),
                    new ElaText(displayName,this),
                    new ElaText(doc->metaValue("Author"),this),
                    buttonWidget,false
                );
            }else{
                gArea = gFunc->GenerateArea(
                    this,
                    QString(":/Pictures/Pictures/CS2.png"),
                    new ElaText(displayName,this),
                    new ElaText(doc->metaValue("Author"),this),
                    buttonWidget,false
                );
            }
            localFileDrawerArea->addDrawer(gArea);
            m_scannedAreaMap[subDirPath] = gArea;
            qDebug() << "[T_Deploy] About to connect editButton clicked";
            connect(editButton,&ElaPushButton::clicked,this,[this,gArea,localFileDrawerArea,subDirPath,doc,userInfoMap,selectAccountComboBox,editButton]() -> void{
                SteamUserInfo currentUserInfo=userInfoMap[selectAccountComboBox->currentText()];
                qDebug()<< "[T_Deploy] About to set global variables";
                qDebug()<< "[T_Deploy] __UserId: "<<currentUserInfo.userId;
                qDebug()<< "[T_Deploy] __UserShortId: "<<currentUserInfo.userShortId;
                qDebug()<< "[T_Deploy] __AccountName: "<<currentUserInfo.accountName;
                qDebug()<< "[T_Deploy] __PersonaName: "<<currentUserInfo.personaName;
                QDir CFGDir(QDir::cleanPath(gSettings->getCFGPath()+"/"+doc->metaValue("Target")));

                QMap<QString,QString> registerMap={
                    {"__UserId",currentUserInfo.userId},
                    {"__UserShortId",currentUserInfo.userShortId},
                    {"__AccountName",currentUserInfo.accountName},
                    {"__PersonaName",currentUserInfo.personaName},
                    {"__WhereAmI", CFGDir.absolutePath()}
                };
                doc->setGlobalVariables(registerMap);

                qDebug() << "[T_Deploy] About to create T_DeployPanel with doc";
                T_DeployPanel * deployPanel = new T_DeployPanel(nullptr,doc,subDirPath);
                qDebug() << "[T_Deploy] T_DeployPanel created";
                connect(deployPanel,&T_DeployPanel::deployFinished,this,[this,gArea,doc,subDirPath,deployPanel,localFileDrawerArea](){
                    QDir sourceDir(subDirPath);
                    QFile AsulConfig(sourceDir.filePath("config.asul"));
                    if(!AsulConfig.open(QIODevice::ReadWrite | QIODevice::Truncate)){
                        gFunc->showErr(tr("保存"),tr("文件打开失败:")+" "+AsulConfig.fileName());
                        qDebug() << "[T_Deploy] Failed to open config.asul : "<<AsulConfig.fileName();
                        return;
                    }
                    if(!AsulConfig.write(doc->dump().toUtf8())){
                        gFunc->showErr(tr("保存"),tr("文件写入失败:")+" "+AsulConfig.fileName());
                        qDebug() << "[T_Deploy] Failed to write config.asul : "<<AsulConfig.fileName();
                        return;
                    }
                    AsulConfig.close();

                    QDir CFGDir(QDir::cleanPath(gSettings->getCFGPath()+"/"+doc->metaValue("Target")));
                    for(auto &CFGExport: doc->toCFGs()){
                        DBG(" ============ deployProfile ============ ");
                        DBG(" - deployProfile:" << CFGExport.output);
                        DBG(" - deployProfile:" << CFGExport.content);
                        DBG(" - deployProfile:" << CFGExport.relativePath);
                        DBG(" - deployProfile:" << CFGExport.absolutePath);
                        DBG(" - deployProfile:" << CFGExport.fileName);
                        DBG(" - deployProfile:" << CFGExport.sourceFormId);
                        DBG(" - deployProfile:" << CFGExport.description);
                        DBG(" - deployProfile done ===============");
                        QFile cfgFile(CFGDir.filePath(CFGExport.fileName));
                        if(!cfgFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){
                            gFunc->showErr(tr("保存"),tr("文件打开失败:")+" "+cfgFile.fileName());
                            qDebug() << "[T_Deploy] Failed to open config file : "<<cfgFile.fileName();
                            return;
                        }
                        QTextStream out(&cfgFile);
                        out << tr("// ====== 生成的文件 =======")+"\n";
                        out << tr("//=这个 %1 文件由 Asul-CFGManager(AM) 根据配置自动生成 ").arg(CFGExport.fileName)+"\n";
                        out << tr("\n//==这个 配置文件 从哪儿来的?")+"\n";
                        out << tr("//CFG 制作者: ")<<CFGExport.sourceFormId<<"\n";
                        out << tr("//CFG 名称: ")<<CFGExport.description<<"\n";
                        out << tr("//CFG 版本: ")<<doc->metaValue("Version")<<"\n";
                        out << tr("//==CFG 详细 结束")+"\n\n";

                        out << CFGExport.content << "\r\n";

                        out << "\r\n"+tr("//==参数结束")+"\n";
                        out << tr("//AM 是由 Alivn开发的部署 CS2 CFG 的程序,旨在为CFG制作者提供更方便的分发服务 以及 使用者提供方便的配置服务")+"\n";
                        out << tr("//开发者:Github(https://github.com/AsulTop),网站(http://www.asul.top)")+"\n";
                        out << tr("//配置时间: ")<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")+"\n\n";
                    }

                    if(gSets->getOpenFolderAfterDeploy()){
                        QDesktopServices::openUrl(QUrl("file:///" + CFGDir.absolutePath()));
                    }
                    if(gSets->getExecuteProgramAfterDeploy() && !doc->metaValue("Exec").isEmpty()){
                        QString execRelativePath = doc->metaValue("Exec");
                        QString cleanExecPath = QDir::cleanPath(CFGDir.absoluteFilePath(execRelativePath));
                        if(!cleanExecPath.contains(gSettings->getCFGPath())){
                            gFunc->showErr(tr("运行"),tr("可执行文件路径不在允许的目录下"));
                            qCritical()<< "[T_Deploy] Exec path not in CFG path: "<<cleanExecPath;
                        }else if(!QFile::exists(cleanExecPath)){
                            gFunc->showErr(tr("运行"),tr("可执行文件不存在")+" "+cleanExecPath);
                            qDebug() << "[T_Deploy] Exec file not found: "<<cleanExecPath;
                        }else{
                            QProcess::startDetached("cmd", QStringList() << "/c" << "start" << "" << cleanExecPath, CFGDir.absolutePath());
                        }
                    }

                    deployPanel->deleteLater();

                });
                qDebug() << "[T_Deploy] editButton clicked, about to show deployPanel";
                deployPanel->show();
                qDebug() << "[T_Deploy] deployPanel->show() called";
            });
            connect(deleteButton, &ElaPushButton::clicked, this, [this, gArea, localFileDrawerArea, subDirPath, doc]() {
                if(GlobalFunc::askDialog(this, tr("注意"), tr("删除将导致CFG不可用并且按键恢复默认设置\n是否继续?"))) {
                    uninstallCFG(subDirPath);
                    m_scannedAreaMap.remove(subDirPath);
                    localFileDrawerArea->removeDrawer(gArea);
                    gArea->deleteLater();
                    // delete gArea;
                    // gArea = nullptr;
                }
            });
            localFileDrawerArea->expand();
        }

    });

    localFileDrawerHeaderLayout->addWidget(localFileDrawerText);
    localFileDrawerHeaderLayout->addStretch();
    localFileDrawerHeaderLayout->addWidget(headerWidget);
    
    // QWidget *templateNoFileWidget=new QWidget(this);
    // ElaText *templateNoFileText=new ElaText(tr("暂无文件"),this);
    // QVBoxLayout *templateNoFileLayout=new QVBoxLayout(templateNoFileWidget);
    // templateNoFileLayout->addStretch();
    // templateNoFileLayout->addWidget(templateNoFileText);
    // templateNoFileLayout->addStretch();
    // templateNoFileLayout->setContentsMargins(0,0,0,0);
    // templateNoFileText->setTextPixelSize(15);
    // localFileDrawerArea->addDrawer(templateNoFileWidget);

    localFileDrawerArea->setDrawerHeader(localFileDrawerHeader);
    centerVLayout->addWidget(localFileDrawerArea);
    centerVLayout->addStretch();


    connect(importButton,&ElaPushButton::clicked,this,[this,localFileDrawerArea,userInfoMap,selectAccountComboBox](){
        QString rootCaPath(":/certs/RootCA-Cert/root-ca.crt.pem");
        QString filePath = QFileDialog::getOpenFileName(this,tr("打开文件"),QApplication::applicationDirPath(),tr(" Files (*.*)"));
        if(filePath.isEmpty()){
            return;
        }

        LoadingProgressDialog *loadingProgressDialog = new LoadingProgressDialog(this);
        loadingProgressDialog->setTitle(tr("解析包"));
        loadingProgressDialog->setProgressMode(LoadingProgressDialog::ProgressMode::Step);
        loadingProgressDialog->setRange(0, 100);
        loadingProgressDialog->show();

        QString destLocation = gSettings->getGLoc()->filePath(QFileInfo(filePath).fileName());
        qDebug() << "Copy "<< filePath << " -> " << destLocation;
        QApplication::processEvents();
        QFile::copy(filePath,destLocation);
        
        QString extracDir = gSettings->getGLoc()->path() + "/" +QFileInfo(filePath).baseName() +"-extract";
        qDebug() << "Unzip "<< destLocation << " -> " << extracDir;
        // 验证 zip 包签名
        VerifyFile::SimpleVerifyResult result = VerifyFile::VerifyFileSdk::verifyFileSimple(destLocation, rootCaPath);
        qDebug() << "Has Signature:" << result.hasSignature;
        qDebug() << "Signature valid:" << result.signatureValid;
        qDebug() << "Trusted:" << result.trusted;
        qDebug() << "Signer:" << result.info.signer;
        qDebug() << "Status:" << static_cast<int>(result.status);
        if (!result.errorMessage.isEmpty()) {
            qDebug() << "Error:" << result.errorMessage;
        }
        if(result.trusted){
            GlobalFunc::showSuccess(tr("签名"),tr("受信任开发者")+" "+result.info.signer);
        }else{
            bool askRec=GlobalFunc::askDialog(this,tr("签名：未受信任开发者签名"),tr("文件 ")+QFileInfo(filePath).fileName()+tr(" 可能存在风险，是否继续？"));
            if(!askRec){
                QFile::remove(destLocation);
                loadingProgressDialog->close();
                delete loadingProgressDialog;
                return;
            }
        }

        // 异步解压
        gFunc->UnzipFileAsync(destLocation, extracDir, [this, destLocation, extracDir, result, localFileDrawerArea, userInfoMap, selectAccountComboBox, loadingProgressDialog](bool success){
            QFile::remove(destLocation);
            loadingProgressDialog->setValue(25);
            if(!success){
                GlobalFunc::showErr(tr("导入"), tr("解压失败"));
                QDir(extracDir).removeRecursively();
                loadingProgressDialog->close();
                delete loadingProgressDialog;
                return;
            }
            handleExtractedPackage(extracDir, tr("导入"), result, localFileDrawerArea, userInfoMap, selectAccountComboBox);
            loadingProgressDialog->close();
            delete loadingProgressDialog;
            localFileDrawerArea->expand();
        });

    });
}

static bool copyFolder(const QString &fromDir, const QString &toDir, bool coverFileIfExist = true)
{

    QDir sourceDir(QDir::cleanPath(fromDir));
    QDir targetDir(QDir::cleanPath(toDir));


    if (!sourceDir.exists()) {
        qWarning() << "[T_Deploy] Source directory does not exist:" << fromDir;
        return false;
    }

    if (!targetDir.exists()) {
        if (!targetDir.mkpath(targetDir.absolutePath())) {
            qWarning() << "[T_Deploy] Failed to create target directory:" << toDir;
            return false;
        }
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

    foreach (QFileInfo fileInfo, fileInfoList) {
        QString fileName = fileInfo.fileName();
        QString srcPath = fileInfo.absoluteFilePath();
        QString dstPath = targetDir.absoluteFilePath(fileName);

        if (fileInfo.isDir()) {
            if (!copyFolder(srcPath, dstPath, coverFileIfExist)) {
                return false;
            }
        } else {
            if (coverFileIfExist && QFile::exists(dstPath)) {
                QFile::remove(dstPath); // 覆盖
            }
            if (!QFile::copy(srcPath, dstPath)) {
                qWarning() << "[T_Deploy] Failed to copy file:" << srcPath;
                return false;
            }
        }
    }
    return true;
}

void T_Deploy::uninstallCFG(const QString& location)
{
    qDebug() << location;
    QString userConf = gSettings->getSteamConfPath() + "/loginusers.vdf";
    QList<SteamUserInfo> allUsers = F_SteamUserQuery::parseUsersFile(userConf);
    QString userMachieLocation = R"(%STEAM_USERPATH%%STEAM_SHORTID%/730/local/cfg/cs2_machine_convars.vcfg)";
    QString userFileLocation = R"(%STEAM_USERPATH%%STEAM_SHORTID%/730/local/cfg/cs2_user_keys_0_slot0.vcfg)";
    userFileLocation.replace("%STEAM_USERPATH%", gSettings->getSteamUserPath());
    userMachieLocation.replace("%STEAM_USERPATH%", gSettings->getSteamUserPath());
    QString userName;
    for(const auto& node : allUsers) {
        if(node.mostRecent) {
            userFileLocation.replace("%STEAM_SHORTID%", node.userShortId);
            userMachieLocation.replace("%STEAM_SHORTID%", node.userShortId);
            userName = node.personaName;
        }
    }

    if(GlobalFunc::askDialog(this, tr("注意"), tr("这会重置 ") + userName + tr("(**最近登陆**) 的所有按键绑定设置! "))) {
        QFile cs2_user_keys_0_slot0(userFileLocation);
        QDir(location).removeRecursively();
        if(!cs2_user_keys_0_slot0.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "无法打开文件：" << cs2_user_keys_0_slot0.errorString();
            return;
        }
        QTextStream out(&cs2_user_keys_0_slot0);
        out << R"("config"
{

})";
        cs2_user_keys_0_slot0.close();

        try {
            std::ifstream file(userMachieLocation.toStdString());
            if(!file.is_open()) {
                DBG(tr("无法打开文件: ") + userMachieLocation);
                return;
            }

            tyti::vdf::Options options;
            tyti::vdf::object root;
            bool parse_ok;
            try {
                root = tyti::vdf::read(file, &parse_ok, options);
            } catch(const std::exception& e) {
                DBG(tr("解析失败: ") + e.what());
                return;
            }

            if(!parse_ok) {
                DBG(tr("[内部错误] VDF 格式错误!"));
                return;
            }

            auto convars_it = root.childs.find("convars");
            if(convars_it == root.childs.end() || !convars_it->second) {
                DBG(tr("[内部错误] convar 节点丢失"));
                return;
            }

            auto& convars_node = *convars_it->second;
            convars_node.attribs["cl_scoreboard_mouse_enable_binding"] = "+attack2";

            auto writeVdfToFile = [](const std::string& filePath, const tyti::vdf::object& root) {
                std::ofstream file(filePath);
                if(!file.is_open()) {
                    DBG(tr("无法打开文件进行写入: ") + QString::fromStdString(filePath) + "\n");
                    return false;
                }
                tyti::vdf::Options options;
                tyti::vdf::write(file, root);
                return true;
            };

            if(writeVdfToFile(userMachieLocation.toStdString(), root)) {
                DBG(tr("操作成功"));
            }
        } catch(const std::exception& e) {
            DBG(tr("VDF 操作错误: ") + QString(e.what()));
            return;
        }

        GlobalFunc::showSuccess(tr("删除"), tr("操作成功"));
    }
}

bool T_Deploy::handleExtractedPackage(const QString &extracDir, const QString &title,
                                      const VerifyFile::SimpleVerifyResult &signResult,
                                      ElaDrawerArea *drawerArea,
                                      const QMap<QString, SteamUserInfo> &userInfoMap,
                                      ElaComboBox *selectAccountComboBox)
{
    QString AsulFormFilePath = extracDir + "/config.asul";
    QFile AsulFormFile(AsulFormFilePath);
    if(!AsulFormFile.exists()){
        GlobalFunc::showErr(title, tr("包中未找到 config.asul"));
        QDir(extracDir).removeRecursively();
        return false;
    }
    if(!AsulFormFile.open(QIODevice::ReadOnly)){
        GlobalFunc::showErr(title, tr("文件打开失败"));
        QDir(extracDir).removeRecursively();
        return false;
    }
    QString configContent = AsulFormFile.readAll();
    AsulFormFile.close();

    AFormParser::ParseError err;
    auto doc = AFormParser::Document::from(configContent, &err);
    if(!doc){
        GlobalFunc::showErr(title, tr("解析失败: ") + err.message);
        QDir(extracDir).removeRecursively();
        return false;
    }
    QStringList argList = {"Version","Name","Target","Author","Description"};
    for(auto arg : argList){
        if(doc->metaValue(arg).isEmpty()){
            GlobalFunc::showErr(title, tr("缺少参数: ") + arg);
            QDir(extracDir).removeRecursively();
            return false;
        }
    }

    // 校验 Target 路径安全性
    QString targetDir = QDir(gSets->getCFGPath()).filePath(doc->metaValue("Target"));
    QString cleanTarget = QDir::cleanPath(targetDir);
    if(!cleanTarget.contains(gSets->getCFGPath())){
        GlobalFunc::showErr(title, tr("目标路径不在CFG目录下"));
        QDir(extracDir).removeRecursively();
        return false;
    }

    // 创建安装 UI
    ElaPushButton *installButton = new ElaPushButton(tr("安装"), this);
    installButton->setFixedWidth(50);

    QWidget *subtitleWidget = new QWidget(this);
    QHBoxLayout *subtitleLayout = new QHBoxLayout(subtitleWidget);
    subtitleLayout->setContentsMargins(0,0,0,0);
    subtitleLayout->setSpacing(6);
    ElaText *authorText = new ElaText(doc->metaValue("Author"), this);
    authorText->setTextPixelSize(12);
    subtitleLayout->addWidget(authorText);
    subtitleLayout->addStretch();

    ElaScrollPageArea *gArea;
    if(!doc->metaValue("Picture").isEmpty() && QFile(extracDir + "/" + doc->metaValue("Picture")).exists()){
        gArea = gFunc->GenerateArea(this, QString(extracDir + "/" + doc->metaValue("Picture")),
            new ElaText(doc->metaValue("Name"), this), subtitleWidget, installButton, false);
    }else{
        gArea = gFunc->GenerateArea(this, QString(":/Pictures/Pictures/CS2.png"),
            new ElaText(doc->metaValue("Name"), this), subtitleWidget, installButton, false);
    }
    if(signResult.trusted){
        installButton->setText(installButton->text() + " " + tr("(受信任)"));
        installButton->setLightDefaultColor(QColor(143, 204, 167));
        installButton->setDarkDefaultColor(QColor(39, 135, 95));
    }

    drawerArea->addDrawer(gArea);
    drawerArea->expand();

    // 连接安装按钮
    connect(installButton, &ElaPushButton::clicked, this, [this, signResult, gArea, drawerArea, extracDir, doc, userInfoMap, selectAccountComboBox](){
        SteamUserInfo currentUserInfo = userInfoMap[selectAccountComboBox->currentText()];
        QDir CFGDir(QDir::cleanPath(gSettings->getCFGPath() + "/" + doc->metaValue("Target")));

        // Target 冲突检测
        if(CFGDir.exists()){
            QFile existingConfig(CFGDir.filePath("config.asul"));
            if(existingConfig.exists() && existingConfig.open(QIODevice::ReadOnly)){
                AFormParser::ParseError existErr;
                auto existDoc = AFormParser::Document::from(QString::fromUtf8(existingConfig.readAll()), &existErr);
                existingConfig.close();
                if(existDoc){
                    QString existAuthor = existDoc->metaValue("Author");
                    QString existVersion = existDoc->metaValue("Version");
                    QString newAuthor = doc->metaValue("Author");
                    QString newVersion = doc->metaValue("Version");
                    if(existAuthor == newAuthor){
                        if(!GlobalFunc::askDialog(this, tr("更新确认"),
                            tr("目标目录已存在同作者配置：\n\n") +
                            tr("作者: ") + existAuthor + "\n" +
                            tr("当前版本: ") + existVersion + "\n" +
                            tr("新版本: ") + newVersion + "\n\n" +
                            tr("是否更新？"))){
                            return;
                        }
                    }else{
                        if(!GlobalFunc::askDialog(this, tr("目标冲突"),
                            tr("目标目录已存在不同作者的配置：\n\n") +
                            tr("已有作者: ") + existAuthor + " (" + existVersion + ")\n" +
                            tr("导入作者: ") + newAuthor + " (" + newVersion + ")\n\n" +
                            tr("覆盖可能导致配置丢失，是否继续？"))){
                            return;
                        }
                    }
                }
            }
        }

        QMap<QString,QString> registerMap={
            {"__UserId", currentUserInfo.userId},
            {"__UserShortId", currentUserInfo.userShortId},
            {"__AccountName", currentUserInfo.accountName},
            {"__PersonaName", currentUserInfo.personaName},
            {"__WhereAmI", CFGDir.absolutePath()}
        };
        doc->setGlobalVariables(registerMap);

        T_DeployPanel *deployPanel = new T_DeployPanel(nullptr, doc, extracDir);
        connect(deployPanel, &T_DeployPanel::deployFinished, this, [this, gArea, doc, extracDir, deployPanel, drawerArea, signResult](){
            QDir CFGDir(QDir::cleanPath(gSettings->getCFGPath() + "/" + doc->metaValue("Target")));
            if(CFGDir.exists()){
                CFGDir.removeRecursively();
            }
            QDir().mkpath(CFGDir.absolutePath());
            copyFolder(extracDir, CFGDir.absolutePath(), true);
            QDir().rmdir(extracDir);

            QFile AsulConfig(QDir(CFGDir.absolutePath()).filePath("config.asul"));
            if(AsulConfig.exists() && AsulConfig.open(QIODevice::ReadWrite | QIODevice::Truncate)){
                AsulConfig.write(doc->dump().toUtf8());
                AsulConfig.close();
            }

            for(auto &CFGExport : doc->toCFGs()){
                QFile cfgFile(CFGDir.filePath(CFGExport.fileName));
                if(!cfgFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){
                    continue;
                }
                QTextStream out(&cfgFile);
                out << tr("// ====== 生成的文件 =======") + "\n";
                out << tr("//=这个 %1 文件由 Asul-CFGManager(AM) 根据配置自动生成 ").arg(CFGExport.fileName) + "\n";
                out << tr("\n//==这个 配置文件 从哪儿来的?") + "\n";
                out << tr("//CFG 制作者: ") << CFGExport.sourceFormId << "\n";
                out << tr("//CFG 名称: ") << CFGExport.description << "\n";
                out << tr("//CFG 版本: ") << doc->metaValue("Version") << "\n";
                out << tr("//==CFG 详细 结束") + "\n\n";
                out << CFGExport.content << "\r\n";
                out << "\r\n" + tr("//==参数结束") + "\n";
                out << tr("//AM 是由 Alivn开发的部署 CS2 CFG 的程序,旨在为CFG制作者提供更方便的分发服务 以及 使用者提供方便的配置服务") + "\n";
                out << tr("//开发者:Github(https://github.com/AsulTop),网站(http://www.asul.top)") + "\n";
                out << tr("//配置时间: ") << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "\n\n";
            }

            if(gSets->getOpenFolderAfterDeploy()){
                QDesktopServices::openUrl(QUrl("file:///" + CFGDir.absolutePath()));
            }
            if(gSets->getExecuteProgramAfterDeploy() && !doc->metaValue("Exec").isEmpty()){
                QString execRelativePath = doc->metaValue("Exec");
                QString cleanExecPath = QDir::cleanPath(CFGDir.absoluteFilePath(execRelativePath));
                if(!cleanExecPath.contains(gSettings->getCFGPath())){
                    gFunc->showErr(tr("运行"), tr("可执行文件路径不在允许的目录下"));
                }else if(!QFile::exists(cleanExecPath)){
                    gFunc->showErr(tr("运行"), tr("可执行文件不存在") + " " + cleanExecPath);
                }else{
                    if(signResult.trusted){
                        QProcess::startDetached("cmd", QStringList() << "/c" << "start" << "" << cleanExecPath, CFGDir.absolutePath());
                    }else{
                        bool askRec = GlobalFunc::askDialog(this, tr("运行"),
                            tr("此配置未受信任，是否要运行其可执行文件？\n\n") + cleanExecPath +
                            tr("\n\n警告：运行未受信任的程序可能存在安全风险"));
                        if(askRec){
                            QProcess::startDetached("cmd", QStringList() << "/c" << "start" << "" << cleanExecPath, CFGDir.absolutePath());
                        }
                    }
                }
            }

            drawerArea->removeDrawer(gArea);
            deployPanel->deleteLater();
            delete gArea;
        });
        deployPanel->show();
    });

    return true;
}

void T_Deploy::handleManifestEntry(const QJsonObject &entry, ElaDrawerArea *drawerArea,
                                   const QMap<QString, SteamUserInfo> &userInfoMap,
                                   ElaComboBox *selectAccountComboBox)
{
    QString name = entry.value("name").toString();
    QString version = entry.value("version").toString();
    QString author = entry.value("author").toString();
    QString description = entry.value("description").toString();
    QString downloadUrl = entry.value("download_url").toString();
    QString pictureUrl = entry.value("picture_url").toString();

    if(name.isEmpty() || downloadUrl.isEmpty()){
        qDebug() << "[T_Deploy] Skipping manifest entry with missing name or download_url";
        return;
    }

    QString displayName = name + " (" + version + ")";

    ElaPushButton *getButton = new ElaPushButton(tr("获取"), this);
    getButton->setFixedWidth(80);
    ElaProgressRing *entryRing = new ElaProgressRing(this);
    entryRing->setFixedSize(36, 36);
    entryRing->setIsBusying(false);
    entryRing->setIsDisplayValue(false);
    entryRing->setMinimum(0);
    entryRing->setMaximum(100);
    entryRing->setValue(0);
    entryRing->setBusyingWidth(4);
    entryRing->hide();

    QWidget *buttonWidget = new QWidget(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch();
    buttonLayout->addWidget(getButton);
    buttonLayout->addWidget(entryRing);

    ElaScrollPageArea *gArea;
    if(!pictureUrl.isEmpty()){
        gArea = gFunc->GenerateArea(this, QString(":/Pictures/Pictures/CS2.png"),
            new ElaText(displayName, this),
            new ElaText(author + " - " + description, this),
            buttonWidget, false);

        // 异步下载图片
        QString picPath = gSets->getGLoc()->path() + "/pic_" + name + ".png";
        AsulMultiDownloader *picDownloader = new AsulMultiDownloader(this);
        picDownloader->setMaxConcurrentDownloads(1);
        picDownloader->addDownload(QUrl(pictureUrl), picPath);
        connect(picDownloader, &AsulMultiDownloader::downloadFinished, this,
            [picDownloader, picPath](const QString &taskId, const QString &savePath){
                Q_UNUSED(taskId);
                Q_UNUSED(savePath);
                picDownloader->deleteLater();
            });
        connect(picDownloader, &AsulMultiDownloader::downloadFailed, this,
            [picDownloader](const QString &taskId, const QString &errorString){
                Q_UNUSED(taskId);
                Q_UNUSED(errorString);
                picDownloader->deleteLater();
            });
    }else{
        gArea = gFunc->GenerateArea(this, QString(":/Pictures/Pictures/CS2.png"),
            new ElaText(displayName, this),
            new ElaText(author + " - " + description, this),
            buttonWidget, false);
    }

    drawerArea->addDrawer(gArea);

    connect(getButton, &ElaPushButton::clicked, this,
        [this, getButton, entryRing, gArea, drawerArea, downloadUrl, userInfoMap, selectAccountComboBox]()
    {
        QUrl qurl(downloadUrl);
        if(!qurl.isValid()){
            GlobalFunc::showErr(tr("获取"), tr("无效的下载地址"));
            return;
        }

        getButton->hide();
        entryRing->setIsBusying(true);
        entryRing->setIsDisplayValue(false);
        entryRing->show();

        QString downloadDir = gSets->getGLoc()->path() + "/Downloads";
        QDir().mkpath(downloadDir);
        QString fileName = qurl.fileName().isEmpty() ? "package.zip" : qurl.fileName();
        QString destPath = downloadDir + "/" + fileName;

        AsulMultiDownloader *downloader = new AsulMultiDownloader(this);
        downloader->setMaxConcurrentDownloads(1);
        downloader->setAutoRetry(true);
        downloader->setMaxRetryCount(3);
        downloader->addDownload(qurl, destPath);

        connect(downloader, &AsulMultiDownloader::downloadProgress, this,
            [entryRing](const QString &taskId, qint64 received, qint64 total){
                Q_UNUSED(taskId);
                if(total > 0){
                    entryRing->setIsBusying(false);
                    entryRing->setRange(0, static_cast<int>(total));
                    entryRing->setValue(static_cast<int>(received));
                }
            });

        connect(downloader, &AsulMultiDownloader::downloadFinished, this,
            [this, downloader, destPath, gArea, drawerArea, getButton, entryRing, userInfoMap, selectAccountComboBox]
            (const QString &taskId, const QString &savePath)
        {
            Q_UNUSED(taskId);
            Q_UNUSED(savePath);

            entryRing->setIsBusying(true);
            entryRing->setIsDisplayValue(false);

            QString rootCaPath(":/certs/RootCA-Cert/root-ca.crt.pem");
            VerifyFile::SimpleVerifyResult result = VerifyFile::VerifyFileSdk::verifyFileSimple(destPath, rootCaPath);

            if(!result.trusted){
                bool userAccept = GlobalFunc::askDialog(this, tr("签名：未受信任"),
                    tr("此包签名验证未通过，可能存在安全风险。\n\n是否继续？"));
                if(!userAccept){
                    QFile::remove(destPath);
                    entryRing->hide();
                    entryRing->setIsBusying(false);
                    getButton->show();
                    downloader->deleteLater();
                    return;
                }
            }else{
                GlobalFunc::showSuccess(tr("签名"), tr("受信任开发者：") + result.info.signer);
            }

            QString extracDir = gSets->getGLoc()->path() + "/Downloads/" + QFileInfo(destPath).baseName() + "-extract";
            gFunc->UnzipFileAsync(destPath, extracDir,
                [this, destPath, extracDir, result, gArea, drawerArea, getButton, entryRing, downloader, userInfoMap, selectAccountComboBox](bool success)
            {
                QFile::remove(destPath);
                entryRing->hide();
                entryRing->setIsBusying(false);

                if(!success){
                    GlobalFunc::showErr(tr("获取"), tr("解压失败"));
                    QDir(extracDir).removeRecursively();
                    getButton->show();
                    downloader->deleteLater();
                    return;
                }

                handleExtractedPackage(extracDir, tr("获取"), result, drawerArea, userInfoMap, selectAccountComboBox);
                drawerArea->removeDrawer(gArea);
                delete gArea;
                GlobalFunc::showSuccess(tr("获取"), tr("包校验完成，已就绪"));
                downloader->deleteLater();
            });
        });

        connect(downloader, &AsulMultiDownloader::downloadFailed, this,
            [downloader, getButton, entryRing](const QString &taskId, const QString &errorString){
                Q_UNUSED(taskId);
                entryRing->hide();
                entryRing->setIsBusying(false);
                getButton->show();
                GlobalFunc::showErr(tr("下载"), tr("下载失败: ") + errorString);
                downloader->deleteLater();
            });
    });
}
