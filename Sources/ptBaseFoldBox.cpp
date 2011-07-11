/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2010-2011 Michael Munzert <mail@mm-log.com>
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

#include <cassert>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>

#include "ptBaseFoldBox.h"
#include "ptTheme.h"

extern ptTheme* Theme;

////////////////////////////////////////////////////////////////////////////////
//
// constructor and destructor
//
////////////////////////////////////////////////////////////////////////////////

ptBaseFoldBox::ptBaseFoldBox(QWidget* Parent, const QString Title,
                               const QString ObjectName, QWidget* Body /*= new QWidget()*/)
: QWidget(Parent),
  m_Head(new QWidget()),
  m_Body(Body),
  m_ArrowLabel(new QLabel()),
  m_Enabled(1),
  m_Folded(0),
  m_Title(Title),
  m_TitleLabel(new QLabel("<b>" + Title + "</b>"))
{
  assert(Parent != NULL);
  m_DefaultArrow[0] = QPixmap(QString::fromUtf8(":/photivo/Icons/downarrow.png"));
  m_DefaultArrow[1] = QPixmap(QString::fromUtf8(":/photivo/Icons/rightarrow.png"));
  this->setObjectName(ObjectName);
  m_Head->setObjectName("ToolHeader");
  m_Head->setStyleSheet(Theme->ptStyleSheet);
  m_Head->installEventFilter(this);
  m_Body->setObjectName("Body");

  // Setup groupbox head
  m_ArrowLabel->setPixmap(m_DefaultArrow[m_Folded]);
  m_ArrowLabel->setObjectName("ArrowLabel");
  m_TitleLabel->setTextFormat(Qt::RichText);
  m_TitleLabel->setTextInteractionFlags(Qt::NoTextInteraction);
  m_TitleLabel->setObjectName("TitleLabel");
  QHBoxLayout* HeadLayout = new QHBoxLayout(m_Head);
  HeadLayout->addWidget(m_ArrowLabel);
  HeadLayout->addWidget(m_TitleLabel);
  HeadLayout->addStretch();
  HeadLayout->setContentsMargins(-3, 0, 0, 0);
  HeadLayout->setSpacing(4);
  HeadLayout->setMargin(3);

  // Setup groupbox itself
  QVBoxLayout* BoxLayout = new QVBoxLayout(this);
  BoxLayout->addWidget(m_Head);
  BoxLayout->addWidget(m_Body);
  BoxLayout->setContentsMargins(0,0,0,0);
  BoxLayout->setSpacing(0);
  BoxLayout->setMargin(0);
  BoxLayout->setAlignment(Qt::AlignTop);


}


ptBaseFoldBox::~ptBaseFoldBox() {
}


////////////////////////////////////////////////////////////////////////////////
//
// toggleFolded
//
////////////////////////////////////////////////////////////////////////////////

void ptBaseFoldBox::toggleFolded() {
  m_Folded = 1 - m_Folded;
  m_Body->setVisible(m_Folded);
  m_ArrowLabel->clear();
  m_ArrowLabel->setPixmap(m_DefaultArrow[m_Folded]);

  // Head has different visual appearance depending on folded state.
  // This is controlled by the #ToolHeader ID in the stylesheet.
  if (m_Folded) {
    m_Head->setObjectName("");
  } else {
    m_Head->setObjectName("ToolHeader");
  }
  m_Head->setStyleSheet(Theme->ptStyleSheet);
}


////////////////////////////////////////////////////////////////////////////////
//
// setEnabled
//
////////////////////////////////////////////////////////////////////////////////

void ptBaseFoldBox::setEnabled(bool enabled) {
  m_Enabled = enabled;
  m_Body->setEnabled(enabled);
}


////////////////////////////////////////////////////////////////////////////////
//
// Event handling
//
////////////////////////////////////////////////////////////////////////////////

bool ptBaseFoldBox::eventFilter(QObject* object, QEvent* event) {
  if (object == m_Head) {
    if (event->type() == QEvent::MouseButtonPress) {
      handleMousePress(static_cast<QMouseEvent*>(event));
      return true;

    } else {
      return false;
    }

  } else {
    return QWidget::eventFilter(object, event);
  }
}


void ptBaseFoldBox::handleMousePress(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    event->accept();
    toggleFolded();

  } else {
    event->ignore();
  }
}
