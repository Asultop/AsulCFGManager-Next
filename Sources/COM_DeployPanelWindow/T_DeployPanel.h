#ifndef T_DeployPanel_H
#define T_DeployPanel_H

#include <QObject>
#include <QWidget>
#include <QShowEvent>
#include <QMap>
#include "../SystemKit/BaseInclude.h"
#include "../SystemKit/BaseScrollpage.h"
#include "AFormParser/AFormParser.hpp"
#include "AFormParser/LuaRuntime.hpp"
#include <QTextBrowser>
#include <ElaWindow.h>

class T_DeployPanel : public ElaWindow
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_DeployPanel(QWidget* parent,const AFormParser::Document::Ptr& doc,QString tempCFGLocation);
    ~T_DeployPanel();

private:
    void refreshAllFieldVisibility();

protected:
    void showEvent(QShowEvent *event) override;

private:
    AFormParser::Document::Ptr m_doc;
    QMap<QString, QWidget*> m_fieldWidgetMap;
    QMap<QString, ElaScrollPageArea*> m_fieldAreaMap;
signals:
    void deployFinished();
};

#endif // T_DeployPanel_H
