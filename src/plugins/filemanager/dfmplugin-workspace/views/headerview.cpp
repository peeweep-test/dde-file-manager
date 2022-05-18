/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huanyu<huanyub@uniontech.com>
 *
 * Maintainer: zhengyouge<zhengyouge@uniontech.com>
 *             yanghao<yanghao@uniontech.com>
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

#include "headerview.h"
#include "models/filesortfilterproxymodel.h"

DPWORKSPACE_USE_NAMESPACE

HeaderView::HeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
    setHighlightSections(false);
    setSectionsClickable(true);
    setSortIndicatorShown(true);
    setSectionsMovable(true);
    setFirstSectionMovable(false);
}

QSize HeaderView::sizeHint() const
{
    QSize size = QHeaderView::sizeHint();

    size.setWidth(length());

    return size;
}

int HeaderView::sectionsTotalWidth() const
{
    int totalWidth = 0;
    for (int i = 0; i < model()->columnCount(); ++i) {
        totalWidth += sectionSize(i);
    }

    return totalWidth;
}

void HeaderView::updateColumnWidth()
{
    auto proxyModel = this->proxyModel();
    if (proxyModel) {
        int columnCount = count();
        int i = 0;
        int j = columnCount - 1;

        for (; i < columnCount; ++i) {
            int logicalIndex = this->logicalIndex(i);
            if (isSectionHidden(logicalIndex))
                continue;

            resizeSection(logicalIndex, proxyModel->getColumnWidth(i) + kLeftPadding + kListModeLeftMargin + 2 * kColumnPadding);
            break;
        }

        for (; j > 0; --j) {
            int logicalIndex = this->logicalIndex(j);
            if (isSectionHidden(logicalIndex))
                continue;

            resizeSection(logicalIndex, proxyModel->getColumnWidth(j) + kRightPadding + kListModeRightMargin + 2 * kColumnPadding);
            break;
        }

        if (firstVisibleColumn != i) {
            if (firstVisibleColumn > 0)
                resizeSection(logicalIndex(firstVisibleColumn), proxyModel->getColumnWidth(firstVisibleColumn) + 2 * kColumnPadding);

            firstVisibleColumn = i;
        }

        if (lastVisibleColumn != j) {
            if (lastVisibleColumn > 0)
                resizeSection(logicalIndex(lastVisibleColumn), proxyModel->getColumnWidth(lastVisibleColumn) + 2 * kColumnPadding);

            lastVisibleColumn = j;
        }
    }
}

void HeaderView::updataFirstColumnWidth(const int totalWidth)
{
    auto proxyModel = this->proxyModel();

    if (proxyModel) {
        int tailWidth = 0;
        for (int i = 1; i < proxyModel->columnCount(); ++i) {
            if (i < count()) {
                int columnWidth = proxyModel->getColumnWidth(i);
                resizeSection(i, columnWidth);
                tailWidth += columnWidth;
            }
        }
        resizeSection(0, totalWidth - tailWidth);
    }
}

void HeaderView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_EMIT mouseReleased();

    return QHeaderView::mouseReleaseEvent(e);
}

void HeaderView::resizeEvent(QResizeEvent *e)
{
    Q_EMIT viewResized();

    return QHeaderView::resizeEvent(e);
}

FileSortFilterProxyModel *HeaderView::proxyModel() const
{
    return qobject_cast<FileSortFilterProxyModel *>(model());
}
