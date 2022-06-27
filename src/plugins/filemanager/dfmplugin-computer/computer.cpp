/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     xushitong<xushitong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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
#include "computer.h"
#include "utils/computerutils.h"
#include "views/computerview.h"
#include "fileentity/entryfileentities.h"
#include "events/computerunicastreceiver.h"
#include "events/computereventreceiver.h"
#include "watcher/computeritemwatcher.h"
#include "menu/computermenuscene.h"

#include "plugins/common/dfmplugin-menu/menu_eventinterface_helper.h"

#include "services/filemanager/sidebar/sidebarservice.h"
#include "services/filemanager/search/searchservice.h"
#include "services/filemanager/titlebar/titlebarservice.h"
#include "services/filemanager/workspace/workspaceservice.h"
#include "services/common/propertydialog/propertydialogservice.h"

#include "dfm-base/dfm_global_defines.h"
#include "dfm-base/base/urlroute.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm-base/file/entry/entryfileinfo.h"
#include "dfm-base/widgets/dfmwindow/filemanagerwindowsmanager.h"

DSB_FM_USE_NAMESPACE
DFMBASE_USE_NAMESPACE

namespace dfmplugin_computer {
/*!
 * \class Computer
 * \brief the plugin initializer
 */
void Computer::initialize()
{
    DFMBASE_USE_NAMESPACE
    using namespace dfmplugin_computer;
    DSC_USE_NAMESPACE

    UrlRoute::regScheme(ComputerUtils::scheme(), "/", ComputerUtils::icon(), true, tr("Computer"));
    ViewFactory::regClass<ComputerView>(ComputerUtils::scheme());
    UrlRoute::regScheme(Global::Scheme::kEntry, "/", QIcon(), true);
    InfoFactory::regClass<EntryFileInfo>(Global::Scheme::kEntry);

    EntryEntityFactor::registCreator<UserEntryFileEntity>(SuffixInfo::kUserDir);
    EntryEntityFactor::registCreator<BlockEntryFileEntity>(SuffixInfo::kBlock);
    EntryEntityFactor::registCreator<ProtocolEntryFileEntity>(SuffixInfo::kProtocol);
    EntryEntityFactor::registCreator<StashedProtocolEntryFileEntity>(SuffixInfo::kStashedProtocol);
    EntryEntityFactor::registCreator<AppEntryFileEntity>(SuffixInfo::kAppEntry);

    ComputerUnicastReceiver::instance()->connectService();

    connect(&FMWindowsIns, &FileManagerWindowsManager::windowCreated, this, &Computer::onWindowCreated, Qt::DirectConnection);
    connect(&FMWindowsIns, &FileManagerWindowsManager::windowOpened, this, &Computer::onWindowOpened, Qt::DirectConnection);
    connect(&FMWindowsIns, &FileManagerWindowsManager::windowClosed, this, &Computer::onWindowClosed, Qt::DirectConnection);

    bindEvents();
}

bool Computer::start()
{
    dpfSignalDispatcher->subscribe(SideBar::EventType::kEjectAction, ComputerEventReceiverIns, &ComputerEventReceiver::handleItemEject);

    dfmplugin_menu_util::menuSceneRegisterScene(ComputerMenuCreator::name(), new ComputerMenuCreator());

    DSB_FM_USE_NAMESPACE
    WorkspaceService::service()->addScheme(ComputerUtils::scheme());
    WorkspaceService::service()->setWorkspaceMenuScene(ComputerUtils::scheme(), ComputerMenuCreator::name());

    return true;
}

dpf::Plugin::ShutdownFlag Computer::stop()
{
    return kSync;
}

void Computer::onWindowCreated(quint64 winId)
{
    Q_UNUSED(winId);
    regComputerCrumbToTitleBar();
}

void Computer::onWindowOpened(quint64 winId)
{
    auto window = FMWindowsIns.findWindowById(winId);
    Q_ASSERT_X(window, "Computer", "Cannot find window by id");

    auto regSortAndQueryItems = [] {
        SideBarService::service()->registerSortFunc(DFMBASE_NAMESPACE::Global::Scheme::kComputer, [](const QUrl &a, const QUrl &b) { return ComputerUtils::sortItem(a, b); });
        ComputerItemWatcherInstance->startQueryItems();
    };
    if (window->workSpace())
        regSortAndQueryItems();
    else
        connect(window, &FileManagerWindow::workspaceInstallFinished, this, [regSortAndQueryItems] { regSortAndQueryItems(); }, Qt::DirectConnection);

    if (window->sideBar())
        addComputerToSidebar();
    else
        connect(window, &FileManagerWindow::sideBarInstallFinished, this, [this] { addComputerToSidebar(); }, Qt::DirectConnection);

    if (window->titleBar())
        regComputerToSearch();
    else
        connect(window, &FileManagerWindow::titleBarInstallFinished, this, [this] { regComputerToSearch(); }, Qt::DirectConnection);

    propertyServIns->registerCustomizePropertyView(ComputerUtils::devicePropertyDialog, DFMBASE_NAMESPACE::Global::Scheme::kEntry);
}

void Computer::onWindowClosed(quint64 winId)
{
    Q_UNUSED(winId);
}

void Computer::addComputerToSidebar()
{
    SideBar::ItemInfo entry;
    entry.group = SideBar::DefaultGroup::kDevice;
    entry.iconName = ComputerUtils::icon().name();
    entry.text = tr("Computer");
    entry.url = ComputerUtils::rootUrl();
    entry.flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    SideBarService::service()->insertItem(0, entry);
}

void Computer::regComputerCrumbToTitleBar()
{
    TitleBar::CustomCrumbInfo info;
    info.scheme = ComputerUtils::scheme();
    info.hideIconViewBtn = true;
    info.hideListViewBtn = true;
    info.hideDetailSpaceBtn = true;
    info.supportedCb = [](const QUrl &url) -> bool { return url.scheme() == ComputerUtils::scheme(); };
    info.seperateCb = [](const QUrl &url) -> QList<TitleBar::CrumbData> {
        Q_UNUSED(url);
        return { TitleBar::CrumbData(ComputerUtils::rootUrl(), tr("Computer"), ComputerUtils::icon().name()) };
    };
    TitleBarService::service()->addCustomCrumbar(info);
}

void Computer::regComputerToSearch()
{
    Search::CustomSearchInfo info;
    info.scheme = ComputerUtils::scheme();
    info.redirectedPath = "/";
    SearchService::service()->regCustomSearchInfo(info);
}

void Computer::bindEvents()
{
    dpfSlotChannel->connect(EventNameSpace::kComputerEventSpace, "slot_SetContextMenuEnable", ComputerUnicastReceiver::instance(), &ComputerUnicastReceiver::setContextMenuEnable);
    dpfSlotChannel->connect(EventNameSpace::kComputerEventSpace, "slot_AddDevice", ComputerUnicastReceiver::instance(), &ComputerUnicastReceiver::doAddDevice);
    dpfSlotChannel->connect(EventNameSpace::kComputerEventSpace, "slot_RemoveDevice", ComputerUnicastReceiver::instance(), &ComputerUnicastReceiver::doRemoveDevice);
}

}
