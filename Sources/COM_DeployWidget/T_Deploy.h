#ifndef T_Deploy_H
#define T_Deploy_H

#include <QWidget>
#include <qtmetamacros.h>
#include "../SystemKit/BaseScrollPage.h"
#include "../CTL_AsulComboBox/AsulComboBox.h"
class T_Deploy : public BaseScrollPage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_Deploy(QWidget *parent = nullptr);
    
signals:

};

#endif // T_Deploy_H
