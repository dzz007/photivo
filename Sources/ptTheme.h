/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2010 Michael Munzert <mail@mm-log.com>
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

#ifndef DLTHEME_H
#define DLTHEME_H

//==============================================================================

#include <QtCore>
#include <QtGui>

//==============================================================================

class ptTheme {
public:
  ptTheme(QApplication* Application);
  ~ptTheme();

  void      Reset();
  void      Normal(const short Color);
  void      MidGrey(const short Color);
  void      DarkGrey(const short Color);
  void      VeryDark(const short Color);
  void      SetHighLightColor(const short Color);
  void      SetCustomCSS(const QString CSSFileName);

  // Variables
  QStyle*     ptThemeStyle;
  QStyle*     ptStyle;
  QPalette    ptPalette;
  QPalette    ptMenuPalette;
  QString     ptStyleSheet;

  #ifdef Q_OS_MAC
    QString     MacBackGround;
    bool        MacStyleFlag;
  #endif

  QPalette    ptSystemPalette;
  QStyle*     ptSystemStyle;

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

  QColor      ptBackground;


private:
  void CSS();
  void JustTools();
  QColor ptHighLight;
  QColor ptGradient;
  QColor ptText;
  QColor ptDark;
  QColor ptBright;
  QColor ptVeryBright;
  QColor ptDisabled;
  QColor ptSliderStart;
  QColor ptSliderStop;
  QColor ptSliderStartDisabled;
  QColor ptSliderStopDisabled;

  QString SliderStripe;
  QString SliderStripeDisabled;

  QString m_CustomCSS;

//==============================================================================
};
#endif
