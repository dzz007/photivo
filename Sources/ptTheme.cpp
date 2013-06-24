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

#include <cassert>

#include <QStyleFactory>
#include <QFile>
#include <QTextStream>

#include "ptDefines.h"
#include "ptTheme.h"
#include "ptConstants.h"
#include "ptSettings.h"

extern ptSettings* Settings;
extern QString     ShareDirectory;

//==============================================================================

ptTheme::ptTheme(const QApplication* app,
                 Theme newTheme /*= thDarkGrey*/,
                 Highlight newHighlight /*= hlGreen*/)
: IconAddBookmark (QPixmap(QString::fromUtf8(":/dark/icons/bookmark-new.png"))),
  IconGoNext      (QPixmap(QString::fromUtf8(":/dark/icons/go-next.png"))),
  IconGoPrevious  (QPixmap(QString::fromUtf8(":/dark/icons/go-previous.png"))),
  ColorNames(QStringList()
             << "AltBase"
             << "Base"
             << "Emphasized"
             << "Gradient"
             << "SliderStart"
             << "SliderStartDisabled"
             << "SliderStop"
             << "SliderStopDisabled"
             << "Strong"
             << "Text"
             << "TextDisabled"),
  GraphicsNames(QStringList() << "SliderStripe" << "SliderStripeDisabled"),
  m_CurrentHighlight(hlInvalid),
  m_CurrentTheme(thInvalid)
{
  assert(Settings != NULL);
  m_SystemStyle = (QStyle*)(app->style()->metaObject()->newInstance());
  m_SystemPalette = app->palette();
#ifdef Q_OS_WIN
  m_ThemeStyle = QStyleFactory::create("windows");
#else
  #if QT_VERSION >= 0x050000
    m_ThemeStyle = QStyleFactory::create("fusion");
  #else
    m_ThemeStyle = QStyleFactory::create("cleanlooks");
  #endif  
#endif
#ifdef Q_OS_MAC
  MacBackGround = "";
  MacStyleFlag = false;
#endif

  // Icons
  ptIconCircleGreen = new QPixmap(QString::fromUtf8(":/dark/icons/indicator-allow.png"));
  ptIconCircleRed = new QPixmap(QString::fromUtf8(":/dark/icons/indicator-block.png"));
  ptIconCrossRed = new QPixmap(QString::fromUtf8(":/dark/icons/tool-hide.png"));
  ptIconCheckGreen = new QPixmap(QString::fromUtf8(":/dark/icons/tool-show.png"));
  ptIconReset = new QPixmap(QString::fromUtf8(":/dark/icons/reset.png"));
  ptIconSavePreset = new QPixmap(QString::fromUtf8(":/dark/icons/document-save.png"));
  ptIconAppendPreset = new QPixmap(QString::fromUtf8(":/dark/icons/template-append.png"));
  ptIconQuestion = new QPixmap(QString::fromUtf8(":/dark/ui-graphics/bubble-question.png"));
  ptIconStar = new QPixmap(QString::fromUtf8(":/dark/icons/bookmark-new.png"));
  ptIconStarGrey = new QPixmap(QString::fromUtf8(":/dark/icons/bookmark-remove.png"));

  if (!SwitchTo(newTheme, newHighlight)) {
    m_CurrentTheme = thNone;
    m_CurrentHighlight = newHighlight;
    c_Highlight.setRgb(60,180,255);
  }
}

//==============================================================================

ptTheme::~ptTheme() {
  DelAndNull(m_SystemStyle);
  DelAndNull(m_ThemeStyle);
}

//==============================================================================

bool ptTheme::SwitchTo(Theme newTheme, Highlight newHighlight /*= hlBlue*/) {
  if (newTheme == m_CurrentTheme && newHighlight == m_CurrentHighlight) {
    // avoid expensive theme change if new setting are same as old ones
    return true;
  }

  // determine highlight colour
  QColor hlColor;
  switch (newHighlight) {
    case hlWhite:   hlColor.setRgb(255,255,255); break;
    case hlPurple:  hlColor.setRgb(140, 60,240); break;
    case hlBlue:    hlColor.setRgb( 60,180,255); break;
    case hlGreen:   hlColor.setRgb(100,255,100); break;
    case hlOrange:  hlColor.setRgb(255,180, 60); break;
    default:
      printf("Warning! Unhandled highlight colour index (%d). Defaulting to blue.\n", newHighlight);
      newHighlight = hlBlue;
      Settings->SetValue("StyleHighLight", (int)hlBlue);
      hlColor.setRgb(60,180,255);
      break;
  }

  // determine theme file name
  QString ThemeFileName = ShareDirectory + "Themes/";
  switch (newTheme) {
    case thNone:      Reset(); return true;   // Intentional! No theme, no more processing.
    case thShapeOnly: ThemeFileName += "ShapeOnly.ptheme"; break;
    case th50Grey:    ThemeFileName += "50Grey.ptheme"; break;
    case thDarkGrey:  ThemeFileName += "DarkGrey.ptheme"; break;
    case thNight:     ThemeFileName += "Night.ptheme"; break;
    default:
      printf("Warning! Unhandled theme index (%d). Defaulting to Dark Grey.\n", newTheme);
      newTheme = thDarkGrey;
      Settings->SetValue("Style", (int)thDarkGrey);
      ThemeFileName += "DarkGrey.ptheme";
      break;
  }

  // If SetupStylesheet() fails no members have been tampered with at this point.
  // I.e. we can safely return and still have an intact ptTheme.
  if (!SetupStylesheet(ThemeFileName, hlColor)) return false;

  // A successfull SetupStylesheet() updates stylesheet/QColor members. That leaves ...
  // ... the highlight color
  c_Highlight = hlColor;

  // ... the "current" flags
  m_CurrentTheme = newTheme;
  m_CurrentHighlight = newHighlight;

  // ... the mac hack
#ifdef Q_OS_MAC
  MacStyleFlag = true;
#endif

  // ... the palettes
  if (newTheme == thShapeOnly) {
    m_ThemePalette = m_SystemPalette;
    m_ThemeMenuPalette = m_SystemPalette;
  } else {
    //m_ThemePalette.setColor(QPalette::Window, QColor(255,0,0));
    m_ThemePalette.setColor(QPalette::Background, c_Base);
    //m_ThemePalette.setColor(QPalette::WindowText, QColor(255,0,0));
    m_ThemePalette.setColor(QPalette::Foreground, c_AltBase); // QLabel QFrame
    m_ThemePalette.setColor(QPalette::Base, c_Base); // Menu
    //m_ThemePalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
    //m_ThemePalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
    //m_ThemePalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
    m_ThemePalette.setColor(QPalette::Text, c_Text);
    m_ThemePalette.setColor(QPalette::Button, c_Base); // Splitter
    //m_ThemePalette.setColor(QPalette::ButtonText, c_Text); // Menu
    //m_ThemePalette.setColor(QPalette::BrightText, QColor(255,0,0));
    m_ThemePalette.setColor(QPalette::Light, c_AltBase); // Splitter
    //m_ThemePalette.setColor(QPalette::Midlight, QColor(255,0,0));
    m_ThemePalette.setColor(QPalette::Dark, c_AltBase);
    m_ThemePalette.setColor(QPalette::Mid, c_AltBase);
    //m_ThemePalette.setColor(QPalette::Shadow, QColor(255,0,0));
    m_ThemePalette.setColor(QPalette::Highlight, c_AltBase); // menu
    m_ThemePalette.setColor(QPalette::HighlightedText, c_Text);
    m_ThemePalette.setColor(QPalette::Link, c_Highlight);
    m_ThemePalette.setColor(QPalette::LinkVisited, c_Highlight);
    //m_ThemePalette.setColor(QPalette::NoRole, QColor(255,0,0));

    //m_ThemeMenuPalette.setColor(QPalette::Window, QColor(255,0,0));
    m_ThemeMenuPalette.setColor(QPalette::Background, c_Base);
    //m_ThemeMenuPalette.setColor(QPalette::WindowText, QColor(255,0,0));
    m_ThemeMenuPalette.setColor(QPalette::Foreground, c_AltBase); // QLabel QFrame
    m_ThemeMenuPalette.setColor(QPalette::Base, c_Base); // Menu
    //m_ThemeMenuPalette.setColor(QPalette::AlternateBase, QColor(255,0,0));
    //m_ThemeMenuPalette.setColor(QPalette::ToolTipBase, QColor(255,0,0));
    //m_ThemeMenuPalette.setColor(QPalette::ToolTipText, QColor(255,0,0));
    m_ThemeMenuPalette.setColor(QPalette::Text, c_Text);
    m_ThemeMenuPalette.setColor(QPalette::Button, c_Base); // Splitter
    m_ThemeMenuPalette.setColor(QPalette::ButtonText, c_Text); // Menu
    //m_ThemeMenuPalette.setColor(QPalette::BrightText, QColor(255,0,0));
    m_ThemeMenuPalette.setColor(QPalette::Light, c_AltBase); // Splitter
    //m_ThemeMenuPalette.setColor(QPalette::Midlight, QColor(255,0,0));
    m_ThemeMenuPalette.setColor(QPalette::Dark, c_AltBase);
    m_ThemeMenuPalette.setColor(QPalette::Mid, c_AltBase);
    //m_ThemeMenuPalette.setColor(QPalette::Shadow, QColor(255,0,0));
    m_ThemeMenuPalette.setColor(QPalette::Highlight, c_AltBase); // menu
    m_ThemeMenuPalette.setColor(QPalette::HighlightedText, c_Text);
    //m_ThemeMenuPalette.setColor(QPalette::Link, QColor(255,0,0));
    //m_ThemeMenuPalette.setColor(QPalette::LinkVisited, QColor(255,0,0));
    //m_ThemeMenuPalette.setColor(QPalette::NoRole, QColor(255,0,0));
  }

  return true;
}

//==============================================================================

bool ptTheme::SetupStylesheet(QString ThemeFileName, const QColor& highlightColor) {
  // read the complete ptheme file into a string
  QString data = ReadUTF8TextFile(ThemeFileName);
  if (data.isEmpty()) {
    printf("Theme engine: Could not read \"%s\"\n", ThemeFileName.toLocal8Bit().data());
    return false;
  }

  // process the theme file to get color variables
  QMap<QString, QString>* vars = new QMap<QString, QString>;
  int dataPos;
  if (!ParseTheme(data, vars, &dataPos)) {
    printf("Theme engine: Could not parse \"%s\"\n", ThemeFileName.toLocal8Bit().data());
    return false;
  }

  // Search&replace variables in the QSS. After the s&r qssData contains the
  // fully processed QSS string
  QString qssData = data.mid(dataPos);
  vars->insert("Highlight", highlightColor.name());
  if(!ReplaceColorVars(qssData, vars)) {
    printf("Theme engine: Could not replace color variables\n");
    return false;
  }

  m_Stylesheet = qssData;
  QMapIterator<QString, QString> i(*vars);
  while (i.hasNext()) {
    i.next();
    if (i.key() == "AltBase") {
      c_AltBase.setNamedColor(i.value());
    } else if (i.key() == "Base") {
      c_Base.setNamedColor(i.value());
    } else if (i.key() == "Emphasized") {
      c_Emphasized.setNamedColor(i.value());
    } else if (i.key() == "Gradient") {
      c_Gradient.setNamedColor(i.value());
    } else if (i.key() == "SliderStart") {
      c_SliderStart.setNamedColor(i.value());
    } else if (i.key() == "SliderStartDisabled") {
      c_SliderStartDisabled.setNamedColor(i.value());
    } else if (i.key() == "SliderStop") {
      c_SliderStop.setNamedColor(i.value());
    } else if (i.key() == "SliderStopDisabled") {
      c_SliderStopDisabled.setNamedColor(i.value());
    } else if (i.key() == "Strong") {
      c_Strong.setNamedColor(i.value());
    } else if (i.key() == "Text") {
      c_Text.setNamedColor(i.value());
    } else if (i.key() == "TextDisabled") {
      c_TextDisabled.setNamedColor(i.value());
    } else if (i.key() == "SliderStripe") {
      g_SliderStripe = i.value();
    } else if (i.key() == "SliderStripeDisabled") {
      g_SliderStripeDisabled = i.value();
    }
  }

#ifdef Q_OS_MAC
  MacBackGround = c_Base.name();
#endif

  DelAndNull(vars);
  return true;
}

//==============================================================================

bool ptTheme::ParseTheme(QString& data, QMap<QString, QString>* vars, int* dataPosition) {
  QString Variables;

  // Find and extract the @Variables section
  int dataPos = 0;
  int dataLen = data.count();
  while (dataPos < dataLen) {
    if (data[dataPos] == QChar('@')) {
      // possible start of a valid section
      if (dataPos < dataLen-9) {
        int end = data.indexOf("{", dataPos+1);   // find opening {
        if (end <= 0) return false;           // opening { not found
        if (data.mid(dataPos+1, end-dataPos-1).trimmed() == "Variables") {
          // find closing }
          dataPos = end+1;
          end = data.indexOf("}", dataPos);
          if (end <= 0) return false;
          // extract contents of @Variables section
          Variables = data.mid(dataPos, end-dataPos-1).simplified();
          dataPos = end+1;
          break;
        } else {
          return false;
        }
      }

    } else if (data[dataPos] == QChar('/')) {
      // possible start of a comment
      if (data.midRef(dataPos+1, 1) != "*") return false;  // not comment start: failure
      dataPos = data.indexOf("*/", dataPos+2);    // find end of comment
      if (dataPos == -1) return false;
      dataPos += 2;     // seek to after end of comment

    } else if (data[dataPos].isSpace()) {
      // ignore whitespace and continue with next character
      dataPos++;

    } else {
      // anything else except white space: parsing error
      return false;
    }
  }

  // Basic sanity check before further processing
  if (Variables.isEmpty()) return false;
  if (dataPos >= dataLen-1) return false;   // no content after @Variables section

  // dataPos now points to the position after the closing } of the @Variables section.
  // The caller needs this for further processing.
  *dataPosition = dataPos;

  if (Variables == "UseSystemColors") {
    // TODO: Using system colours is not implemented yet. It’s only needed for the
    // half broken "shapes only" theme anyway. Mabe fix later. For now just make it
    // not break the parser.
    for (int i = 0; i < ColorNames.count(); i++) {
      vars->insert(ColorNames.at(i), "#000000");
    }
    for (int i = 0; i < GraphicsNames.count(); i++) {
      vars->insert(GraphicsNames.at(i), "\"\"");
    }
    vars->insert("TextDisabled", "#888888");

  } else {
    // Extract colour/graphics variables as key/value pairs
    int pos = 0;
    int varsLen = Variables.count();
    while (pos < varsLen) {
      int i = Variables.indexOf(":", pos+1);
      if (i == -1) break;
      int k = Variables.indexOf(";", i+1);
      if (k == -1) break;
      vars->insert(Variables.mid(pos, i-pos).trimmed(), Variables.mid(i+1, k-i-1).trimmed());
      pos = k+1;
    }
  }

  // check that vars contains all needed variables (and only those) and they have valid values
  if (vars->count() != ColorNames.size()+GraphicsNames.size()) return false;
  QMutableMapIterator<QString, QString> i(*vars);
  while (i.hasNext()) {
    i.next();
    if (ColorNames.contains(i.key())) {
      // colours
      if (!QColor::isValidColor(i.value())) return false;
    } else if (GraphicsNames.contains(i.key())) {
      // graphical element (e.g. slider stripe)
      if(!(i.value().startsWith("\"") && i.value().endsWith("\""))) return false;
    } else {
      // not a valid variable name
      return false;
    }
  }

  // We created and validated the QMap with all variable names/values
  return true;
}

//==============================================================================

bool ptTheme::ReplaceColorVars(QString& data, QMap<QString, QString>* vars) {
  data = data.simplified();
  if (data.left(7) != "@import") return false;
  int start = data.indexOf("\"", 7);
  if (start <= 0) return false;
  int end = data.indexOf("\"", start+1);
  QString FileBase = ShareDirectory % "Themes/" % data.mid(start+1, end-start-1);

  data = ReadUTF8TextFile(FileBase % ".qss");
  if (data.isEmpty()) return false;

  // append optional OS dependent styles
#ifdef Q_OS_WIN
  data.append(ReadUTF8TextFile(FileBase % ".windows.qss"));
#elif defined Q_OS_LINUX
  data.append(ReadUTF8TextFile(FileBase % ".linux.qss"));
#elif defined Q_OS_MAC
  data.append(ReadUTF8TextFile(FileBase % ".mac.qss"));
#endif

  // the actual search & replace
  QMapIterator<QString, QString> i(*vars);
  while (i.hasNext()) {
    i.next();
    QString varName = "$" % i.key() % "$";
    data = data.replace(varName, i.value());
  }

  ////////////////// for testing: dump final stylesheet
//  QFile f("test.qss");
//  f.open(QIODevice::WriteOnly);
//  f.write(data.toUtf8());
//  f.close();
  //////////////////////////////////////////////////

  return true;
}

//==============================================================================

QString ptTheme::ReadUTF8TextFile(const QString& FileName) {
  QString result;
  if (!QFile::exists(FileName)) return result;
  QFile TheFile(FileName);
  if (!TheFile.open(QIODevice::ReadOnly)) return result;
  QTextStream stream(&TheFile);
  stream.setCodec("UTF-8");
  result = stream.readAll();
  TheFile.close();
  return result;
}

//==============================================================================

QPalette ptTheme::menuPalette() {
  if (m_CurrentTheme == thNone) {
    return m_SystemPalette;
  } else {
    return m_ThemeMenuPalette;
  }
}

//==============================================================================

QPalette ptTheme::palette() {
  if (m_CurrentTheme == thNone) {
    return m_SystemPalette;
  } else {
    return m_ThemePalette;
  }
}

//==============================================================================

QStyle* ptTheme::style() const {
  if (m_CurrentTheme == thNone) {
    return m_SystemStyle;
  } else {
    return m_ThemeStyle;
  }
}

//==============================================================================

void ptTheme::setCustomCSS(const QString& CSSFileName) {
  m_CustomCSS = ReadUTF8TextFile(CSSFileName);
}

//==============================================================================

void ptTheme::Reset() {
  m_Stylesheet.clear();
  m_CustomCSS.clear();
  m_CurrentTheme = thNone;

#ifdef Q_OS_MAC
  MacStyleFlag = false;
#endif
}

//==============================================================================
