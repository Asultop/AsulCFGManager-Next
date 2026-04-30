#include "T_Deploy.h"
#include "../Global/GlobalFunc.h"
#include "../Global/GlobalSettings.h"
#include "../SystemKit/AsulApplication.h"
#include "ElaDef.h"
#include "../../3rd/QsLog/QsLog.h"

#include "ElaComboBox.h"
#include "ElaContentDialog.h"
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
#include "VerifyFileSdk/VerifyFileSdk.h"
#include "AFormParser/AFormParser.hpp"
#include "../ToolKit/ASteamSDK/ASteamUserQuery/F_SteamUserQuery.h"
#include "../ToolKit/LoadingProgress/LoadingProgressDialog.h"
#include <QThread>

#include "../COM_DeployPanelWindow/T_DeployPanel.h"
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
            editButton->setFixedWidth(50);
            ElaScrollPageArea *gArea;
            QString displayName = doc->metaValue("Name") + " (" + doc->metaValue("Target") + ")";
            if(!doc->metaValue("Picture").isEmpty() && QFile(subDirPath+"/"+doc->metaValue("Picture")).exists()){
                gArea = gFunc->GenerateArea(
                    this,
                    QString(subDirPath+"/"+doc->metaValue("Picture")),
                    new ElaText(displayName,this),
                    new ElaText(doc->metaValue("Author"),this),
                    editButton,false
                );
            }else{
                gArea = gFunc->GenerateArea(
                    this,
                    QString(":/Pictures/Pictures/CS2.png"),
                    new ElaText(displayName,this),
                    new ElaText(doc->metaValue("Author"),this),
                    editButton,false
                );
            }
            localFileDrawerArea->addDrawer(gArea);
            m_scannedAreaMap[subDirPath] = gArea;
            qDebug() << "[T_Deploy] About to connect editButton clicked";
            connect(editButton,&ElaPushButton::clicked,this,[this,gArea,localFileDrawerArea,subDirPath,doc,userInfoMap,selectAccountComboBox]() -> void{
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
        gFunc->UnzipFile(destLocation,extracDir);
        QFile::remove(destLocation);
        loadingProgressDialog->setValue(25);
        QString AsulFormFilePath = extracDir + "/config.asul";
        QFile AsulFormFile(AsulFormFilePath);
        if(!AsulFormFile.exists()){
            GlobalFunc::showErr(tr("导入"),tr("文件不存在")+" "+AsulFormFilePath);
            qCritical()<<"导入失败: 文件不存在";
            loadingProgressDialog->close();
            delete loadingProgressDialog;
            return;
        }
        QString configFormFileContent;
        
        if(!AsulFormFile.open(QIODevice::ReadOnly)){
            GlobalFunc::showErr(tr("导入"),tr("文件打开失败")+" "+AsulFormFilePath);
            qDebug()<<"导入失败：文件打开失败";
            loadingProgressDialog->close();
            delete loadingProgressDialog;
            return;
        }
        configFormFileContent=AsulFormFile.readAll();

        // Verify using .sig if available
        bool enableCrlCheck=false;
        QStringList crlPemPaths;
        bool enableOcspCheck=false;
        QString ocspResponseDerPath;
        QFile sigFile(extracDir + "/config.sig");
        VerifyFile::SimpleVerifyResult result;
        if(sigFile.exists()){
            result = VerifyFile::VerifyFileSdk::verifyFileDetachedSimple(
                AsulFormFilePath,
                sigFile.fileName(),
                rootCaPath,
                enableCrlCheck,
                crlPemPaths,
                enableOcspCheck,
                ocspResponseDerPath
            );
        }else{
            result = VerifyFile::VerifyFileSdk::verifyFileSimple(
                filePath,
                rootCaPath,
                enableCrlCheck,
                crlPemPaths,
                enableOcspCheck,
                ocspResponseDerPath
            );
        }
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
        }
        else{
            bool askRec=GlobalFunc::askDialog(this,tr("签名：未受信任开发者签名"),tr("文件 ")+QFileInfo(filePath).fileName()+tr(" 可能存在风险，是否继续？"));
            if(!askRec){
                loadingProgressDialog->close();
                delete loadingProgressDialog;
                return;
            }
        }

        AFormParser::ParseError err;
        auto doc = AFormParser::Document::from(configFormFileContent,&err);
        if(!doc){
            GlobalFunc::showErr(tr("导入"),tr("解析失败")+err.message);
            qWarning()<<"解析失败："<<err.line<<"<"<<err.column<<">: "<<err.message;
            loadingProgressDialog->setValue(100);
            loadingProgressDialog->close();
            delete loadingProgressDialog;
            return;
        }
        qDebug() << "[T_Deploy] check arguments ";
        QStringList argList={"Version","Name","Target","Author","Description"};
        for(auto arg:argList){
            if(doc->metaValue(arg).isEmpty()){
                GlobalFunc::showErr(tr("导入"),tr("缺少参数")+" "+arg);
                qCritical()<< "[T_Deploy] Missing argument: "<<arg;
                loadingProgressDialog->close();
                delete loadingProgressDialog;
                return;
            }
        }
        qDebug() << "[T_Deploy] check argument validation";
        QString targetDir = QDir(gSets->getCFGPath()).filePath(doc->metaValue("Target"));
        QString cleanTarget = QDir::cleanPath(targetDir);
        if(!cleanTarget.contains(gSets->getCFGPath())){
            GlobalFunc::showErr(tr("导入"),tr("目标路径不在CFG目录下"));
            qCritical()<< "[T_Deploy] Target path not in CFG path: "<<doc->metaValue("Target");
            loadingProgressDialog->close();
            delete loadingProgressDialog;
            return;
        }
        qDebug() << "[T_Deploy] doc parsed successfully, forms count:" << doc->forms.size();
        qDebug() << "[T_Deploy] About to create T_DeployPanel with doc:" << doc.get();
        
        // deployPanel->show();
        ElaPushButton *installButton = new ElaPushButton(tr("安装"), this);
        installButton->setFixedWidth(50);
        ElaScrollPageArea *gArea;
        if(!doc->metaValue("Picture").isEmpty()&&QFile(extracDir+"/"+doc->metaValue("Picture")).exists()){
            gArea = gFunc->GenerateArea(
                this,
                QString(extracDir+"/"+doc->metaValue("Picture")),
                new ElaText(doc->metaValue("Name"),this),
                new ElaText(doc->metaValue("Author"),this),
                installButton,false
            );
        }else{
            gArea = gFunc->GenerateArea(
                this,
                QString(":/Pictures/Pictures/CS2.png"),
                new ElaText(doc->metaValue("Name"),this),
                new ElaText(doc->metaValue("Author"),this),
                installButton,false
            );
        }
        if(result.trusted){
            // installButton->setIcon(QIcon((":/Pictures/Pictures/verified.png")));
            // installButton->setStyleSheet(R"(
            //     ElaPushButton {
            //         text-align: left;   
            //         padding-left: 10px;  
            //     }
            // )");
            installButton->setText(installButton->text()+" "+tr(" (本地受信任)"));
            installButton->setLightDefaultColor(QColor(143, 204, 167));
            installButton->setDarkDefaultColor(QColor(39, 135, 95));
        }else installButton->setText(installButton->text() + " "+tr(" (本地)"));
        
        localFileDrawerArea->addDrawer(gArea);
        qDebug() << "[T_Deploy] About to connect installButton clicked";
        connect(installButton,&ElaPushButton::clicked,this,[this,result,gArea,localFileDrawerArea,extracDir,doc,userInfoMap,selectAccountComboBox]() -> void{
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
                // waiting to add more personal profile like keybinding
                // {"__Forward",currentUserInfo.forwardKey}, (...)
            };
            doc->setGlobalVariables(registerMap);
            
            qDebug() << "[T_Deploy] About to create T_DeployPanel with doc";
            T_DeployPanel * deployPanel = new T_DeployPanel(nullptr,doc,extracDir);
            qDebug() << "[T_Deploy] T_DeployPanel created";
            connect(deployPanel,&T_DeployPanel::deployFinished,this,[this,gArea,doc,extracDir,deployPanel,localFileDrawerArea,result](){
                // 准备实现toCFGs保存后复制配置到CFG文件目录
                QDir CFGDir(QDir::cleanPath(gSettings->getCFGPath()+"/"+doc->metaValue("Target")));
                if(CFGDir.exists()){
                    CFGDir.removeRecursively();
                }
                QDir().mkdir(CFGDir.absolutePath());
                copyFolder(extracDir,CFGDir.absolutePath(),true);
                // 复制完成，删除临时目录
                QDir().rmdir(extracDir);
                // 导出config.asul
                QFile AsulConfig(QDir(CFGDir.absolutePath()).filePath("config.asul"));
                if(!AsulConfig.exists()){
                    gFunc->showErr(tr("导出"),tr("文件不存在")+" "+AsulConfig.fileName());
                    qDebug() << "[T_Deploy] Failed to find config.asul file which should be found without thinking twice: "<<AsulConfig.fileName();
                    localFileDrawerArea->removeDrawer(gArea);
                    delete gArea;
                    return;
                }
                if(!AsulConfig.open(QIODevice::ReadWrite | QIODevice::Truncate)){
                    gFunc->showErr(tr("导出"),tr("文件打开失败:")+" "+AsulConfig.fileName());
                    qDebug() << "[T_Deploy] Failed to open config.asul : "<<AsulConfig.fileName();
                    localFileDrawerArea->removeDrawer(gArea);
                    delete gArea;
                    return;
                }
                if(!AsulConfig.write(doc->dump().toUtf8())){
                    gFunc->showErr(tr("导出"),tr("文件写入失败:")+" "+AsulConfig.fileName());
                    qDebug() << "[T_Deploy] Failed to write config.asul : "<<AsulConfig.fileName();
                    localFileDrawerArea->removeDrawer(gArea);
                    delete gArea;
                    return;
                }
                AsulConfig.close();
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
                        gFunc->showErr(tr("导出"),tr("文件打开失败:")+" "+cfgFile.fileName());
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
                        if(result.trusted){
                            QProcess::startDetached("cmd", QStringList() << "/c" << "start" << "" << cleanExecPath, CFGDir.absolutePath());
                        }else{
                            bool askRec = GlobalFunc::askDialog(this,tr("运行"),tr("此配置未受信任，是否要运行其可执行文件？\n\n")+cleanExecPath+tr("\n\n警告：运行未受信任的程序可能存在安全风险"));
                            if(askRec){
                                QProcess::startDetached("cmd", QStringList() << "/c" << "start" << "" << cleanExecPath, CFGDir.absolutePath());
                            }
                        }
                    }
                }

                localFileDrawerArea->removeDrawer(gArea);
                deployPanel->deleteLater();
                delete gArea;
                
            });
            qDebug() << "[T_Deploy] installButton clicked, about to show deployPanel";
            deployPanel->show();
            qDebug() << "[T_Deploy] deployPanel->show() called";
        });
        qDebug() << "[T_Deploy] connect done, about to close loadingProgressDialog";
        
        {
            loadingProgressDialog->close();
            localFileDrawerArea->expand();
            delete loadingProgressDialog;
        }
        
        qDebug() << "[T_Deploy] All setup finished";

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
