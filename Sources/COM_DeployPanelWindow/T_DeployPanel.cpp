#include "T_DeployPanel.h"
#include <qapplication.h>
#include <qboxlayout.h>
#include <windows.h>
#include <QThreadPool>
#include <QTimer>
#include <QDebug>
#include "ElaDef.h"
#include "ElaComboBox.h"
#include "../Global/GlobalSettings.h"
#include "../Global/GlobalFunc.h"
#include "../SystemKit/BaseInclude.h"
#include "ElaDrawerArea.h"
#include "ElaKeyBinder.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include <QApplication>

#define DBG(x) qDebug() << "[T_DeployPanel] Line<" <<__LINE__<< ">: "<<x;

T_DeployPanel::T_DeployPanel(QWidget *parent,const AFormParser::Document::Ptr& doc,QString tempCFGLocation)
    : ElaWindow{nullptr}
    , m_visibilityUpdatePending(false)
{
    DBG("Constructor started");
    m_doc = doc;
    DBG(doc.get());
    DBG("tempCFGLocation:" << tempCFGLocation);
    this->setUserInfoCardPixmap(QPixmap(QString(":/pic/Pic/favicon.png").replace("favicon.png",eTheme->getThemeMode()==ElaThemeType::Light?"favicon_dark.png":"favicon.png")));
    this->setNavigationBarDisplayMode(ElaNavigationType::Minimal);
    this->setFixedSize(QSize(1000,600));
    this->setIsNavigationBarEnable(false);
    this->setWindowButtonFlags(ElaAppBarType::CloseButtonHint | ElaAppBarType::MaximizeButtonHint | ElaAppBarType::MinimizeButtonHint);
    this->setIsDefaultClosed(false);
    
    QString CFGName=""
            ,CFGVersion=""
            ,CFGAuthor=""
            ,CFGPNGFilePath=""
            ,CFGDescription="";
    
    DBG(" - iterate metaEntries");
    for(auto &[key,value]:doc->metaEntries()){
        DBG("meta:" << key << "=" << value);
        if(key=="Name"){
            CFGName=value;
        }
        if(key=="Version"){
            CFGVersion=value;
        }
        if(key=="Author"){
            CFGAuthor=value;
        }
        if(key=="Picture"){
            CFGPNGFilePath=value;
        }
        if(key=="Description"){
            CFGDescription=value;
        }
    }
    
    DBG(" - connect closeButtonClicked");
    connect(this,&ElaWindow::closeButtonClicked,[=](){
        bool ask=gFunc->askDialog(this,tr("关闭"),tr("是否退出配置")+" **"+CFGName.simplified()+" ** ["+CFGVersion+"]-"+CFGAuthor+" ?");
        if (ask) this->close();
    });
    
    DBG(" - CFGPNGFilePath:" << CFGPNGFilePath);
    if(!CFGPNGFilePath.isEmpty()){
        DBG(" - setUserInfoCardPixmap from file");
        this->setUserInfoCardPixmap(QPixmap(tempCFGLocation+"/"+CFGPNGFilePath));
    }else {
        DBG(" - setUserInfoCardPixmap from resource");
        this->setUserInfoCardPixmap(QPixmap(":/pic/Pic/CS2.png"));
    }
    
    DBG(" - setUserInfoCardTitle");
    this->setUserInfoCardTitle(CFGName);
    DBG(" - setUserInfoCardSubTitle");
    this->setUserInfoCardSubTitle("@"+CFGAuthor);
    DBG(" - setUserInfoCardVisible");    
    this->setUserInfoCardVisible(true);
    DBG(" - setWindowTitle");
    this->setWindowTitle(tr("部署 CFG"));
    this->setProperty("ElaBaseClassName","FakeElaWindow");

    DBG(" - create BaseScrollPage");
    BaseScrollPage *CFGSettingPage=new BaseScrollPage(this);
    DBG(" - CFGSettingPage->initWidget");
    CFGSettingPage->initWidget(CFGName,CFGName+tr(" 项目"),CFGDescription);
    DBG(" - CFGSettingPage->setWindowTitle");   
    CFGSettingPage->setWindowTitle(CFGName+tr(" 配置"));
    DBG(" - create centerVLayout");
    QVBoxLayout *centerVLayout=new QVBoxLayout(CFGSettingPage->centralWidget);
    centerVLayout->setContentsMargins(20, 30, 20, 30);
    centerVLayout->setSpacing(20);
    
    QMap<QString, QWidget*> fieldWidgetMap;
    QMap<QString, ElaScrollPageArea*> fieldAreaMap;

    DBG(" - addPageNode");
    this->addPageNode("安装",CFGSettingPage,ElaIconType::GearComplex);

    for(const auto &form: doc->forms){
        QApplication::processEvents();
        for(const auto &group : form->groups){
            QApplication::processEvents();
            
            ElaText *title = new ElaText(group->title, CFGSettingPage);
            QFont font = title->font();
            font.setBold(true);
            font.setPointSize(16);
            title->setFont(font);
            centerVLayout->addWidget(title);
            
            for(const auto &field : group->fields){
                QApplication::processEvents();
                DBG(" - addFieldWidget");
                
                if(auto keyBind=field->toKeyBind()){
                    DBG(" - addKeyBindWidget");
                    
                    ElaScrollPageArea *area = new ElaScrollPageArea(CFGSettingPage);
                    
                    QWidget *contentWidget = new QWidget(area);
                    QHBoxLayout *areaLayout = new QHBoxLayout(contentWidget);
                    areaLayout->setContentsMargins(0, 0, 0, 0);
                    areaLayout->setSpacing(15);
                    
                    QVBoxLayout *textLayout = new QVBoxLayout();
                    textLayout->setSpacing(5);
                    textLayout->setContentsMargins(20, 15, 0, 15);
                    ElaText *descText = new ElaText(keyBind->description, contentWidget);
                    QFont descFont = descText->font();
                    descFont.setBold(true);
                    descText->setFont(descFont);
                    descText->setTextPixelSize(14);
                    
                    ElaText *subDescText = new ElaText(keyBind->subDescription, contentWidget);
                    subDescText->setTextPixelSize(12);
                    subDescText->setStyleSheet("color: gray;");
                    
                    textLayout->addWidget(descText);
                    textLayout->addWidget(subDescText);
                    textLayout->addStretch();
                    
                    QHBoxLayout *widgetLayout = new QHBoxLayout();
                    widgetLayout->setSpacing(10);
                    widgetLayout->setContentsMargins(0, 15, 20, 15);
                    
                    ElaLineEdit *binderLE = new ElaLineEdit(contentWidget);
                    binderLE->setFixedSize(150, 40);
                    binderLE->setText(keyBind->bind);
                    
                    ElaKeyBinder *binderKB = new ElaKeyBinder(contentWidget);
                    binderKB->setFixedSize(150, 40);
                    binderKB->setBinderKeyText(keyBind->bind);
                    
                    connect(binderKB,&ElaKeyBinder::binderKeyTextChanged,[=](QString key){
                        DBG(" - binderKeyTextChanged:" << key);
                        QString val = key.trimmed();
                        if(val=="Up") val="uparrow";
                        else if(val=="Down") val="downarrow";
                        else if(val=="Left") val="leftarrow";
                        else if(val=="Right") val="rightarrow";
                        else if(val=="Return") val="enter";
                        else if(val=="PgDown") val="pgdn";
                        val=val.toLower();
                        keyBind->bind=val;
                        binderLE->setText(val);
                    });
                    
                    connect(binderLE,&ElaLineEdit::editingFinished,[=](){
                        binderKB->setBinderKeyText(binderLE->text());
                        keyBind->bind=binderLE->text();
                        scheduleVisibilityUpdate();
                    });
                    
                    widgetLayout->addWidget(binderLE);
                    widgetLayout->addWidget(binderKB);
                    
                    areaLayout->addLayout(textLayout, 1);
                    areaLayout->addStretch();
                    areaLayout->addLayout(widgetLayout);
                    
                    QHBoxLayout *mainAreaLayout = new QHBoxLayout(area);
                    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
                    mainAreaLayout->addWidget(contentWidget);
                    
                    area->setFixedHeight(80);
                    
                    centerVLayout->addWidget(area);
                    m_fieldAreaMap[keyBind->id] = area;
                    area->setVisible(doc->evaluateFieldEnabled(keyBind->id));
                    DBG(" - addKeyBindWidget done");
                }
                else if(auto MustField=field->toMustField()){
                    DBG(" - addMustFieldWidget");
                    
                    ElaScrollPageArea *area = new ElaScrollPageArea(CFGSettingPage);
                    
                    QWidget *contentWidget = new QWidget(area);
                    QHBoxLayout *areaLayout = new QHBoxLayout(contentWidget);
                    areaLayout->setContentsMargins(0, 0, 0, 0);
                    areaLayout->setSpacing(15);
                    
                    QVBoxLayout *textLayout = new QVBoxLayout();
                    textLayout->setSpacing(5);
                    textLayout->setContentsMargins(20, 15, 0, 15);
                    ElaText *descText = new ElaText(MustField->description, contentWidget);
                    QFont descFont = descText->font();
                    descFont.setBold(true);
                    descText->setFont(descFont);
                    descText->setTextPixelSize(14);
                    
                    ElaText *subDescText = new ElaText(MustField->subDescription, contentWidget);
                    subDescText->setTextPixelSize(12);
                    subDescText->setStyleSheet("color: gray;");
                    
                    textLayout->addWidget(descText);
                    textLayout->addWidget(subDescText);
                    textLayout->addStretch();
                    
                    ElaLineEdit *mustFieldLE = new ElaLineEdit(contentWidget);
                    mustFieldLE->setFixedSize(250, 40);
                    mustFieldLE->setText(MustField->bind);
                    mustFieldLE->setReadOnly(true);
                    mustFieldLE->setStyleSheet("background-color: #f0f0f0;");
                    
                    QHBoxLayout *widgetLayout = new QHBoxLayout();
                    widgetLayout->setContentsMargins(0, 15, 20, 15);
                    widgetLayout->addStretch();
                    widgetLayout->addWidget(mustFieldLE);
                    
                    areaLayout->addLayout(textLayout, 1);
                    areaLayout->addLayout(widgetLayout);
                    
                    QHBoxLayout *mainAreaLayout = new QHBoxLayout(area);
                    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
                    mainAreaLayout->addWidget(contentWidget);
                    
                    area->setFixedHeight(80);
                    
                    centerVLayout->addWidget(area);
                    m_fieldAreaMap[MustField->id] = area;
                    area->setVisible(doc->evaluateFieldEnabled(MustField->id));
                    DBG(" - addMustFieldWidget done");
                }
                else if(auto TextField=field->toTextField()){
                    DBG(" - addTextFieldWidget");
                    
                    ElaScrollPageArea *area = new ElaScrollPageArea(CFGSettingPage);
                    
                    QWidget *contentWidget = new QWidget(area);
                    QHBoxLayout *areaLayout = new QHBoxLayout(contentWidget);
                    areaLayout->setContentsMargins(0, 0, 0, 0);
                    areaLayout->setSpacing(15);
                    
                    QVBoxLayout *textLayout = new QVBoxLayout();
                    textLayout->setSpacing(5);
                    textLayout->setContentsMargins(20, 15, 0, 15);
                    ElaText *descText = new ElaText(TextField->description, contentWidget);
                    QFont descFont = descText->font();
                    descFont.setBold(true);
                    descText->setFont(descFont);
                    descText->setTextPixelSize(14);
                    
                    ElaText *subDescText = new ElaText(TextField->subDescription, contentWidget);
                    subDescText->setTextPixelSize(12);
                    subDescText->setStyleSheet("color: gray;");
                    
                    textLayout->addWidget(descText);
                    textLayout->addWidget(subDescText);
                    textLayout->addStretch();
                    
                    ElaText *fieldText = new ElaText(TextField->text, contentWidget);
                    fieldText->setTextPixelSize(14);
                    
                    QHBoxLayout *widgetLayout = new QHBoxLayout();
                    widgetLayout->setContentsMargins(0, 15, 20, 15);
                    widgetLayout->addStretch();
                    widgetLayout->addWidget(fieldText);
                    
                    areaLayout->addLayout(textLayout, 1);
                    areaLayout->addLayout(widgetLayout);
                    
                    QHBoxLayout *mainAreaLayout = new QHBoxLayout(area);
                    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
                    mainAreaLayout->addWidget(contentWidget);
                    
                    area->setFixedHeight(80);
                    
                    centerVLayout->addWidget(area);
                    m_fieldAreaMap[TextField->id] = area;
                    area->setVisible(doc->evaluateFieldEnabled(TextField->id));
                    DBG(" - addTextFieldWidget done");
                }
                else if(auto LineField=field->toLineField()){
                    DBG(" - addLineFieldWidget");
                    
                    ElaScrollPageArea *area = new ElaScrollPageArea(CFGSettingPage);
                    
                    // 创建一个中间 widget 来容纳内容
                    QWidget *contentWidget = new QWidget(area);
                    QHBoxLayout *areaLayout = new QHBoxLayout(contentWidget);
                    areaLayout->setContentsMargins(0, 0, 0, 0);
                    areaLayout->setSpacing(15);
                    
                    // 左侧：Description 和 SubDescription（上下）
                    QVBoxLayout *textLayout = new QVBoxLayout();
                    textLayout->setSpacing(5);
                    textLayout->setContentsMargins(20, 15, 0, 15);
                    ElaText *descText = new ElaText(LineField->description, contentWidget);
                    QFont descFont = descText->font();
                    descFont.setBold(true);
                    descText->setFont(descFont);
                    descText->setTextPixelSize(14);
                    
                    ElaText *subDescText = new ElaText(LineField->subDescription, contentWidget);
                    subDescText->setTextPixelSize(12);
                    subDescText->setStyleSheet("color: gray;");
                    
                    textLayout->addWidget(descText);
                    textLayout->addWidget(subDescText);
                    textLayout->addStretch();
                    
                    // 右侧：Arg 的内容（垂直排列）
                    QVBoxLayout *argsLayout = new QVBoxLayout();
                    argsLayout->setSpacing(10);
                    argsLayout->setContentsMargins(0, 15, 20, 15);
                    
                    for(auto &arg: LineField->args){
                        QHBoxLayout *argLayout = new QHBoxLayout();
                        argLayout->setSpacing(10);
                        
                        ElaText *argDesc = new ElaText(arg->description, contentWidget);
                        argDesc->setTextPixelSize(13);
                        
                        ElaLineEdit *argLine = new ElaLineEdit(contentWidget);
                        argLine->setText(arg->value);
                        argLine->setFixedHeight(40);
                        argLine->setMinimumWidth(200);
                        
                        connect(argLine,&ElaLineEdit::editingFinished,[=](){
                            arg->value=argLine->text();
                            scheduleVisibilityUpdate();
                        });
                        
                        argLayout->addWidget(argDesc, 3);
                        argLayout->addWidget(argLine, 7);
                        argsLayout->addLayout(argLayout);
                    }
                    
                    areaLayout->addLayout(textLayout, 3);
                    areaLayout->addLayout(argsLayout, 7);
                    
                    // 把 contentWidget 设置到 area
                    QHBoxLayout *mainAreaLayout = new QHBoxLayout(area);
                    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
                    mainAreaLayout->setSpacing(0);
                    contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    mainAreaLayout->addWidget(contentWidget);

                    // 设置最小高度
                    int minHeight = 80 + LineField->args.size() * 55;
                    area->setFixedHeight(minHeight);
                    
                    centerVLayout->addWidget(area);
                    m_fieldAreaMap[LineField->id] = area;
                    area->setVisible(doc->evaluateFieldEnabled(LineField->id));
                    DBG(" - addLineFieldWidget done");
                }
                else if(auto OptionField=field->toOptionField()){
                    DBG(" - addOptionFieldWidget");
                    
                    ElaScrollPageArea *area = new ElaScrollPageArea(CFGSettingPage);
                    
                    QWidget *contentWidget = new QWidget(area);
                    QHBoxLayout *areaLayout = new QHBoxLayout(contentWidget);
                    areaLayout->setContentsMargins(0, 0, 0, 0);
                    areaLayout->setSpacing(15);
                    
                    QVBoxLayout *textLayout = new QVBoxLayout();
                    textLayout->setSpacing(5);
                    textLayout->setContentsMargins(20, 15, 0, 15);
                    ElaText *descText = new ElaText(OptionField->description, contentWidget);
                    QFont descFont = descText->font();
                    descFont.setBold(true);
                    descText->setFont(descFont);
                    descText->setTextPixelSize(14);
                    
                    ElaText *subDescText = new ElaText(OptionField->subDescription, contentWidget);
                    subDescText->setTextPixelSize(12);
                    subDescText->setStyleSheet("color: gray;");
                    
                    textLayout->addWidget(descText);
                    textLayout->addWidget(subDescText);
                    textLayout->addStretch();
                    
                    ElaComboBox *optionComboBox = new ElaComboBox(contentWidget);
                    optionComboBox->setFixedSize(280, 40);
                    
                    QMap<QString,QString> optionMap;
                    QMap<QString,QString> revertOptionMap;
                    for(auto option: OptionField->options){
                        optionMap[option->description]=option->id;
                        revertOptionMap[option->id]=option->description;
                    }
                    for(auto option: OptionField->options){
                        optionComboBox->addItem(option->description);
                    }
                    optionComboBox->setCurrentText(revertOptionMap[OptionField->selected]);
                    
                    connect(optionComboBox,&ElaComboBox::currentTextChanged,[=](QString text){
                        OptionField->selected=optionMap[text];
                        scheduleVisibilityUpdate();
                    });
                    
                    QHBoxLayout *widgetLayout = new QHBoxLayout();
                    widgetLayout->setContentsMargins(0, 15, 20, 15);
                    widgetLayout->addStretch();
                    widgetLayout->addWidget(optionComboBox);
                    
                    areaLayout->addLayout(textLayout, 1);
                    areaLayout->addLayout(widgetLayout);
                    
                    QHBoxLayout *mainAreaLayout = new QHBoxLayout(area);
                    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
                    mainAreaLayout->addWidget(contentWidget);
                    
                    area->setMinimumHeight(80);
                    
                    centerVLayout->addWidget(area);
                    m_fieldAreaMap[OptionField->id] = area;
                    area->setVisible(doc->evaluateFieldEnabled(OptionField->id));
                    DBG(" - addOptionFieldWidget done");
                }
            }
        }
    }
    
    centerVLayout->addStretch();
    
    ElaPushButton *deployBtn = new ElaPushButton("部署", CFGSettingPage);
    deployBtn->setFixedHeight(50);
    
    centerVLayout->addWidget(deployBtn);
    
    connect(deployBtn,&ElaPushButton::clicked,[=](){
        bool askRet= gFunc->askDialog(this,tr("安装"),tr("确认保存配置 ?"));
        if(!askRet){
            return;
        }
        emit deployFinished();
        this->hide();
    });
}

void T_DeployPanel::showEvent(QShowEvent *event) {
    DBG("showEvent called");
    ElaWindow::showEvent(event);
}

void T_DeployPanel::refreshAllFieldVisibility() {
    for (auto it = m_fieldAreaMap.begin(); it != m_fieldAreaMap.end(); ++it) {
        QString fieldId = it.key();
        ElaScrollPageArea* area = it.value();
        if (area) {
            bool newVisibility = m_doc->evaluateFieldEnabled(fieldId);
            if (newVisibility != m_fieldVisibilityCache.value(fieldId, !newVisibility)) {
                area->setVisible(newVisibility);
                m_fieldVisibilityCache[fieldId] = newVisibility;
            }
        }
    }
}

void T_DeployPanel::scheduleVisibilityUpdate() {
    if (m_visibilityUpdatePending) {
        return;
    }
    m_visibilityUpdatePending = true;
    QTimer::singleShot(0, this, [this]() {
        m_visibilityUpdatePending = false;
        refreshAllFieldVisibility();
    });
}

T_DeployPanel::~T_DeployPanel(){

}
