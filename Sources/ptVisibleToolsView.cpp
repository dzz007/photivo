/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2011 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2010-2011 Bernd Schoeler <brjohn@brother-john.net>
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
** along with Photivo.  If not, see <http:**www.gnu.org/licenses/>.
**
*******************************************************************************/

#include "ptVisibleToolsView.h"
#include "ptConstants.h"
#include "ptTheme.h"

extern ptTheme* Theme;

ptVisibleToolsModel::ptVisibleToolsModel(QObject *parent) :
		QStandardItemModel(parent) {
}

bool ptVisibleToolsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	QStandardItemModel::setData(index, value, role);
	if (role == Qt::UserRole+1)
	{
		if (value.toInt() == tsHidden)
			setData(index, QIcon(* Theme->ptIconCrossRed), Qt::DecorationRole);
		if (value.toInt() == tsFavourite)
			setData(index, QIcon(* Theme->ptIconStar), Qt::DecorationRole);
		if (value.toInt() == tsNormal)
			setData(index, QIcon(* Theme->ptIconStarGrey), Qt::DecorationRole);
	}
}


ptVisibleToolsItemDelegate::ptVisibleToolsItemDelegate(QObject *parent) :
		QStyledItemDelegate(parent) {
}

QWidget* ptVisibleToolsItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
	return new QComboBox(parent);
}

void ptVisibleToolsItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
	QComboBox *box = qobject_cast<QComboBox*>(editor);
//	box->setInsertPolicy(QComboBox::InsertAtTop);
	box->addItem(/*qvariant_cast<QIcon>(index.data(Qt::DecorationRole)),*/ index.data().toString(), index.data(Qt::UserRole+1));
	box->addItem(QIcon(* Theme->ptIconCrossRed), tr("Hidden"), tsHidden);
	box->addItem(QIcon(* Theme->ptIconStarGrey), tr("Normal"), tsNormal);
	box->addItem(QIcon(* Theme->ptIconStar), tr("Favourite"), tsFavourite);
}

void ptVisibleToolsItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
	QComboBox *box = qobject_cast<QComboBox*>(editor);
//	if (box->currentIndex() > 0)
//		model->setData(index, box->currentIndex()-1, Qt::UserRole+1);
	if (box->currentText() == tr("Hidden"))
		model->setData(index, tsHidden, Qt::UserRole+1);
	if (box->currentText() == tr("Normal"))
		model->setData(index, tsNormal, Qt::UserRole+1);
	if (box->currentText() == tr("Favourite"))
		model->setData(index, tsFavourite, Qt::UserRole+1);
}
