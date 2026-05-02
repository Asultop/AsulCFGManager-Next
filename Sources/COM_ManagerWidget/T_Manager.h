#ifndef T_MANAGER_H
#define T_MANAGER_H

#include <QWidget>
#include <QMap>
#include <qtmetamacros.h>
#include "../SystemKit/BaseScrollPage.h"
#include "../CTL_AsulComboBox/AsulComboBox.h"
#include "../ToolKit/ASteamSDK/ASteamUserQuery/F_SteamUserQuery.h"

class ElaComboBox;
class ElaToggleSwitch;
class ElaScrollPageArea;

class T_Manager : public BaseScrollPage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit T_Manager(QWidget *parent = nullptr);

private:
    enum class Platform { Steam, WMPVP, EEEEE };

    void setupTopWidget();
    void setupSteamUI();
    void setupWMPVPUI();
    void setupEEEEEUI();
    void clearCurrentUI();
    void scanLocalCFGs();
    QString extractLaunchArgs(const QString& configAsulPath);
    QString getLocalConfigVDFPath();
    QStringList getCurrentLaunchOptions();
    bool checkIfArgEnabled(const QString& arg);
    void applySteamChanges();
    void applyWMPVPChanges();
    void applyEEEEEChanges();
    void restartPlatform();
    void killPlatformWithElevation(const QString& processName);
    Platform getPlatformFromString(const QString& platformStr);
    QString getPlatformName(Platform platform);
    void saveCurrentPlatformArgs();
    void restorePreviousPlatformArgs(Platform fromPlatform, Platform toPlatform);
    void loadSavedLaunchOptions();
    void clearWMPVPArgs();
    void clearEEEEEArgs();
    void restoreWMPVPArgs(const QString& args);
    void restoreEEEEEArgs(const QString& args);
    void restoreSteamArgs(const QString& args);
    void updateApplyAreaVisibility();
    QString readWMPVPLaunchOptions();
    QString readEEEEELaunchOptions();

private:
    Platform m_currentPlatform;
    Platform m_previousPlatform;
    ElaComboBox* m_platformComboBox;
    ElaComboBox* m_userComboBox;
    QMap<QString, SteamUserInfo> m_userInfoMap;
    QMap<QString, QPair<QString, ElaToggleSwitch*>> m_cfgSwitchMap;
    QVBoxLayout* m_contentLayout;
    QString m_originalLaunchOptions;
    QMap<Platform, QString> m_savedLaunchOptionsMap;
    bool m_hasUnsavedChanges;
    ElaScrollPageArea* m_applyArea;

signals:

};

#endif // T_MANAGER_H