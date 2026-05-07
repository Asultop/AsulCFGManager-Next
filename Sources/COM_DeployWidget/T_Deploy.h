#ifndef T_Deploy_H
#define T_Deploy_H

#include <QWidget>
#include <QMap>
#include <QJsonObject>
#include <qtmetamacros.h>
#include "../SystemKit/BaseScrollPage.h"
#include "../CTL_AsulComboBox/AsulComboBox.h"
#include "VerifyFileSdk/VerifyFileSdk.h"
#include "../ToolKit/ASteamSDK/ASteamUserQuery/F_SteamUserQuery.h"

class ElaScrollPageArea;
class ElaDrawerArea;

class T_Deploy : public BaseScrollPage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_Deploy(QWidget *parent = nullptr);

private:
    QMap<QString, ElaScrollPageArea*> m_scannedAreaMap;
    void uninstallCFG(const QString& location);
    bool handleExtractedPackage(const QString &extracDir, const QString &title,
                                const VerifyFile::SimpleVerifyResult &signResult,
                                ElaDrawerArea *drawerArea,
                                const QMap<QString, SteamUserInfo> &userInfoMap,
                                ElaComboBox *selectAccountComboBox);
    void handleManifestEntry(const QJsonObject &entry, ElaDrawerArea *drawerArea,
                             const QMap<QString, SteamUserInfo> &userInfoMap,
                             ElaComboBox *selectAccountComboBox);

private slots:

};

#endif // T_Deploy_H
