/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010-2011 Michael Munzert <mail@mm-log.com>
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
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

// Copyright for the original CSS is 2010 Bernd Schoeler

#include <QStringBuilder>

#include "ptTheme.h"
#include "ptConstants.h"

////////////////////////////////////////////////////////////////////////////////
//
// Constructor.
//
////////////////////////////////////////////////////////////////////////////////

ptTheme::ptTheme(QApplication* Application) {
  ptSystemStyle = (QStyle*)(Application->style()->metaObject()->newInstance());
  ptSystemPalette = Application->palette();
#ifdef Q_OS_WIN32
  ptThemeStyle = new QWindowsXPStyle;
#else
  ptThemeStyle = new QCleanlooksStyle;
#endif
  ptStyleSheet = "";
#ifdef Q_OS_MAC
  MacBackGround ="";
  MacStyleFlag=false;
#endif
  ptPalette = QPalette();
  ptMenuPalette = QPalette();
  ptHighLight = QColor(255,255,255);
  ptStyle = ptSystemStyle;
  m_CustomCSS = "";

  // Icons
  ptIconCircleGreen = new QPixmap(QString::fromUtf8(":/dark/icons/indicator-allow.png"));
  ptIconCircleRed = new QPixmap(QString::fromUtf8(":/dark/icons/indicator-block.png"));
  ptIconCrossRed = new QPixmap(QString::fromUtf8(":/dark/icons/tool-hide.png"));
  ptIconCheckGreen = new QPixmap(QString::fromUtf8(":/dark/icons/tool-show.png"));
  ptIconReset =  new QPixmap(QString::fromUtf8(":/dark/icons/reset.png"));
  ptIconSavePreset =  new QPixmap(QString::fromUtf8(":/dark/icons/document-save.png"));
  ptIconAppendPreset =  new QPixmap(QString::fromUtf8(":/dark/icons/template-append.png"));
  ptIconQuestion =  new QPixmap(QString::fromUtf8(":/dark/ui-graphics/bubble-question.png"));
  ptIconStar =  new QPixmap(QString::fromUtf8(":/dark/icons/bookmark-new.png"));
  ptIconStarGrey =  new QPixmap(QString::fromUtf8(":/dark/icons/bookmark-remove.png"));
}

////////////////////////////////////////////////////////////////////////////////
//
// Reset the theme
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::Reset() {
  ptStyle = ptSystemStyle;
  ptPalette = ptSystemPalette;
  ptMenuPalette = ptSystemPalette;
  ptStyleSheet = "";
#ifdef Q_OS_MAC
  MacStyleFlag=false;
#endif
  m_CustomCSS = "";
}

////////////////////////////////////////////////////////////////////////////////
//
// Normal (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::Normal(const short Color) {
  SetHighLightColor(Color);
  ptStyle = ptSystemStyle;
  JustTools();
  ptPalette = ptSystemPalette;
  ptMenuPalette = ptSystemPalette;
#ifdef Q_OS_MAC
  MacStyleFlag=false;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// Mid grey (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::MidGrey(const short Color) {
  SetHighLightColor(Color);
  ptText = QColor(30,30,30);
  ptDark = QColor(180,180,180);
  ptBackground = QColor(127,127,127);
  ptGradient = QColor(150,150,150);
  ptBright = QColor(115,115,115);
  ptVeryBright = QColor(220,220,220);
  ptDisabled = QColor(190,190,190);
  ptSliderStart = QColor(155,155,155);
  ptSliderStop = QColor(145,145,145);
  ptSliderStartDisabled = QColor(150,150,150);
  ptSliderStopDisabled = QColor(150,150,150);
  SliderStripe = ":/patterns/slider-stripe-midgrey.png";
  SliderStripeDisabled = "";
#ifdef Q_OS_MAC
  MacStyleFlag=true;
#endif

  ptStyle = ptThemeStyle;
  CSS();

  // set the palette
  //ptPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptPalette.setColor(QPalette::Background, ptBackground);
  //ptPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptPalette.setColor(QPalette::Base, ptBackground); // Menu
  //ptPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
  //ptPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
  //ptPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Text, ptText);
  ptPalette.setColor(QPalette::Button, ptBackground); // Splitter
  // ptPalette.setColor(QPalette::ButtonText, ptText); // Menu
  //ptPalette.setColor(QPalette::BrightText, QColor(255,0,0)); //
  ptPalette.setColor(QPalette::Light, ptDark); // Splitter
  //ptPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptPalette.setColor(QPalette::Dark, ptBright);
  ptPalette.setColor(QPalette::Mid, ptDark);
  //ptPalette.setColor(QPalette::Shadow, QColor(255,0,0));
  ptPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptPalette.setColor(QPalette::HighlightedText, ptText);
  ptPalette.setColor(QPalette::Link, ptHighLight);
  ptPalette.setColor(QPalette::LinkVisited, ptHighLight);
  //ptPalette.setColor(QPalette::NoRole, QColor(255,0,0));

  //ptMenuPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Background, ptBackground);
  //ptMenuPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptMenuPalette.setColor(QPalette::Base, ptBackground); // Menu
  //ptMenuPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
  //ptMenuPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
  //ptMenuPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Text, ptText);
  ptMenuPalette.setColor(QPalette::Button, ptBackground); // Splitter
  ptMenuPalette.setColor(QPalette::ButtonText, ptText); // Menu
  //ptMenuPalette.setColor(QPalette::BrightText, QColor(255,0,0)); //
  ptMenuPalette.setColor(QPalette::Light, ptDark); // Splitter
  //ptMenuPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Dark, ptBright);
  ptMenuPalette.setColor(QPalette::Mid, ptDark);
  //ptMenuPalette.setColor(QPalette::Shadow, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptMenuPalette.setColor(QPalette::HighlightedText, ptText);
  //ptMenuPalette.setColor(QPalette::Link, QColor(255,0,0));
  //ptMenuPalette.setColor(QPalette::LinkVisited, QColor(255,0,0));
  //ptMenuPalette.setColor(QPalette::NoRole, QColor(255,0,0));
}

////////////////////////////////////////////////////////////////////////////////
//
// Dark grey (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::DarkGrey(const short Color) {
  SetHighLightColor(Color);
  ptText = QColor(240,240,240);
  ptDark = QColor(30,30,30);
  ptBackground = QColor(51,51,51);
  ptGradient = QColor(70,70,70);
  ptBright = QColor(115,115,115);
  ptVeryBright = QColor(180,180,180);
  ptDisabled = QColor(136,136,136);
  ptSliderStart = QColor(90,90,90);
  ptSliderStop = QColor(80,80,80);
  ptSliderStartDisabled = QColor(70,70,70);
  ptSliderStopDisabled = QColor(60,60,60);
  SliderStripe = ":/patterns/slider-stripe-grey.png";
  SliderStripeDisabled = ":/patterns/slider-stripe-darkgrey.png";
#ifdef Q_OS_MAC
  MacStyleFlag=true;
#endif

  ptStyle = ptThemeStyle;
  CSS();

  // set the palette
  //ptPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptPalette.setColor(QPalette::Background, ptBackground);
  //ptPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptPalette.setColor(QPalette::Base, ptBackground); // Menu
  //ptPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
  //ptPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
  //ptPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Text, ptText);
  ptPalette.setColor(QPalette::Button, ptBackground); // Splitter
  //ptPalette.setColor(QPalette::ButtonText, ptText); // Menu
  //ptPalette.setColor(QPalette::BrightText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Light, ptDark); // Splitter
  //ptPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptPalette.setColor(QPalette::Dark, ptDark);
  ptPalette.setColor(QPalette::Mid, ptDark);
  //ptPalette.setColor(QPalette::Shadow, QColor(255,0,0));
  ptPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptPalette.setColor(QPalette::HighlightedText, ptText);
  ptPalette.setColor(QPalette::Link, ptHighLight);
  ptPalette.setColor(QPalette::LinkVisited, ptHighLight);
  //ptPalette.setColor(QPalette::NoRole, QColor(255,0,0));

    //ptMenuPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Background, ptBackground);
    //ptMenuPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptMenuPalette.setColor(QPalette::Base, ptBackground); // Menu
    //ptMenuPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Text, ptText);
  ptMenuPalette.setColor(QPalette::Button, ptBackground); // Splitter
  ptMenuPalette.setColor(QPalette::ButtonText, ptText); // Menu
    //ptMenuPalette.setColor(QPalette::BrightText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Light, ptDark); // Splitter
    //ptMenuPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Dark, ptDark);
  ptMenuPalette.setColor(QPalette::Mid, ptDark);
    //ptMenuPalette.setColor(QPalette::Shadow, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptMenuPalette.setColor(QPalette::HighlightedText, ptText);
    //ptMenuPalette.setColor(QPalette::Link, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::LinkVisited, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::NoRole, QColor(255,0,0));
}

////////////////////////////////////////////////////////////////////////////////
//
// Very dark (with tint of the hovers)
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::VeryDark(const short Color) {
  SetHighLightColor(Color);
  ptText = QColor(200,200,200);
  ptDark = QColor(51,51,51);
  ptBackground = QColor(30,30,30);
  ptGradient = QColor(40,40,40);
  ptBright = QColor(70,70,70);
  ptVeryBright = QColor(150,150,150);
  ptDisabled = QColor(136,136,136);
  ptSliderStart = QColor(70,70,70);
  ptSliderStop = QColor(60,60,60);
  ptSliderStartDisabled = QColor(60,60,60);
  ptSliderStopDisabled = QColor(50,50,50);
  SliderStripe = ":/patterns/slider-stripe-darkgrey.png";
  SliderStripeDisabled = ":/patterns/slider-stripe-dark.png";
#ifdef Q_OS_MAC
  MacStyleFlag=true;
#endif

  ptStyle = ptThemeStyle;
  CSS();

  // set the palette
  //ptPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptPalette.setColor(QPalette::Background, ptBackground);
  //ptPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptPalette.setColor(QPalette::Base, ptBackground); // Menu
  //ptPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
  //ptPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
  //ptPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Text, ptText);
  ptPalette.setColor(QPalette::Button, ptBackground); // Splitter
  //ptPalette.setColor(QPalette::ButtonText, ptText); // Menu
  //ptPalette.setColor(QPalette::BrightText, QColor(255,0,0));
  ptPalette.setColor(QPalette::Light, ptDark); // Splitter
  //ptPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptPalette.setColor(QPalette::Dark, ptDark);
  ptPalette.setColor(QPalette::Mid, ptDark);
  //ptPalette.setColor(QPalette::Shadow, QColor(255,0,0));
  ptPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptPalette.setColor(QPalette::HighlightedText, ptText);
  ptPalette.setColor(QPalette::Link, ptHighLight);
  ptPalette.setColor(QPalette::LinkVisited, ptHighLight);
  //ptPalette.setColor(QPalette::NoRole, QColor(255,0,0));

    //ptMenuPalette.setColor(QPalette::Window, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Background, ptBackground);
    //ptMenuPalette.setColor(QPalette::WindowText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Foreground, ptDark); // QLabel QFrame
  ptMenuPalette.setColor(QPalette::Base, ptBackground); // Menu
    //ptMenuPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Text, ptText);
  ptMenuPalette.setColor(QPalette::Button, ptBackground); // Splitter
  ptMenuPalette.setColor(QPalette::ButtonText, ptText); // Menu
    //ptMenuPalette.setColor(QPalette::BrightText, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Light, ptDark); // Splitter
    //ptMenuPalette.setColor(QPalette::Midlight, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Dark, ptDark);
  ptMenuPalette.setColor(QPalette::Mid, ptDark);
    //ptMenuPalette.setColor(QPalette::Shadow, QColor(255,0,0));
  ptMenuPalette.setColor(QPalette::Highlight, ptDark); // -menu
  ptMenuPalette.setColor(QPalette::HighlightedText, ptText);
    //ptMenuPalette.setColor(QPalette::Link, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::LinkVisited, QColor(255,0,0));
    //ptMenuPalette.setColor(QPalette::NoRole, QColor(255,0,0));
}

////////////////////////////////////////////////////////////////////////////////
//
// CSS
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::CSS() {
  QString HighLight = "rgb(" % QString::number(ptHighLight.red()) % "," %
                               QString::number(ptHighLight.green()) % "," %
                               QString::number(ptHighLight.blue()) % ")";
  QString Text = "rgb(" % QString::number(ptText.red()) % "," %
                          QString::number(ptText.green()) % "," %
                          QString::number(ptText.blue()) % ")";
  QString BackGround = "rgb(" % QString::number(ptBackground.red()) % "," %
                                QString::number(ptBackground.green()) % "," %
                                QString::number(ptBackground.blue()) % ")";
  QString Dark = "rgb(" % QString::number(ptDark.red()) % "," %
                          QString::number(ptDark.green()) % "," %
                          QString::number(ptDark.blue()) % ")";
  QString Gradient = "rgb(" % QString::number(ptGradient.red()) % "," %
                              QString::number(ptGradient.green()) % "," %
                              QString::number(ptGradient.blue()) % ")";
  QString Bright = "rgb(" % QString::number(ptBright.red()) % "," %
                            QString::number(ptBright.green()) % "," %
                            QString::number(ptBright.blue()) % ")";
  QString VeryBright = "rgb(" % QString::number(ptVeryBright.red()) % "," %
                                QString::number(ptVeryBright.green()) % "," %
                                QString::number(ptVeryBright.blue()) % ")";
  QString Disabled = "rgb(" % QString::number(ptDisabled.red()) % "," %
                                QString::number(ptDisabled.green()) % "," %
                                QString::number(ptDisabled.blue()) % ")";
  QString SliderStart = "rgb(" % QString::number(ptSliderStart.red()) % "," %
                                QString::number(ptSliderStart.green()) % "," %
                                QString::number(ptSliderStart.blue()) % ")";
  QString SliderStop = "rgb(" % QString::number(ptSliderStop.red()) % "," %
                                QString::number(ptSliderStop.green()) % "," %
                                QString::number(ptSliderStop.blue()) % ")";
  QString SliderStartDisabled = "rgb(" % QString::number(ptSliderStartDisabled.red()) % "," %
                                         QString::number(ptSliderStartDisabled.green()) % "," %
                                         QString::number(ptSliderStartDisabled.blue()) % ")";
  QString SliderStopDisabled = "rgb(" % QString::number(ptSliderStopDisabled.red()) % "," %
                                        QString::number(ptSliderStopDisabled.green()) % "," %
                                        QString::number(ptSliderStopDisabled.blue()) % ")";
#ifdef Q_OS_MAC
  MacBackGround=BackGround;
#endif
  ptStyleSheet =
/************************************************************************************
Global colour definitions. Not needed for normal.css
************************************************************************************/
    "* {"
    "  background-color: " % BackGround % ";"
    "  color: " % Text % ";"
    "}"

    "*#ToolHeader {"
    "  background: " % Dark % ";"
    "}"

/************************************************************************************
Following stuff is also needed for normal.css
************************************************************************************/

/** Spinbox ****************************************/
    "QAbstractSpinBox {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid black;"
    "  selection-background-color: " % Dark % ";"
    "  selection-color: " % Text % ";"

    "  border-radius: 3px;"
    "  padding-left: 2px;"
    "  padding-right: 2px;"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "  max-width: 40px;"
    "  min-width: 40px;"
    "}"

    "QAbstractSpinBox:disabled {"
    "  color: " % Disabled % ";"
    "}"

    "QAbstractSpinBox:hover {"
    "  border: 1px solid " % HighLight % ";"
    "}"

    "QAbstractSpinBox::down-button {"
    "  height: 0;"
    "  width: 0;"
    "}"

    "QAbstractSpinBox::up-button {"
    "  height: 0;"
    "  width: 0;"
    "}"

    "ptSlider QAbstractSpinBox {"
    "  padding-bottom: 2px;"
    "  padding-top: 2px;"
    "  max-height: 16px;"
    "  min-height: 16px;"
    "  max-width: 80px;"
    "  min-width: 20px;"
    "}"


/** Checkbox ****************************************/
    "QCheckBox {"
    "  border: 1px solid " % BackGround % ";"

    "  border-radius: 3px;"
    "  padding: 2px;"
    "}"

    "QCheckBox:disabled {"
    "  color: " % Disabled % ";"
    "}"

    "QCheckBox:hover {"
    "  border: 1px solid " % HighLight % ";"
    "}"

    "QCheckBox::indicator {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "  height: 0.8em;"
    "  width: 0.8em;"
    "}"

    "QCheckBox::indicator:checked {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #555, stop:1 " % HighLight % ");"
    "}"


/** Combobox ****************************************/
    "QComboBox {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "  padding-left: 5px;"
    "  padding-right: 2px;"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "}"

    "QComboBox:disabled {"
    "  color: " % Disabled % ";"
    "}"

    "QComboBox:hover {"
    "  border: 1px solid " % HighLight % ";"
    "}"

    "QComboBox::drop-down {"
    "  border: none;"
    "  border-left: 1px solid " % Bright % ";"

    "  border-bottom-right-radius: 3px;"
    "  border-top-right-radius: 3px;"
    "  width: 10px;"
    "}"

    "QComboBox QAbstractItemView {"
    "  border: 1px solid " % VeryBright % ";"
    "  selection-background-color: " % Dark % ";"
    "  selection-color: " % Text % ";"

    "  border-radius: 3px;"
    "}"


/** Textedits ****************************************/
    "QLineEdit, QPlainTextEdit {"
    "  background-color: " % BackGround % ";"
    "  color: " % Text % ";"
    "  border: 1px solid " % VeryBright % ";"
    "  border-radius: 3px;"
    "  padding-left: 2px;"
    "  padding-right: 2px;"
    "}"

    "QLineEdit {"
    "  max-height: 1.1em;"
    "  min-height: 1.1em;"
    "}"

    "QLineEdit:disabled, QPlainTextEdit:disabled {"
    "  color: " % Disabled % ";"
    "}"

    "QLineEdit:hover, QPlainTextEdit:hover {"
    "  border: 1px solid " % HighLight % ";"
    "}"

    "QLineEdit#SearchInputWidget {"
    "  margin-left: 5px;"
    "}"

    "QLineEdit#m_PathInput {"
    "  padding-bottom: 2px;"
    "  padding-top: 2px;"
    "}"


/** QScrollArea ****************************************/
    "QScrollArea {"
    "  border: none;"
    "}"

/** tree view ****************************************/
    "QTreeView {"
    "  border: 1px solid " % Dark % ";"
    "}"

/** Slider, Progressbar ****************************************/
    "QSlider {"
    "  background: none;"
    "  border: 1px solid " % BackGround % ";"

    "  padding: 2px;"
    "  max-height: 8px;"
    "  min-height: 8px;"
    "}"

    "QSlider:hover {"
    "  border: 1px solid " % HighLight % ";"

    "  border-radius: 3px;"
    "}"

    "QSlider::groove:horizontal, QProgressBar::groove:horizontal {"
    "  background: " % VeryBright % ";"
    "  border: 1px solid #000;"

    "  border-radius: 3px;"
    "}"

    "QSlider::handle:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"

    "  width: 12px;"
    "}"

    "QSlider#HueSlider::groove:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 red, stop: 0.25 yellow, stop: 0.5 green, stop: 0.75 blue, stop: 1 red);"
    "}"

    "ptSlider, QProgressBar {"
    "  background: none;"
    "  border: 1px solid #000;"

    "  padding: 2px;"
    "  max-height: 16px;"
    "  min-height: 16px;"
    "  border-radius: 3px;"
    "}"

      "QProgressBar {"
      "  border-color: " % VeryBright % ";"
      "}"

    "QWidget#HueWidget {"
    "  max-height: 6px;"
    "  min-height: 6px;"
    "  border-radius: 3px;"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 red, stop: 0.25 yellow, stop: 0.5 green, stop: 0.75 blue, stop: 1 red);"
    "}"

    "ptSlider:disabled {"
    "  color: " % Disabled % ";"
    "}"

    "QWidget#HueWidget:disabled {"
    "  max-height: 6px;"
    "  min-height: 6px;"
    "  border-radius: 3px;"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 darkred, stop: 0.25 #808000, stop: 0.5 darkgreen, stop: 0.75 darkblue, stop: 1 darkred);"
    "}"

    "ptSlider::chunk, QProgressBar::chunk {"
    "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % SliderStart % ", stop: 1 " % SliderStop % ");"
    "  background-image: url(" % SliderStripe % ");"
    "  border-radius: 2px;"
    "  border: 1px solid " % ptSliderStart.darker(200).name() % ";"
#ifdef Q_OS_UNIX
    "  margin: 1px;"
#endif
    "}"

    "ptSlider::chunk:disabled {"
    "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % SliderStartDisabled % ", stop: 1 " % SliderStopDisabled % ");"
    "  background-image: url(" % SliderStripeDisabled % ");"
    "}"

/** Button ****************************************/
    "QToolButton {"
    "  border: none;"

    "  padding: 2px;"
    "}"

    "QToolButton:hover {"
    "  border: 1px solid " % HighLight % ";"

    "  border-radius: 3px;"
    "}"


/************************************************************************************
Everything from here on shouldn't be relevant for normal.css (I think ;) ...)
************************************************************************************/

      "#m_MainSplitter {"
      "  background-color: " % Dark % ";"
      "  padding: 10px;"
      "}"

      "QTreeView {"
      "  border-radius: 5px;"
      "  border: none;"
      "}"

      "#FMTreePane, #FMThumbPane {"
      "  border-radius: 5px;"
      "}"


/** Menu ****************************************/
    "QMenu {"
    "  background-color: " % BackGround % ";"
    "}"

    "QMenu::item:selected {"
    "  background-color: " % Bright % ";"
    "  color: " % Dark % ";"
    "}"

    "QMenu::separator {"
    "     height: 2px;"
    "     background: #555;"
    "     margin: 2px 5px 2px 5px;"
    "}"

    "QMenu::indicator, QMenu::indicator:non-exclusive:unchecked, QMenu::indicator:non-exclusive:checked {"
    "  background: " % BackGround % ";"
    "  border: 0px solid " % BackGround % ";"
    "}"


/** Scrollbar ****************************************/
    "QScrollBar:horizontal {"
    "  background: " % Dark % ";"

    "  border: none;"
    "  border-radius: 3px;"
    "  margin: 0 40px 0 0;"
    "}"

    "QScrollBar:vertical {"
    "  background: " % Dark % ";"

    "  border: none;"
    "  border-radius: 3px;"
    "  margin: 0 0 40px 0;"
    "}"

    "QScrollBar::handle:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-radius: 4px;"
    "  min-width: 20px;"
    "}"

    "QScrollBar::handle:vertical {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-radius: 4px;"
    "  min-height: 20px;"
    "}"

    "QScrollBar::handle:horizontal:hover, QScrollBar::handle:vertical:hover,"
    "QScrollBar::add-line:horizontal:hover, QScrollBar::add-line:vertical:hover,"
    "QScrollBar::sub-line:horizontal:hover, QScrollBar::sub-line:vertical:hover {"
    "  border: 1px solid " % HighLight % ";"
    "}"

    "QScrollBar::add-line:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-top-right-radius: 7px;"
    "  border-bottom-right-radius: 7px;"
    "  subcontrol-position: right;"
    "  subcontrol-origin: margin;"
    "  width: 16px;"
    "}"

    "QScrollBar::add-line:vertical {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-bottom-left-radius: 7px;"
    "  border-bottom-right-radius: 7px;"
    "  subcontrol-position: bottom;"
    "  subcontrol-origin: margin;"
    "  height: 16px;"
    "}"

    "QScrollBar::sub-line:horizontal {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
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
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " % Gradient % ", stop: 1 " % BackGround % ");"
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
    "  background-color: " % Dark % ";"
//    "  border: 1px solid #000;"
//    "  color: #000;"

    "  width: 4px;"
    "}"

    "QSplitter::handle:vertical {"
    "  background-color: " % Dark % ";"
//    "  border: 1px solid #000;"
//    "  color: #000;"

    "  height: 4px;"
    "}"


/** Tabs ****************************************/
    "QTabWidget::pane {"
    "  border-top: 0px solid qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #000, stop: 1 " % BackGround % ");"
    "}"

    "QTabWidget::tab-bar {"
    "  alignment: center;"
    "}"

    "QTabBar::tab {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Bright % ", stop: 1 " % BackGround % ");"
    "  border: 1px solid #000;"

    "  border-radius: 4px;"
    "  margin: 5px 1px 5px 1px;"
    "  padding: 4px;"
    "}"

    "QTabBar::tab:selected, QTabBar::tab:hover {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " % Bright % ", stop: 1 " % Dark % ");"
    "}"

    "QTabBar::tab:selected {"
    "  border-color: " % HighLight % ";"
    "}"

    "QTabBar::tab:!selected {"
    "  margin-top: 7px;"
    "}"

    /* Scrolling of tabs when not enough space */
    "QTabBar::tear {"
    "  background: " % BackGround % ";"
    "}"

    /* Special treatment for vertical tabs */
    "QTabWidget::pane {"
    "  border: none;"
    "}"

    "QTabWidget::tab-bar {"
    "  alignment: left;"
    "}"

    "QTabWidget QTabBar::tab {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " % Bright % ", stop: 1 " % Gradient % ");"
    "  margin: 1px 5px 1px 5px;"
    "}"

    "QTabWidget QTabBar::tab:selected, QTabWidget QTabBar::tab:hover {"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 " % Gradient % ", stop: 1 " % Dark % ");"
    "}"

    "QTabWidget QTabBar::tab:!selected {"
    "  margin-top: 1px;"
    "  margin-left: 7px;"
    "}"

/** Tooltips ****************************************/
    "QToolTip {"
    "  border: none;"   // needed to enable proper styling
    "  color: " % Text % ";"
    "}"

/******************************************/
    "*#StatusFrame {  /* status text bottom left without border */"
    "  border: 1px solid " % Dark % ";"
    "}"

    "*#StatusLabel {"
    "  color: " % Text % ";"
    "}" % m_CustomCSS;
}

////////////////////////////////////////////////////////////////////////////////
//
// Just tools
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::JustTools() {
  QString HighLight = "rgb(" % QString::number(ptHighLight.red()) % "," %
                               QString::number(ptHighLight.green()) % "," %
                               QString::number(ptHighLight.blue()) % ")";
  QString Disabled = "#888";
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
    "  border: 1px solid " % HighLight % ";"
    "}"

    "QAbstractSpinBox::down-button {"
    "  height: 0;"
    "  width: 0;"
    "}"

    "QAbstractSpinBox::up-button {"
    "  height: 0;"
    "  width: 0;"
    "}"

    "ptSlider QAbstractSpinBox {"
    "  padding-bottom: 2px;"
    "  padding-top: 2px;"
    "  max-height: 16px;"
    "  min-height: 16px;"
    "  max-width: 80px;"
    "  min-width: 20px;"
    "}"


/** Checkbox ****************************************/
    "QCheckBox {"
    "  border-radius: 3px;"
    "  padding: 2px;"
    "}"

    "QCheckBox:disabled {"
    "}"

    "QCheckBox:hover {"
    "  border: 1px solid " % HighLight % ";"
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
    "  color: " % Disabled % ";"
    "}"

    "QComboBox:hover {"
    "  border: 1px solid " % HighLight % ";"
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
    "  border: 1px solid " % HighLight % ";"
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
    "  border: 1px solid " % HighLight % ";"
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

    "ptSlider {"
    "  background: none;"
    "  border: 1px solid #000;"

    "  padding: 2px;"
    "  max-height: 16px;"
    "  min-height: 16px;"
    "  border-radius: 3px;"
    "}"

    "QWidget#HueWidget {"
    "  max-height: 6px;"
    "  min-height: 6px;"
    "  border-radius: 3px;"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 red, stop: 0.25 yellow, stop: 0.5 green, stop: 0.75 blue, stop: 1 red);"
    "}"

    "ptSlider:disabled {"
    "  color: " % Disabled % ";"
    "  border: 1px none"
    "}"

    "QWidget#HueWidget:disabled {"
    "  max-height: 6px;"
    "  min-height: 6px;"
    "  border-radius: 3px;"
    "  background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 darkred, stop: 0.25 #808000, stop: 0.5 darkgreen, stop: 0.75 darkblue, stop: 1 darkred);"
    "}"


/** Button ****************************************/
    "QToolButton {"
    "  border: none;"

    "  padding: 2px;"
    "}"

    "QToolButton:hover {"
    "  border: 1px solid " % HighLight % ";"

    "  border-radius: 3px;"
    "}"


/** Frame ****************************************/
    "#StatusFrame {  /* status text bottom left without border */"
    "  border: none;"
    "}" % m_CustomCSS;
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
// Set custom css
//
////////////////////////////////////////////////////////////////////////////////

void ptTheme::SetCustomCSS(const QString CSSFileName) {
  if (CSSFileName == "") {
    m_CustomCSS = "";
  } else {
    if (QFile::exists(CSSFileName)) {
      QFile *data;
      data = new QFile(CSSFileName);

      /* ...to open the file */
      if(data->open(QFile::ReadOnly)) {
        /* QTextStream... */
        QTextStream styleIn(data);
        /* ...read file to a string. */
        m_CustomCSS = styleIn.readAll();
        data->close();
      }
      delete data;
    }
  }
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


