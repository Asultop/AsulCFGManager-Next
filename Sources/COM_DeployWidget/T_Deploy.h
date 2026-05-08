#ifndef T_Deploy_H
#define T_Deploy_H

#include <QWidget>
#include <QMap>
#include <QJsonObject>
#include <QUrl>
#include <qtmetamacros.h>
#include "../SystemKit/BaseScrollPage.h"
#include "../CTL_AsulComboBox/AsulComboBox.h"
#include "VerifyFileSdk/VerifyFileSdk.h"
#include "AFormParser/AFormParser.hpp"
#include "../ToolKit/ASteamSDK/ASteamUserQuery/F_SteamUserQuery.h"

class ElaScrollPageArea;
class ElaDrawerArea;
class ElaPushButton;
class ElaProgressRing;
class AsulMultiDownloader;

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

    ElaProgressRing* createProgressRing();
    AsulMultiDownloader* createDownloader(const QUrl &url, const QString &destPath);
    void downloadAndProcessPackage(const QUrl &url, const QString &title,
                                   ElaDrawerArea *drawerArea,
                                   const QMap<QString, SteamUserInfo> &userInfoMap,
                                   ElaComboBox *selectAccountComboBox,
                                   ElaPushButton *actionButton,
                                   ElaProgressRing *ring,
                                   ElaScrollPageArea *existingArea = nullptr);
    void writeDeployedConfigs(AFormParser::Document::Ptr doc, const QString &sourceDir,
                              const VerifyFile::SimpleVerifyResult &signResult,
                              ElaDrawerArea *drawerArea, ElaScrollPageArea *gArea);
    static QString resolveIconPath(const QString &basePath, const QString &pictureValue);

private slots:

};

#endif // T_Deploy_H
