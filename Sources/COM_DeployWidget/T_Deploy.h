#ifndef T_Deploy_H
#define T_Deploy_H

#include <QWidget>
#include <QMap>
#include <qtmetamacros.h>
#include "../SystemKit/BaseScrollPage.h"
#include "../CTL_AsulComboBox/AsulComboBox.h"

class ElaScrollPageArea;

class T_Deploy : public BaseScrollPage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_Deploy(QWidget *parent = nullptr);

private:
    QMap<QString, ElaScrollPageArea*> m_scannedAreaMap;
    void uninstallCFG(const QString& location);

private slots:

};

#endif // T_Deploy_H
