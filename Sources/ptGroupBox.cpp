////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include "ptGroupBox.h"
#include "ptSettings.h"

#include <QMessageBox>

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptGroupBox::ptGroupBox(const QString Title,
           QWidget* Parent,
           const QString Name) {

  QVBoxLayout *Layout = new QVBoxLayout(this);

  setParent(Parent);

  RightArrow = QPixmap(QString::fromUtf8(":/photivo/Icons/rightarrow.png"));
  DownArrow = QPixmap(QString::fromUtf8(":/photivo/Icons/downarrow.png"));
  ActiveRightArrow = QPixmap(QString::fromUtf8(":/photivo/Icons/activerightarrow.png"));
  ActiveDownArrow = QPixmap(QString::fromUtf8(":/photivo/Icons/activedownarrow.png"));
  setObjectName("Box");
  m_Widget = new QWidget();
  m_Widget->setContentsMargins(5,5,0,5);

  m_Icon = new QLabel();
  m_Icon->setPixmap(DownArrow);

  m_Title = new QLabel();
  m_Title->setObjectName("Title");
  m_Title->setText("<b>"+Title+"</b>");
  m_Title->setTextFormat(Qt::RichText);
  m_Title->setTextInteractionFlags(Qt::NoTextInteraction);

  QHBoxLayout *ButtonLayout = new QHBoxLayout();

  ButtonLayout->addWidget(m_Icon);
  ButtonLayout->addWidget(m_Title);
  ButtonLayout->addStretch();
  ButtonLayout->setContentsMargins(0,0,0,0);
  ButtonLayout->setSpacing(4);
  ButtonLayout->setMargin(0);

  Layout->addLayout(ButtonLayout);
  Layout->addWidget(m_Widget);
  Layout->setContentsMargins(0,0,0,0);
  Layout->setSpacing(0);
  Layout->setMargin(0);
  Layout->setAlignment(Qt::AlignTop);

  m_Name = Name;

  m_Folded = Settings->m_IniSettings->value(m_Name,1).toBool();
  m_IsActive = Settings->ToolIsActive(m_Name);
  if (m_Folded==1) {
    m_Widget->setVisible(false);
    m_Icon->clear();
    m_Icon->setPixmap(m_IsActive?ActiveRightArrow:RightArrow);
  } else {
    m_Widget->setVisible(true);
    m_Icon->clear();
    m_Icon->setPixmap(m_IsActive?ActiveDownArrow:DownArrow);
  }
  //~ if (i==1 && j==1) this->setVisible(false);
  //~ test = new QLabel();
  //~ test->setText("Hallo");
  //~ Layout->addWidget(test);
}

////////////////////////////////////////////////////////////////////////////////
//
// Active
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::SetActive(const short IsActive) {
  m_IsActive = IsActive;

  if (m_Folded==1) {
    m_Icon->clear();
    m_Icon->setPixmap(m_IsActive?ActiveRightArrow:RightArrow);
  } else {
    m_Icon->clear();
    m_Icon->setPixmap(m_IsActive?ActiveDownArrow:DownArrow);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// MousePress
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::mousePressEvent(QMouseEvent *event) {
  if (event->y()<20 && event->x()<250) {
    if (m_Folded==0) {
      m_Widget->setVisible(false);
      m_Icon->clear();
      m_Icon->setPixmap(m_IsActive?ActiveRightArrow:RightArrow);
      m_Folded = 1;
    } else {
      m_Widget->setVisible(true);
      m_Icon->clear();
      m_Icon->setPixmap(m_IsActive?ActiveDownArrow:DownArrow);
      m_Folded = 0;
    }
    Settings->m_IniSettings->setValue(m_Name,m_Folded);
  }
}

 void ptGroupBox::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptGroupBox::~ptGroupBox() {
}

////////////////////////////////////////////////////////////////////////////////


