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
#include "ptConstants.h"
#include "ptTheme.h"

#include <QMessageBox>

extern ptTheme* Theme;

// Prototype
void Update(const QString GuiName);

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
  BlockedRightArrow = QPixmap(QString::fromUtf8(":/photivo/Icons/blockedrightarrow.png"));
  BlockedDownArrow = QPixmap(QString::fromUtf8(":/photivo/Icons/blockeddownarrow.png"));
  setObjectName("Box");
  m_Widget = new QWidget();
  m_Widget->setContentsMargins(5,5,0,5);

  m_Icon = new QLabel();
  m_Icon->setPixmap(DownArrow);

  m_Symbol = new QLabel();
  m_Symbol->setPixmap(QPixmap(QString::fromUtf8(":/photivo/Icons/attention.png")));

  QString Temp = Title;
  Temp.replace("(*)","");
  Temp.trimmed();

  m_Title = new QLabel();
  m_Title->setObjectName("Title");
  m_Title->setText("<b>"+Temp+"</b>");
  m_Title->setTextFormat(Qt::RichText);
  m_Title->setTextInteractionFlags(Qt::NoTextInteraction);

  QHBoxLayout *ButtonLayout = new QHBoxLayout();

  ButtonLayout->addWidget(m_Icon);
  ButtonLayout->addWidget(m_Title);
  if (Temp!=Title) {
    ButtonLayout->addWidget(m_Symbol);
  }
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
  m_IsBlocked = Settings->ToolIsBlocked(m_Name);

  m_AtnHide = new QAction(QObject::tr("Hide"), this);
  connect(m_AtnHide, SIGNAL(triggered()), this, SLOT(Hide()));

  m_AtnBlock = new QAction(QObject::tr("Block"), this);
  connect(m_AtnBlock, SIGNAL(triggered()), this, SLOT(SetBlocked()));
  m_AtnBlock->setCheckable(true);
  m_AtnBlock->setChecked(m_IsBlocked);

  UpdateView();

  //~ if (i==1 && j==1) this->setVisible(false);
  //~ test = new QLabel();
  //~ test->setText("Hallo");
  //~ Layout->addWidget(test);
}

////////////////////////////////////////////////////////////////////////////////
//
// Update
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::Update() {
  m_IsBlocked = Settings->ToolIsBlocked(m_Name);
  UpdateView();
}

////////////////////////////////////////////////////////////////////////////////
//
// UpdateView
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::UpdateView() {
  if (m_Folded==1) {
    m_Widget->setVisible(false);
    m_Icon->clear();
    if (m_IsBlocked) m_Icon->setPixmap(BlockedRightArrow);
    else if (m_IsActive) m_Icon->setPixmap(ActiveRightArrow);
    else m_Icon->setPixmap(RightArrow);
  } else {
    m_Widget->setVisible(true);
    m_Icon->clear();
    if (m_IsBlocked) m_Icon->setPixmap(BlockedDownArrow);
    else if (m_IsActive) m_Icon->setPixmap(ActiveDownArrow);
    else m_Icon->setPixmap(DownArrow);
  }
  if (m_IsBlocked) {
    m_Widget->setEnabled(0);
  } else {
    m_Widget->setEnabled(1);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Active
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::SetActive(const short IsActive) {
  m_IsActive = IsActive;

  UpdateView();
}

////////////////////////////////////////////////////////////////////////////////
//
// Blocked
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::SetBlocked() {
  m_IsBlocked = 1 - m_IsBlocked;
  m_Folded = m_IsBlocked;
  UpdateView();
  int Active = Settings->ToolIsActive(m_Name);
  QStringList Temp = Settings->GetStringList("BlockedTools");
  Temp.removeDuplicates();
  if (m_IsBlocked) {
    if (!Temp.contains(m_Name)) Temp.append(m_Name);
  } else {
    Temp.removeOne(m_Name);
  }
  Settings->SetValue("BlockedTools",Temp);
  // processor only needed after RAW since those tools are always visible
  if (Active || Settings->ToolIsActive(m_Name))
    ::Update(m_Name);
}

////////////////////////////////////////////////////////////////////////////////
//
// Hide
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::Hide() {
  int Active = Settings->ToolIsActive(m_Name);
  QStringList Temp = Settings->GetStringList("HiddenTools");
  if (!Temp.contains(m_Name)) Temp.append(m_Name);
  Settings->SetValue("HiddenTools",Temp);
  hide();
  // processor only needed after RAW since those tools are always visible
  if (Active) ::Update(m_Name);
}

////////////////////////////////////////////////////////////////////////////////
//
// changeEvent handler.
// To react on enable/disable
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::changeEvent(QEvent *) {
}

////////////////////////////////////////////////////////////////////////////////
//
// MousePress
//
////////////////////////////////////////////////////////////////////////////////

void ptGroupBox::mousePressEvent(QMouseEvent *event) {
  if (event->y()<20 && event->x()<250) {
    if (event->button()==Qt::LeftButton) {
      m_Folded = 1 - m_Folded;
      UpdateView();
      Settings->m_IniSettings->setValue(m_Name,m_Folded);
    } else if (event->button()==Qt::RightButton) {
      if (!Settings->ToolAlwaysVisible(m_Name)) {
        QMenu Menu(NULL);
        m_AtnBlock->setChecked(m_IsBlocked);
        Menu.setPalette(Theme->ptMenuPalette);
        Menu.setStyle(Theme->ptStyle);
        Menu.addAction(m_AtnBlock);
        Menu.addSeparator();
        Menu.addAction(m_AtnHide);
        Menu.exec(event->globalPos());
      }
    }/* else if (event->button()==Qt::RightButton &&
              event->modifiers() == Qt::ControlModifier) {
      if (!Settings->ToolAlwaysVisible(m_Name))
        m_IsBlocked = 1 - m_IsBlocked;
    }*/
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


