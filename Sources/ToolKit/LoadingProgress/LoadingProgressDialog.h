#ifndef LOADING_PROGRESS_DIALOG_H
#define LOADING_PROGRESS_DIALOG_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "stdafx.h"
#include "ElaProgressBar.h"
#include "ElaText.h"

class LoadingProgressDialog : public QObject
{
    Q_OBJECT
public:
    enum class ProgressMode {
        Infinity,
        Step
    };

    explicit LoadingProgressDialog(QWidget *parent = nullptr);
    ~LoadingProgressDialog();

    void setTitle(const QString &title);
    void setProgressMode(ProgressMode mode);
    void setRange(int minimum, int maximum);
    void setValue(int value);
    void setMaskOpacity(double opacity);
    void show();
    void hide();
    void close();

public slots:
    void step();
    void reset();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateGeometry();

    QWidget *m_maskWidget;
    QWidget *m_centralWidget;
    ElaText *m_titleText;
    ElaProgressBar *m_progressBar;

    QWidget *m_parentWindow;
    double m_maskOpacity;
    ProgressMode m_progressMode;
    bool m_isActive;
};

#endif
