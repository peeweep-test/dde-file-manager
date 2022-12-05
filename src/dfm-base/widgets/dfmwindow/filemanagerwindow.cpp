/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "filemanagerwindow.h"
#include "private/filemanagerwindow_p.h"

#include "dfm-base/base/application/application.h"
#include "dfm-base/base/application/settings.h"

#include <QUrl>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QHideEvent>

namespace dfmbase {

/*!
 * \class FileManagerWindowPrivate
 * \brief
 */

FileManagerWindowPrivate::FileManagerWindowPrivate(const QUrl &url, FileManagerWindow *qq)
    : QObject(nullptr),
      q(qq),
      currentUrl(url)
{
}

bool FileManagerWindowPrivate::processKeyPressEvent(QKeyEvent *event)
{
    switch (event->modifiers()) {
    case Qt::NoModifier: {
        switch (event->key()) {
        case Qt::Key_F5:
            emit q->reqRefresh();
            return true;
        }
        break;
    }
    case Qt::ControlModifier: {
        switch (event->key()) {
        case Qt::Key_Tab:
            emit q->reqActivateNextTab();
            return true;
        case Qt::Key_Backtab:
            emit q->reqActivatePreviousTab();
            return true;
        case Qt::Key_F:
            emit q->reqSearchCtrlF();
            return true;
        case Qt::Key_L:
            emit q->reqSearchCtrlL();
            return true;
        case Qt::Key_Left:
            emit q->reqBack();
            return true;
        case Qt::Key_Right:
            emit q->reqForward();
            return true;
        case Qt::Key_W:
            emit q->reqCloseCurrentTab();
            return true;
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
            emit q->reqTriggerActionByIndex(event->key() - Qt::Key_1);
            return true;
        }
        break;
    }
    case Qt::AltModifier:
    case Qt::AltModifier | Qt::KeypadModifier:
        if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_8) {
            emit q->reqActivateTabByIndex(event->key() - Qt::Key_1);
            return true;
        }

        switch (event->key()) {
        case Qt::Key_Left:
            emit q->reqBack();
            return true;
        case Qt::Key_Right:
            emit q->reqForward();
            return true;
        }
        break;
    case Qt::ControlModifier | Qt::ShiftModifier:
        if (event->key() == Qt::Key_Question) {
            emit q->reqShowHotkeyHelp();
            return true;
        } else if (event->key() == Qt::Key_Backtab) {
            emit q->reqActivatePreviousTab();
            return true;
        }
        break;
    }
    return false;
}

int FileManagerWindowPrivate::splitterPosition() const
{
    return splitter ? splitter->sizes().at(0) : kMaximumLeftWidth;
}

void FileManagerWindowPrivate::setSplitterPosition(int pos)
{
    if (splitter)
        splitter->setSizes({ pos, splitter->width() - pos - splitter->handleWidth() });
}

/*!
 * \class FileManagerWindow
 * \brief
 */

FileManagerWindow::FileManagerWindow(const QUrl &url, QWidget *parent)
    : DMainWindow(parent),
      d(new FileManagerWindowPrivate(url, this))
{
    initializeUi();
    initConnect();
}

FileManagerWindow::~FileManagerWindow()
{
}

void FileManagerWindow::cd(const QUrl &url)
{
    d->currentUrl = url;
    if (d->titleBar)
        d->titleBar->setCurrentUrl(url);
    if (d->sideBar)
        d->sideBar->setCurrentUrl(url);
    if (d->workspace)
        d->workspace->setCurrentUrl(url);
    if (d->detailSpace)
        d->detailSpace->setCurrentUrl(url);
    emit currentUrlChanged(url);
}

bool FileManagerWindow::saveClosedSate() const
{
    return true;
}

QUrl FileManagerWindow::currentUrl() const
{
    return d->currentUrl;
}

void FileManagerWindow::moveCenter(const QPoint &cp)
{
    QRect qr = frameGeometry();

    qr.moveCenter(cp);
    move(qr.topLeft());
}

void FileManagerWindow::installTitleBar(AbstractFrame *w)
{
    Q_ASSERT_X(w, "FileManagerWindow", "Null TitleBar");
    std::call_once(d->titleBarFlag, [this, w]() {
        d->titleBar = w;
        d->titleBar->setCurrentUrl(d->currentUrl);
        titlebar()->setCustomWidget(d->titleBar);

        emit this->titleBarInstallFinished();
    });
}

void FileManagerWindow::installTitleMenu(QMenu *menu)
{
    Q_ASSERT_X(menu, "FileManagerWindow", "Null Title Menu");
    std::call_once(d->titleMenuFlag, [this, menu]() {
        titlebar()->setMenu(menu);

        emit this->titleMenuInstallFinished();
    });
}

void FileManagerWindow::installSideBar(AbstractFrame *w)
{
    Q_ASSERT_X(w, "FileManagerWindow", "Null setSideBar");
    std::call_once(d->sideBarFlag, [this, w]() {
        d->sideBar = w;
        d->splitter->replaceWidget(0, d->sideBar);

        d->sideBar->setContentsMargins(0, 0, 0, 0);
        d->sideBar->setMaximumWidth(d->kMaximumLeftWidth);
        d->sideBar->setMinimumWidth(d->kMinimumLeftWidth);
        d->sideBar->setCurrentUrl(d->currentUrl);

        emit this->sideBarInstallFinished();
    });
}

void FileManagerWindow::installWorkSpace(AbstractFrame *w)
{
    Q_ASSERT_X(w, "FileManagerWindow", "Null Workspace");
    std::call_once(d->workspaceFlag, [this, w]() {
        d->workspace = w;
        d->splitter->replaceWidget(1, d->workspace);

        // NOTE(zccrs): 保证窗口宽度改变时只会调整right view的宽度，侧边栏保持不变
        //              QSplitter是使用QLayout的策略对widgets进行布局，所以此处
        //              设置size policy可以生效
        QSizePolicy sp = d->workspace->sizePolicy();
        sp.setHorizontalStretch(1);
        d->workspace->setSizePolicy(sp);
        d->workspace->setCurrentUrl(d->currentUrl);
        d->workspace->installEventFilter(this);
        emit this->workspaceInstallFinished();
    });
}

/*!
 * \brief NOTE: shouldn't call it if detail button not clicked in titlebar
 * \param w
 */
void FileManagerWindow::installDetailView(AbstractFrame *w)
{
    d->detailSpace = w;
    if (d->detailSpace) {
        d->detailSpace->setFixedWidth(320);
        d->midLayout->addWidget(d->detailSpace, 1);
        d->detailSpace->setVisible(false);
        d->detailSpace->setCurrentUrl(d->currentUrl);
    }

    emit this->detailViewInstallFinished();
}

AbstractFrame *FileManagerWindow::titleBar() const
{
    return d->titleBar;
}

AbstractFrame *FileManagerWindow::sideBar() const
{
    return d->sideBar;
}

AbstractFrame *FileManagerWindow::workSpace() const
{
    return d->workspace;
}

AbstractFrame *FileManagerWindow::detailView() const
{
    return d->detailSpace;
}

void FileManagerWindow::showEvent(QShowEvent *event)
{
    DMainWindow::showEvent(event);

    const QVariantMap &state = Application::appObtuselySetting()->value("WindowManager", "SplitterState").toMap();
    int splitterPos = state.value("sidebar", d->kMaximumLeftWidth).toInt();
    d->setSplitterPosition(splitterPos);
}

void FileManagerWindow::paintEvent(QPaintEvent *event)
{
    DMainWindow::paintEvent(event);

    std::call_once(d->openFlag, [this]() {
        QMetaObject::invokeMethod(this, "aboutToOpen", Qt::QueuedConnection);
    });
}

void FileManagerWindow::closeEvent(QCloseEvent *event)
{
    // NOTE(zhangs): bug 59239
    emit aboutToClose();
    DMainWindow::closeEvent(event);
}

void FileManagerWindow::hideEvent(QHideEvent *event)
{
    QVariantMap state;
    state["sidebar"] = d->splitterPosition();
    Application::appObtuselySetting()->setValue("WindowManager", "SplitterState", state);

    return DMainWindow::hideEvent(event);
}

void FileManagerWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->y() <= d->titleBar->height()) {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    } else {
        DMainWindow::mouseDoubleClickEvent(event);
    }
}

void FileManagerWindow::moveEvent(QMoveEvent *event)
{
    DMainWindow::moveEvent(event);

    emit positionChanged(event->pos());
}

void FileManagerWindow::keyPressEvent(QKeyEvent *event)
{
    if (!d->processKeyPressEvent(event))
        return DMainWindow::keyPressEvent(event);
}

bool FileManagerWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (!d->workspace || watched != d->workspace)
        return false;

    if (event->type() != QEvent::KeyPress)
        return false;

    return d->processKeyPressEvent(static_cast<QKeyEvent *>(event));
}

void FileManagerWindow::initializeUi()
{
    titlebar()->setIcon(QIcon::fromTheme("dde-file-manager", QIcon::fromTheme("system-file-manager")));

    // size
    resize(d->kDefaultWindowWidth, d->kDefaultWindowHeight);
    setMinimumSize(d->kMinimumWindowWidth, d->kMinimumWindowHeight);

    // title bar
    titlebar()->setContentsMargins(0, 0, 0, 0);

    // left view
    d->leftView = new QFrame;
    d->leftView->setMaximumWidth(d->kMaximumLeftWidth);
    d->leftView->setMinimumWidth(d->kMinimumLeftWidth);

    // right view
    d->rightView = new QFrame;

    // splitter
    d->splitter = new Splitter(Qt::Orientation::Horizontal, this);
    d->splitter->setChildrenCollapsible(false);
    d->splitter->setHandleWidth(0);
    d->splitter->addWidget(d->leftView);
    d->splitter->addWidget(d->rightView);

    // central
    d->centralView = new QFrame(this);
    d->centralView->setObjectName("CentralView");
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QWidget *midWidget = new QWidget;
    d->midLayout = new QHBoxLayout;
    midWidget->setLayout(d->midLayout);
    d->midLayout->setContentsMargins(0, 0, 0, 0);
    d->midLayout->addWidget(d->splitter);
    mainLayout->addWidget(midWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    d->centralView->setLayout(mainLayout);
    setCentralWidget(d->centralView);
}

void FileManagerWindow::initConnect()
{
}

}
