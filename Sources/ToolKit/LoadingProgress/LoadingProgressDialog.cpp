#include "LoadingProgressDialog.h"
#include <QApplication>
#include <QPalette>
#include <QShowEvent>
#include <QMoveEvent>

LoadingProgressDialog::LoadingProgressDialog(QWidget *parent)
    : QObject(parent)
    , m_maskWidget(nullptr)
    , m_centralWidget(nullptr)
    , m_titleText(nullptr)
    , m_progressBar(nullptr)
    , m_parentWindow(parent)
    , m_maskOpacity(0.5)
    , m_progressMode(ProgressMode::Infinity)
    , m_isActive(false)
{
    if (m_parentWindow) {
        m_parentWindow->installEventFilter(this);

        m_maskWidget = new QWidget(m_parentWindow);
        m_maskWidget->setAutoFillBackground(true);
        m_maskWidget->setGeometry(m_parentWindow->rect());
        m_maskWidget->hide();

        QPalette maskPalette = m_maskWidget->palette();
        maskPalette.setColor(QPalette::Window, QColor(0, 0, 0, int(255 * m_maskOpacity)));
        m_maskWidget->setPalette(maskPalette);

        QVBoxLayout *centerLayout = new QVBoxLayout(m_maskWidget);
        centerLayout->setAlignment(Qt::AlignCenter);

        m_centralWidget = new QWidget(m_maskWidget);
        m_centralWidget->setFixedSize(400, 150);
        m_centralWidget->setAttribute(Qt::WA_StyledBackground);
        m_centralWidget->setStyleSheet("background-color: rgb(45, 45, 45); border-radius: 10px;");

        QVBoxLayout *contentLayout = new QVBoxLayout(m_centralWidget);
        contentLayout->setContentsMargins(20, 20, 20, 20);

        m_titleText = new ElaText(m_centralWidget);
        m_titleText->setText(tr("Loading..."));
        m_titleText->setTextStyle(ElaTextType::Subtitle);
        m_titleText->setAlignment(Qt::AlignCenter);

        m_progressBar = new ElaProgressBar(m_centralWidget);
        m_progressBar->setMinimumHeight(30);
        m_progressBar->setRange(0, 0);
        m_progressBar->setFormat("");

        contentLayout->addWidget(m_titleText, Qt::AlignCenter);
        contentLayout->addSpacing(10);
        contentLayout->addWidget(m_progressBar);

        centerLayout->addWidget(m_centralWidget, 0, Qt::AlignCenter);
    }
}

LoadingProgressDialog::~LoadingProgressDialog()
{
    if (m_parentWindow) {
        m_parentWindow->removeEventFilter(this);
    }
    if (m_maskWidget) {
        m_maskWidget->deleteLater();
    }
}

void LoadingProgressDialog::setTitle(const QString &title)
{
    if (m_titleText) {
        m_titleText->setText(title);
    }
}

void LoadingProgressDialog::setProgressMode(ProgressMode mode)
{
    m_progressMode = mode;
    if (m_progressBar) {
        if (mode == ProgressMode::Infinity) {
            m_progressBar->setRange(0, 0);
        }
    }
}

void LoadingProgressDialog::setRange(int minimum, int maximum)
{
    if (m_progressBar) {
        m_progressBar->setRange(minimum, maximum);
    }
}

void LoadingProgressDialog::setValue(int value)
{
    if (m_progressBar) {
        m_progressBar->setValue(value);
    }
}

void LoadingProgressDialog::setMaskOpacity(double opacity)
{
    m_maskOpacity = opacity;
    if (m_maskWidget) {
        QPalette pal = m_maskWidget->palette();
        pal.setColor(QPalette::Window, QColor(0, 0, 0, int(255 * m_maskOpacity)));
        m_maskWidget->setPalette(pal);
    }
}

void LoadingProgressDialog::show()
{
    if (!m_parentWindow) {
        m_parentWindow = QApplication::activeWindow();
    }

    if (!m_parentWindow) {
        return;
    }

    m_isActive = true;
    updateGeometry();
    m_maskWidget->show();
    m_maskWidget->raise();
}

void LoadingProgressDialog::hide()
{
    m_isActive = false;
    if (m_maskWidget) {
        m_maskWidget->hide();
    }
}

void LoadingProgressDialog::close()
{
    m_isActive = false;
    if (m_maskWidget) {
        m_maskWidget->hide();
    }
}

bool LoadingProgressDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_parentWindow) {
        if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
            if (m_isActive && m_maskWidget) {
                m_maskWidget->setGeometry(m_parentWindow->rect());
                updateGeometry();
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void LoadingProgressDialog::updateGeometry()
{
    if (m_maskWidget && m_parentWindow) {
        m_maskWidget->setGeometry(m_parentWindow->rect());
        if (m_centralWidget) {
            int x = (m_parentWindow->width() - m_centralWidget->width()) / 2;
            int y = (m_parentWindow->height() - m_centralWidget->height()) / 2;
            m_centralWidget->move(x, y);
        }
        m_maskWidget->raise();
    }
}

void LoadingProgressDialog::step()
{
    if (m_progressBar) {
        m_progressBar->setValue(m_progressBar->value() + 1);
    }
}

void LoadingProgressDialog::reset()
{
    if (m_progressBar) {
        m_progressBar->reset();
    }
}
