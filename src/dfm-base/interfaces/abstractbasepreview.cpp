/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lixiang<lixianga@uniontech.com>
 *
 * Maintainer: lixiang<lixianga@uniontech.com>
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
#include "abstractbasepreview.h"

DFMBASE_USE_NAMESPACE
AbstractBasePreview::AbstractBasePreview(QObject *parent)
    : QObject(parent)
{
}

void AbstractBasePreview::initialize(QWidget *window, QWidget *statusBar) {
    Q_UNUSED(window)
            Q_UNUSED(statusBar)
}

QWidget *AbstractBasePreview::statusBarWidget() const
{
    return nullptr;
}

Qt::Alignment AbstractBasePreview::statusBarWidgetAlignment() const
{
    return Qt::AlignCenter;
}

QString AbstractBasePreview::title() const
{
    return QString();
}

bool AbstractBasePreview::showStatusBarSeparator() const
{
    return false;
}

void AbstractBasePreview::play()
{
}

void AbstractBasePreview::pause()
{
}

void AbstractBasePreview::stop()
{
}
