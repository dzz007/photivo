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

#ifndef PTTHEME_H
#define PTTHEME_H

//==============================================================================

#include <QApplication>
#include <QStringBuilder>
#include <QMap>
#include <QColor>
#include <QPalette>
#include <QStyle>

//==============================================================================

class ptTheme {
public:
  // Values correspond to the UI combobox indexes.
  enum Theme {
    thInvalid   = -1,   // only needed for constructor
    thNone      = 0,
    thShapeOnly = 1,
    th50Grey    = 2,
    thDarkGrey  = 3,
    thNight     = 4
  };

  enum Highlight {
    hlInvalid = -1,   // only needed for constructor
    hlWhite   = 0,
    hlPurple  = 1,
    hlBlue    = 2,
    hlGreen   = 3,
    hlOrange  = 4
  };

  ptTheme(const QApplication* app, Theme newTheme = thDarkGrey, Highlight newHighlight = hlGreen);
  ~ptTheme();

  // TODO: make private and write getter
  QPixmap*    ptIconCircleGreen;
  QPixmap*    ptIconCircleRed;
  QPixmap*    ptIconCrossRed;
  QPixmap*    ptIconCheckGreen;
  QPixmap*    ptIconReset;
  QPixmap*    ptIconSavePreset;
  QPixmap*    ptIconAppendPreset;
  QPixmap*    ptIconQuestion;
  QPixmap*    ptIconStar;
  QPixmap*    ptIconStarGrey;
  const QPixmap IconAddBookmark;
  const QPixmap IconGoNext;
  const QPixmap IconGoPrevious;

  QColor baseColor()                { return c_Base; }
  QColor altBaseColor()             { return c_AltBase; }
  QColor emphasizedColor()          { return c_Emphasized; }
  QColor strongColor()              { return c_Strong; }
  QColor textColor()                { return c_Text; }
  QColor textDisabledColor()        { return c_TextDisabled; }
  QColor gradientColor()            { return c_Gradient; }
  QColor sliderStartColor()         { return c_SliderStart; }
  QColor sliderStopColor()          { return c_SliderStop; }
  QColor sliderStartDisabledColor() { return c_SliderStartDisabled; }
  QColor sliderStopDisabledColor()  { return c_SliderStopDisabled; }
  QColor highlightColor()           { return c_Highlight; }

  QPalette  menuPalette   ();
  QPalette  palette       ();
  void      Reset         ();
  void      setCustomCSS  (const QString& CSSFileName);
  QStyle*   style         () const;
  QString   stylesheet    () { return m_Stylesheet % m_CustomCSS; }
  bool      SwitchTo      (Theme newTheme, Highlight newHighlight = hlBlue);
  QPalette  systemPalette () { return m_SystemPalette; }
  QStyle*   systemStyle   () const { return m_SystemStyle; }

#ifdef Q_OS_MAC
  QString     MacBackGround;
  bool        MacStyleFlag;
#endif


private:
  bool    ParseTheme      (QString& data, QMap<QString, QString>* vars, int* dataPos);
  QString ReadUTF8TextFile(const QString& FileName);
  bool    ReplaceColorVars(QString& data, QMap<QString, QString>* vars);
  bool    SetupStylesheet (QString ThemeFileName, const QColor& highlightColor);

  const QStringList  ColorNames;
  const QStringList  GraphicsNames;

  Highlight m_CurrentHighlight;
  Theme     m_CurrentTheme;
  QString   m_CustomCSS;
  QString   m_Stylesheet;
  QPalette  m_SystemPalette;
  QStyle*   m_SystemStyle;
  QPalette  m_ThemeMenuPalette;
  QPalette  m_ThemePalette;
  QStyle*   m_ThemeStyle;

  // colours
  QColor c_Base;      // formerly: Background
  QColor c_AltBase;   // formerly: Dark
  QColor c_Emphasized;    // formerly: Bright
  QColor c_Strong;        // formerly: VeryBright
  QColor c_Text;
  QColor c_TextDisabled;  // formerly: Disabled
  QColor c_Gradient;
  QColor c_SliderStart;
  QColor c_SliderStop;
  QColor c_SliderStartDisabled;
  QColor c_SliderStopDisabled;
  QColor c_Highlight;

  // other graphical elements
  QString g_SliderStripe;
  QString g_SliderStripeDisabled;

};

//==============================================================================

extern ptTheme *Theme;

//==============================================================================

#endif  // PTTHEME_H
