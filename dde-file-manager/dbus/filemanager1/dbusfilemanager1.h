/*
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

#ifndef DBUSFILEMANAGER1_H
#define DBUSFILEMANAGER1_H

#include <QObject>

class DBusFileManager1 : public QObject
{
    Q_OBJECT

public:
    explicit DBusFileManager1(QObject *parent = 0);

    void ShowFolders(const QStringList &URIs, const QString &StartupId);
    void ShowItemProperties(const QStringList &URIs, const QString &StartupId);
    void ShowItems(const QStringList &URIs, const QString &StartupId);
    void Trash(const QStringList &URIs);

    // debug function
    QStringList GetMonitorFiles() const;
};

#endif // DBUSFILEMANAGER1_H
