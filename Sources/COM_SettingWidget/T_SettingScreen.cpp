#include "T_SettingScreen.h"
#include "../Global/GlobalFunc.h"
#include "../Global/GlobalSettings.h"
#include <ElaToggleSwitch.h>
#include <qboxlayout.h>
#include <qpixmap.h>
#include <qstylehints.h>
#include "../SystemKit/AsulApplication.h"
#include "ElaDef.h"
#include "ElaApplication.h"

#include "ElaColorDialog.h"
#include "ElaPushButton.h"
#include "ElaDrawerArea.h"
#include "ElaScrollPageArea.h"
#include "ElaLineEdit.h"

T_SettingScreen::T_SettingScreen(QWidget *parent)
    : BaseScrollPage{parent}
{
    this->initWidget(tr("设置"),tr("设置"),tr(""));

    QVBoxLayout *centerVLayout=new QVBoxLayout(centralWidget);

    // --- 外观 DrawerArea ---
    ElaDrawerArea *appearanceDrawer = new ElaDrawerArea(this);
    QWidget *appearanceHeader = new QWidget(this);
    QHBoxLayout *appearanceHeaderLayout = new QHBoxLayout(appearanceHeader);
    appearanceHeaderLayout->setContentsMargins(0,0,0,0);
    ElaText *appearanceIcon = new ElaText(this);
    appearanceIcon->setTextPixelSize(15);
    appearanceIcon->setElaIcon(ElaIconType::Paintbrush);
    appearanceIcon->setFixedSize(25,25);
    ElaText *appearanceText = new ElaText(tr("外观"),this);
    appearanceText->setTextPixelSize(15);
    appearanceHeaderLayout->addWidget(appearanceIcon);
    appearanceHeaderLayout->addWidget(appearanceText);
    appearanceHeaderLayout->addStretch();
    appearanceDrawer->setDrawerHeader(appearanceHeader);

    ElaToggleSwitch *closeThemeColorSync=new ElaToggleSwitch(this);
    closeThemeColorSync->setIsToggled(gSets->getEnableThemeColorSyncWithSystem());
    connect(closeThemeColorSync,&ElaToggleSwitch::toggled,[=](bool checked){
        gSets->setEnableThemeColorSyncWithSystem(checked);
        if(checked) GlobalFunc::updateThemeUI();
    });
    appearanceDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("开启颜色同步"),this),new ElaText(tr("同步 Windows 的主题色"),this),closeThemeColorSync,false));

    ElaToggleSwitch *closeThemeModeSync=new ElaToggleSwitch(this);
    closeThemeModeSync->setIsToggled(gSets->getEnableThemeModeSyncWithSystem());
    connect(closeThemeModeSync,&ElaToggleSwitch::toggled,[=](bool checked){
        gSets->setEnableThemeModeSyncWithSystem(checked);
        if(checked){
            Qt::ColorScheme scheme = qApp->styleHints()->colorScheme();
            if(scheme == Qt::ColorScheme::Dark){
                eTheme->setThemeMode(ElaThemeType::Dark);
            }else eTheme->setThemeMode(ElaThemeType::Light);
        }
    });
    appearanceDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("开启明暗同步"),this),new ElaText(tr("同步 Windows 的明暗模式"),this),closeThemeModeSync,false));

    AsulComboBox *SwitchDisplayMode=new AsulComboBox(this);
    SwitchDisplayMode->addItems(QString{"Normal ElaMica Mica MicaAlt Acrylic DWMBlur"}.split(" "));
    SwitchDisplayMode->setCurrentText("Acrylic");
    eApp->setWindowDisplayMode(ElaApplicationType::Acrylic);
    connect(SwitchDisplayMode,&AsulComboBox::currentIndexChanged,[=](int index){
        switch (index){
            case 0: eApp->setWindowDisplayMode(ElaApplicationType::Normal); break;
            case 1: eApp->setWindowDisplayMode(ElaApplicationType::ElaMica); break;
            case 2: eApp->setWindowDisplayMode(ElaApplicationType::Mica); break;
            case 3: eApp->setWindowDisplayMode(ElaApplicationType::MicaAlt); break;
            case 4: eApp->setWindowDisplayMode(ElaApplicationType::Acrylic); break;
            case 5: eApp->setWindowDisplayMode(ElaApplicationType::DWMBlur); break;
        }
    });
    appearanceDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("显示模式"),this),new ElaText(tr("切换窗口显示模式"),this),SwitchDisplayMode,false));

    AsulComboBox * changeThemeMode = new AsulComboBox(this);
    changeThemeMode->addItems(QString{"Light Dark"}.split(" "));
    changeThemeMode->setCurrentText(eTheme->getThemeMode()==ElaThemeType::Dark?"Dark":"Light");
    connect(changeThemeMode,&AsulComboBox::currentIndexChanged,[=](int index){
        if(index==0){
            eTheme->setThemeMode(ElaThemeType::Light);
        }else eTheme->setThemeMode(ElaThemeType::Dark);
    });
    appearanceDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("切换主题"),this),new ElaText(tr("切换 Dark/Light"),this),changeThemeMode,false));

    ElaPushButton * ColorPickBtn=new ElaPushButton(this);
    ElaColorDialog * colorDialog=new ElaColorDialog(this);
    GlobalFunc::addThemeSyncList(ColorPickBtn);
    ColorPickBtn->setText(tr("选择颜色"));
    colorDialog->setWindowTitle(" ");
    connect(ColorPickBtn,&ElaPushButton::clicked,[=](){
        colorDialog->exec();
    });
    connect(colorDialog, &ElaColorDialog::colorSelected, this, [=](const QColor& color) {
        GlobalFunc::updateThemeUI(color);
    });
    appearanceDrawer->addDrawer(GlobalFunc::GenerateArea(this,ElaIconType::Display,new ElaText(tr("选择颜色"),this),new ElaText(tr("选择自定义颜色"),this),ColorPickBtn,false));

    appearanceDrawer->expand();
    centerVLayout->addWidget(appearanceDrawer);

    // --- 语言 (独立) ---
    AsulComboBox * switchLanguage = new AsulComboBox(this);
    switchLanguage->addItems(gSets->getSupportedLang());
    switchLanguage->setCurrentText(gSets->getRegisterSettings()->value("lang").toString());
    connect(switchLanguage,&AsulComboBox::currentTextChanged,[=](QString val){
        gSets->getRegisterSettings()->setValue("lang",val);
        qApp->installTranslator(gSets->translators[val]);
        if(GlobalFunc::askDialog(this,tr("语言"),tr("是否重启以更换语言"))){
            qApp->quit();
            QProcess::startDetached(qApp->applicationFilePath(), QStringList());
        }else{GlobalFunc::showInfo(tr("语言"),tr("重启以更换语言"),0x3f3f3f3f);}
    });
    centerVLayout->addWidget(GlobalFunc::GenerateArea(this,ElaIconType::Globe,new ElaText(tr("切换语言"),this),new ElaText(tr("切换语言"),this),switchLanguage,false));

    // --- 部署 DrawerArea ---
    ElaDrawerArea *deployDrawer = new ElaDrawerArea(this);
    QWidget *deployHeader = new QWidget(this);
    QHBoxLayout *deployHeaderLayout = new QHBoxLayout(deployHeader);
    deployHeaderLayout->setContentsMargins(0,0,0,0);
    ElaText *deployIcon = new ElaText(this);
    deployIcon->setTextPixelSize(15);
    deployIcon->setElaIcon(ElaIconType::Download);
    deployIcon->setFixedSize(25,25);
    ElaText *deployText = new ElaText(tr("部署"),this);
    deployText->setTextPixelSize(15);
    deployHeaderLayout->addWidget(deployIcon);
    deployHeaderLayout->addWidget(deployText);
    deployHeaderLayout->addStretch();
    deployDrawer->setDrawerHeader(deployHeader);

    ElaToggleSwitch *openFolderAfterDeploy=new ElaToggleSwitch(this);
    openFolderAfterDeploy->setIsToggled(gSets->getOpenFolderAfterDeploy());
    connect(openFolderAfterDeploy,&ElaToggleSwitch::toggled,[=](bool checked){
        gSets->setOpenFolderAfterDeploy(checked);
    });
    deployDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("部署后打开文件夹"),this),new ElaText(tr("部署完成后打开 CFG 目录"),this),openFolderAfterDeploy,false));

    ElaToggleSwitch *executeProgramAfterDeploy=new ElaToggleSwitch(this);
    executeProgramAfterDeploy->setIsToggled(gSets->getExecuteProgramAfterDeploy());
    connect(executeProgramAfterDeploy,&ElaToggleSwitch::toggled,[=](bool checked){
        gSets->setExecuteProgramAfterDeploy(checked);
    });
    deployDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("部署后执行程序"),this),new ElaText(tr("部署完成后运行配置中的可执行文件"),this),executeProgramAfterDeploy,false));

    ElaLineEdit *customSourceURLEdit = new ElaLineEdit(this);
    customSourceURLEdit->setText(gSets->getCustomSourceURL());
    customSourceURLEdit->setFixedWidth(400);
    connect(customSourceURLEdit, &ElaLineEdit::editingFinished, [=](){
        gSets->setCustomSourceURL(customSourceURLEdit->text().trimmed());
    });
    deployDrawer->addDrawer(GlobalFunc::GenerateArea(this,new ElaText(tr("自定义源地址"),this),new ElaText(tr("在线商店自定义配置包来源 URL"),this),customSourceURLEdit,false));

    deployDrawer->expand();
    centerVLayout->addWidget(deployDrawer);

    centerVLayout->addStretch();
}
