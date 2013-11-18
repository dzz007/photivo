/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2013 Alexander Tzyganenko <tz@fast-report.com>
**
** This file is part of Photivo.
**
** Photivo is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 3
** as published by the Free Software Foundation.
**
** Photivo is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/
// code from http://qt-project.org/wiki/Filter_Columns_in_a_QFileSystemModel

#ifndef PTFSMPROXY_H
#define PTFSMPROXY_H

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QFileIconProvider>
#include <QMimeData>
     
class ptFSMProxy : public QSortFilterProxyModel
{
    Q_OBJECT
 
public:
    ptFSMProxy(QObject *parent);
    QModelIndex getIndexForPath(const QString &path) const;
    QString getPathForIndex(const QModelIndex &index) const;
    void setCurrentDir(const QString& path) { m_CurrentDir = path; }
protected:
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    Qt::DropActions supportedDropActions() const;
    QStringList mimeTypes() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent);
private:
    QFileSystemModel *m_model;
    QString m_CurrentDir;
};


class ptFileIconProviderPrivate : public QFileIconProvider 
{
public:
  ptFileIconProviderPrivate();
  QIcon icon(const QFileInfo &info) const;

private:
  QIcon folderIcon;
};

#endif