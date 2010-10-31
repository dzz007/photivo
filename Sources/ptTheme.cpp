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

// Copyright for the original CSS is 2010 Bernd Schöler

#include "ptTheme.h"
#include "ptSettings.h"
#include "ptSettings.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptTheme::ptTheme(QApplication* Application) {
  ptSystemStyle = (QStyle*)(Application->style()->metaObject()->newInstance());
  ptSystemPalette = Application->palette();
  ptThemeStyle = new QCleanlooksStyle;
  ptStyleSheet = "";
  ptPalette = QPalette();
  ptHighLight = QColor(255,255,255);
  ptStyle = ptSystemStyle;
}

////////////////////////////////////////////////////////////////////////////////
//
// Reset the theme
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::Reset() {
  ptStyle = ptSystemStyle;
  ptPalette = ptSystemPalette;
  ptStyleSheet = "";
}

////////////////////////////////////////////////////////////////////////////////
//
// Normal (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::Normal(short Color) {
  SetHighLightColor(Color);
  ptStyle = ptSystemStyle;
  JustTools();
  ptPalette = ptSystemPalette;
}

////////////////////////////////////////////////////////////////////////////////
//
// Mid grey (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::MidGrey(short Color) {
  SetHighLightColor(Color);
  ptText = QColor(30,30,30);
  ptDark = QColor(180,180,180);
  ptBackGround = QColor(127,127,127);
  ptGradient = QColor(150,150,150);
  ptBright = QColor(115,115,115);
  ptVeryBright = QColor(220,220,220);

  ptStyle = ptThemeStyle;
  CSS();

  // set the palette
  ptPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptPalette.setColor(QPalette::Background, ptBackGround);
  ptPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptPalette.setColor(QPalette::Base, ptBackGround); // Menu
  ptPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
  ptPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
  ptPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Text, ptText);
  ptPalette.setColor(QPalette::Button, ptBackGround); // Splitter
  // ptPalette.setColor(QPalette::ButtonText, ptText); // Menu
  ptPalette.setColor(QPalette::BrightText, QColor(255,0,0)); //

  ptPalette.setColor(QPalette::Light, ptDark); // Splitter
  ptPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptPalette.setColor(QPalette::Dark, ptBright);
  ptPalette.setColor(QPalette::Mid, ptDark);
  ptPalette.setColor(QPalette::Shadow, QColor(255,0,0));

  ptPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptPalette.setColor(QPalette::HighlightedText, ptText);
  ptPalette.setColor(QPalette::Link, QColor(255,0,0));
  ptPalette.setColor(QPalette::LinkVisited, QColor(255,0,0));
  ptPalette.setColor(QPalette::NoRole, QColor(255,0,0));
}

////////////////////////////////////////////////////////////////////////////////
//
// Dark grey (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::DarkGrey(short Color) {
  SetHighLightColor(Color);
  ptText = QColor(240,240,240);
  ptDark = QColor(30,30,30);
  ptBackGround = QColor(51,51,51);
  ptGradient = QColor(70,70,70);
  ptBright = QColor(115,115,115);
  ptVeryBright = QColor(180,180,180);

  ptStyle = ptThemeStyle;
  CSS();

  // set the palette
  ptPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptPalette.setColor(QPalette::Background, ptBackGround);
  ptPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptPalette.setColor(QPalette::Base, ptBackGround); // Menu
  ptPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
  ptPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
  ptPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Text, ptText);
  ptPalette.setColor(QPalette::Button, ptBackGround); // Splitter
  // ptPalette.setColor(QPalette::ButtonText, ptText); // Menu
  ptPalette.setColor(QPalette::BrightText, QColor(255,0,0)); //

  ptPalette.setColor(QPalette::Light, ptDark); // Splitter
  ptPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptPalette.setColor(QPalette::Dark, ptDark);
  ptPalette.setColor(QPalette::Mid, ptDark);
  ptPalette.setColor(QPalette::Shadow, QColor(255,0,0));

  ptPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptPalette.setColor(QPalette::HighlightedText, ptText);
  ptPalette.setColor(QPalette::Link, QColor(255,0,0));
  ptPalette.setColor(QPalette::LinkVisited, QColor(255,0,0));
  ptPalette.setColor(QPalette::NoRole, QColor(255,0,0));
}

////////////////////////////////////////////////////////////////////////////////
//
// CSS
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::CSS() {
  QString HighLight = "rgb(" + QString::number(ptHighLight.red()) + "," +
                               QString::number(ptHighLight.green()) + "," +
                               QString::number(ptHighLight.blue()) + ")";
  QString Text = "rgb(" + QString::number(ptText.red()) + "," +
                          QString::number(ptText.green()) + "," +
                          QString::number(ptText.blue()) + ")";
  QString BackGround = "rgb(" + QString::number(ptBackGround.red()) + "," +
                                QString::number(ptBackGround.green()) + "," +
                                QString::number(ptBackGround.blue()) + ")";
  QString Dark = "rgb(" + QString::number(ptDark.red()) + "," +
                          QString::number(ptDark.green()) + "," +
                          QString::number(ptDark.blue()) + ")";
  QString Gradient = "rgb(" + QString::number(ptGradient.red()) + "," +
                              QString::number(ptGradient.green()) + "," +
                              QString::number(ptGradient.blue()) + ")";
  QString Bright = "rgb(" + QString::number(ptBright.red()) + "," +
                            QString::number(ptBright.green()) + "," +
                            QString::number(ptBright.blue()) + ")";
  QString VeryBright = "rgb(" + QString::number(ptVeryBright.red()) + "," +
                                QString::number(ptVeryBright.green()) + "," +
                                QString::number(ptVeryBright.blue()) + ")";

  ptStyleSheet =
/************************************************************************************
Global colour definitions. Not needed for normal.css
************************************************************************************/
    "* {"
    "  background-color: " + BackGround + ";"
    "  color: " + Text + ";"
    "}"

/************************************************************************************
Following stuff is also needed for normal.css
************************************************************************************/

/** Spinbox ****************************************/
    "QAbstractSpinBox {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid black;"
    "  selection-background-color: " + Dark + ";"
    "  selection-color: " + Text + ";"

    "  border-radius: 3px;"
    "  padding-left: 2px;"
    "  padding-right: 2px;"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "  max-width: 40px;"
    "  min-width: 40px;"
    "}"

    "QAbstractSpinBox:disabled {"
    "  color: #888;"
    "}"

    "QAbstractSpinBox:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"

    "QAbstractSpinBox::down-button {"
    "  height: 0;"
    "  width: 0;"
    "}"

    "QAbstractSpinBox::up-button {"
    "  height: 0;"
    "  width: 0;"
    "}"


/** Checkbox ****************************************/
    "QCheckBox {"
    "  border: 1px solid " + BackGround + ";"

    "  border-radius: 3px;"
    "  padding: 2px;"
    "}"

    "QCheckBox:disabled {"
    "  color: #888;"
    "}"

    "QCheckBox:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"

    "QCheckBox::indicator {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "  height: 0.8em;"
    "  width: 0.8em;"
    "}"

    "QCheckBox::indicator:checked {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #555, stop:1 " + HighLight + ");"
    "}"


/** Combobox ****************************************/
    "QComboBox {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "  padding-left: 5px;"
    "  padding-right: 2px;"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "}"

    "QComboBox:disabled {"
    "  color: #888;"
    "}"

    "QComboBox:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"

    "QComboBox::drop-down {"
    "  border: none;"
    "  border-left: 1px solid " + Bright + ";"

    "  border-bottom-right-radius: 3px;"
    "  border-top-right-radius: 3px;"
    "  width: 10px;"
    "}"

    "QComboBox QAbstractItemView {"
    "  border: 1px solid " + VeryBright + ";"
    "  selection-background-color: #f00;"
    "  selection-color: #f00;"

    "  border-radius: 3px;"
    "}"


/** Textedits ****************************************/
    "QLineEdit, QPlainTextEdit {"
    "  border: 1px solid " + VeryBright + ";"

    "  border-radius: 3px;"
    "  padding-left: 2px;"
    "  padding-right: 2px;"
    "}"

    "QLineEdit {"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "}"

    "QLineEdit:disabled, QPlainTextEdit:disabled {"
    "  color: #888;"
    "}"

    "QLineEdit:hover, QPlainTextEdit:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"


/** Slider ****************************************/
    "QSlider {"
    "  background: none;"
    "  border: 1px solid " + BackGround + ";"

    "  padding: 2px;"
    "  max-height: 8px;"
    "  min-height: 8px;"
    "}"

    "QSlider:hover {"
    "  border: 1px solid " + HighLight + ";"

    "  border-radius: 3px;"
    "}"

    "QSlider::groove:horizontal {"
    "  background: " + VeryBright + ";"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "}"

    "QSlider::handle:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"

    "  width: 12px;"
    "}"

    "QSlider#HueSlider::groove:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 red, stop: 0.25 yellow, stop: 0.5 green, stop: 0.75 blue, stop: 1 red);"
    "}"


/** Button ****************************************/
    "QToolButton {"
    "  border: none;"

    "  padding: 2px;"
    "}"

    "QToolButton:hover {"
    "  border: 1px solid " + HighLight + ";"

    "  border-radius: 3px;"
    "}"


/************************************************************************************
Everything from here on shouldn't be relevant for normal.css (I think ;) ...)
************************************************************************************/


/** Menu ****************************************/
    "QMenu {"
    "  background-color: " + BackGround + ";"
    "}"

    "QMenu::item:selected {"
    "  background-color: " + Bright + ";"
    "  color: " + Dark + ";"
    "}"

    "QMenu::separator {"
    "     height: 2px;"
    "     background: #555;"
    "     margin: 2px 5px 2px 5px;"
    "}"

    "QMenu::indicator, QMenu::indicator:non-exclusive:unchecked, QMenu::indicator:non-exclusive:checked {"
    "  background: " + BackGround + ";"
    "  border: 0px solid " + BackGround + ";"
    "}"


/** Scrollbar ****************************************/
    "QScrollBar:horizontal {"
    "  background: " + Dark + ";"

    "  border: none;"
    "  border-radius: 3px;"
    "  margin: 0 40px 0 0;"
    "}"

    "QScrollBar:vertical {"
    "  background: " + Dark + ";"

    "  border: none;"
    "  border-radius: 3px;"
    "  margin: 0 0 40px 0;"
    "}"

    "QScrollBar::handle:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-radius: 4px;"
    "  min-width: 20px;"
    "}"

    "QScrollBar::handle:vertical {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-radius: 4px;"
    "  min-height: 20px;"
    "}"

    "QScrollBar::handle:horizontal:hover, QScrollBar::handle:vertical:hover,"
    "QScrollBar::add-line:horizontal:hover, QScrollBar::add-line:vertical:hover,"
    "QScrollBar::sub-line:horizontal:hover, QScrollBar::sub-line:vertical:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"

    "QScrollBar::add-line:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-top-right-radius: 7px;"
    "  border-bottom-right-radius: 7px;"
    "  subcontrol-position: right;"
    "  subcontrol-origin: margin;"
    "  width: 16px;"
    "}"

    "QScrollBar::add-line:vertical {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-bottom-left-radius: 7px;"
    "  border-bottom-right-radius: 7px;"
    "  subcontrol-position: bottom;"
    "  subcontrol-origin: margin;"
    "  height: 16px;"
    "}"

    "QScrollBar::sub-line:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-top-left-radius: 7px;"
    "  border-bottom-left-radius: 7px;"
    "  subcontrol-position: top right;"
    "  subcontrol-origin: margin;"
    "  position: absolute;"
    "  right: 20px;"
    "  width: 16px;"
    "}"

    "QScrollBar::sub-line:vertical {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " + Gradient + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-top-left-radius: 7px;"
    "  border-top-right-radius: 7px;"
    "  subcontrol-position: bottom right;"
    "  subcontrol-origin: margin;"
    "  position: absolute;"
    "  bottom: 20px;"
    "  height: 16px;"
    "}"

    "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal,"
    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
    "  background: none;"
    "}"


/** Splitter ****************************************/
    "QSplitter {"
    "  max-width: 4px;"
    "  min-width: 4px;"
    "}"

    "QSplitter::handle:horizontal {"
    "  background-color: " + Dark + ";"
    "  border: 1px solid #000;"
    "  color: #000;"

    "  width: 4px;"
    "}"

    "QSplitter::handle:vertical {"
    "  background-color: " + Dark + ";"
    "  border: 1px solid #000;"
    "  color: #000;"

    "  height: 4px;"
    "}"


/** Tabs ****************************************/
    "QTabWidget::pane {"
    "  border-top: 0px solid qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #000, stop: 1 " + BackGround + ");"
    "}"

    "QTabWidget::tab-bar {"
    "  alignment: center;"
    "}"

    "QTabBar::tab {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Bright + ", stop: 1 " + BackGround + ");"
    "  border: 1px solid #000;"

    "  border-radius: 4px;"
    "  margin: 5px 1px 5px 1px;"
    "  padding: 4px;"
    "}"

    "QTabBar::tab:selected, QTabBar::tab:hover {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + Bright + ", stop: 1 " + Dark+ ");"
    "}"

    "QTabBar::tab:selected {"
    "  border-color: " + HighLight + ";"
    "}"

    "QTabBar::tab:!selected {"
    "  margin-top: 7px;"
    "}"

    /* Scrolling of tabs when not enough space */
    "QTabBar::tear {"
    "  background: " + BackGround + ";"
    "}"

    /* Special treatment for vertical tabs */
    "QTabWidget::pane {"
    "  border: none;"
    "}"

    "QTabWidget::tab-bar {"
    "  alignment: left;"
    "}"

    "QTabWidget QTabBar::tab {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " + Bright + ", stop: 1 " + Gradient + ");"
    "  margin: 1px 5px 1px 5px;"
    "}"

    "QTabWidget QTabBar::tab:selected, QTabWidget QTabBar::tab:hover {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " + Gradient + ", stop: 1 " + Dark + ");"
    "}"

    "QTabWidget QTabBar::tab:!selected {"
    "  margin-top: 1px;"
    "  margin-left: 7px;"
    "}"


/******************************************/
    "*#StatusFrame {  /* status text bottom left without border */"
    "  border: 1px solid " + Dark + ";"
    "}"

    "*#StatusLabel {"
    "  color: " + Text + ";"
    "}";
}

////////////////////////////////////////////////////////////////////////////////
//
// Just tools
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::JustTools() {
  QString HighLight = "rgb(" + QString::number(ptHighLight.red()) + "," +
                               QString::number(ptHighLight.green()) + "," +
                               QString::number(ptHighLight.blue()) + ")";
  ptStyleSheet =
/** Spinbox ****************************************/
    "QAbstractSpinBox {"
    "  border: 1px solid #000;"
    "  border-radius: 3px;"
    "  padding-left: 2px;"
    "  padding-right: 2px;"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "  max-width: 40px;"
    "  min-width: 40px;"
    "}"

    "QAbstractSpinBox:disabled {"
    "}"

    "QAbstractSpinBox:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"

    "QAbstractSpinBox::down-button {"
    "  height: 0;"
    "  width: 0;"
    "}"

    "QAbstractSpinBox::up-button {"
    "  height: 0;"
    "  width: 0;"
    "}"


/** Checkbox ****************************************/
    "QCheckBox {"
    "  border-radius: 3px;"
    "  padding: 2px;"
    "}"

    "QCheckBox:disabled {"
    "}"

    "QCheckBox:hover {"
    "  border: 1px solid " + HighLight + ";"
    "  padding: 1px;"
    "}"


/** Combobox ****************************************/
    "QComboBox {"
    "  border: 1px solid #000;"
    "  border-radius: 3px;"
    "  padding-left: 5px;"
    "  padding-right: 2px;"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "}"

    "QComboBox:disabled {"
    "  color: #888;"
    "}"

    "QComboBox:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"

    "QComboBox::drop-down {"
    "  border: none;"
    "  border-left: 1px solid #000;"
    "  border-bottom-right-radius: 3px;"
    "  border-top-right-radius: 3px;"
    "  width: 10px;"
    "}"

    "QComboBox QAbstractItemView {"
    "  border: 1px solid #b1b1b1;"
    "  border-radius: 3px;"
    "}"

    "QComboBox QAbstractItemView::indicator, QComboBox QAbstractItemView::icon {"
    "  min-height: 0;"
    "  max-height: 0;"
    "  min-width: 0;"
    "  max-width: 0;"
    "  image: none;"
    "}"


/** Textedits ****************************************/
    "QLineEdit, QPlainTextEdit {"
    "  border: 1px solid #000;"
    "  border-radius: 3px;"
    "  padding-left: 2px;"
    "  padding-right: 2px;"
    "}"

    "QLineEdit {"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "}"

    "QLineEdit:disabled, QPlainTextEdit:disabled {"
    "}"

    "QLineEdit:hover, QPlainTextEdit:hover {"
    "  border: 1px solid " + HighLight + ";"
    "}"


/** Menu ****************************************/
    "QMenu {"
    "  border: 1px solid #000;"
    "  border-radius: 3px;"
    "  padding: 2px;"
    "}"


/** Slider ****************************************/
    "QSlider {"
    "  padding: 3px;"
    "  max-height: 8px;"
    "  min-height: 8px;"
    "}"

    "QSlider:hover {"
    "  border: 1px solid " + HighLight + ";"
    "  padding: 2px;"
    "  border-radius: 3px;"
    "}"

    "QSlider::groove:horizontal {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "}"

    "QSlider::handle:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 rgb(70, 70, 70), stop: 1 rgb(51, 51, 51));"

    "  width: 12px;"
    "}"

    "QSlider#HueSlider::groove:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 red, stop: 0.25 yellow, stop: 0.5 green, stop: 0.75 blue, stop: 1 red);"
    "}"


/** Button ****************************************/
    "QToolButton {"
    "  border: none;"

    "  padding: 2px;"
    "}"

    "QToolButton:hover {"
    "  border: 1px solid " + HighLight + ";"

    "  border-radius: 3px;"
    "}"


/** Frame ****************************************/
    "#StatusFrame {  /* status text bottom left without border */"
    "  border: none;"
    "}";
}

////////////////////////////////////////////////////////////////////////////////
//
// Set highlight color
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::SetHighLightColor(short Color) {
  if (Color == ptStyleHighLight_White)
    ptHighLight = QColor(255,255,255);
  else if (Color == ptStyleHighLight_Purple)
    ptHighLight = QColor(140,60,240);
  else if (Color == ptStyleHighLight_Blue)
    ptHighLight = QColor(60,180,255);
  else if (Color == ptStyleHighLight_Green)
    ptHighLight = QColor(100,255,100);
  else if (Color == ptStyleHighLight_Orange)
    ptHighLight = QColor(255,180,60);
}

////////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
////////////////////////////////////////////////////////////////////////////////

ptTheme::~ptTheme() {
  delete ptSystemStyle;
  delete ptThemeStyle;
}

////////////////////////////////////////////////////////////////////////////////


