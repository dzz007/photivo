/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
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
#ifndef PTFILTERBASE_H
#define PTFILTERBASE_H

#include "ptFilterConfig.h"
#include "ptTempFilterBase.h"
#include <QObject>
#include <QList>
#include <QVariant>
#include <QFlags>

class ptImage;
class ptWidget;
class ptToolBox;
class QWidget;
class QSettings;

//------------------------------------------------------------------------------
/*! Base class for all filters. Must not be instantiated directly. */
class ptFilterBase: /*public QObject,*/ public ptTempFilterBase {
Q_OBJECT

public:
  /*! The TFilterFlag enum describes the capabilites of a filter.
      The flags determine which entries appear in the toolbox context menu.
      IMPORTANT: When you change TFilterFlag do not forget to change the default implementation
      of flags() as well!
   */
  enum TFilterFlag {
    NoFilterFlags         = 0,
    FilterIsBlockable     = 1,
    FilterHasDefault      = 2,
    FilterIsSaveable      = 4,
    FilterIsFavouriteable = 8,
    FilterIsHideable      = 16,
  };

  /*! Standard Qt typedef to make OR combinations of \c TFilterFlag possible. */
  typedef QFlags<TFilterFlag> TFilterFlags;

public:
  virtual ~ptFilterBase();              //!< Destroys a ptFilterBase object.

  virtual TFilterFlags  flags() const;  //!< Returns the filter’s capabilities as an OR combination of \c TFilterFlag values.
  ptToolBox*            gui();          //!< Returns the GUI toolbox.

  void    exportPreset(QSettings *APreset, const bool AIncludeFlags = true) const;
  void    importPreset(QSettings *APreset, const bool ARequestPipeRun = false);
  void    init(const QString &AUniqueName, const QString &AGuiNamePostfix);
  void    reset(const bool ARequestPipeRun = false);
  void    runFilter(ptImage *AImage) const;


  /*! \name Status getters and setters *//*! @{*/
  bool    hasActiveCfg() const;
  bool    isActive() const;
  bool    isBlocked() const;
  bool    setBlocked(const bool AIsBlocked);
  bool    isHidden() const;
  bool    setHidden(const bool AIsHidden);
  /*! @}*/

  /*! \name Other getters and setters *//*! @{*/
  QString   caption()        const;           //!< \see init()
  bool      hasHelp()        const;           //!< Returns \c true if the filter has a help URL.
  QString   helpUri()        const;           //!< Valid URL to the help page for this filter on photivo.org.
  bool      isFavourite()    const;           //!< Returns \c true when the filter is in the favourites list, \c false otherwise.
  bool      setFavourite(bool AIsFavourite);  //!< Adds or removes the filter from the list of favourites.
  bool      isSlow()         const;           //!< If \c true the toolbox header will get an icon marking the filter as computationally expensive.
  int       idxInParentTab() const;           //!< \see init()
  int       parentTabIdx()   const;           //!< \see init()
  void      setPos(int ATab, int AIdx);       //!< Sets the filter’s position in the pipe
  QString   uniqueName()     const;           //!< Returns the filter’s unique name.
  /*! @}*/

  /*! Deprecated! These functions are strictly for compatibility with old ptGroupBox specific code.
      Will disappear once the old groupbox system is gone. DO NOT USE except for said compatibility
      stuff. You have been warned! *//*! @{*/
  QWidget* guiWidget();
  bool canHide() const;
  /*! @}*/

signals:
  void activityChanged();   //!< Emitted whenever the filter’s isActive() status changes.

protected:
  static ptWidget* createWidgetByType(const ptCfgItem &ACfgItem, QWidget *AParent);

protected:
  ptFilterBase();       //!< Ctor is only usable by derived classes.

  bool              checkActiveChanged(const bool ANoSignal = false);
  void              connectCommonDispatch();
  int               cfgIdx(const QString &AId) const;
  void              initDesignerGui(QWidget *AGuiBody);
  void              internalInit();
  ptWidget         *findPtWidget(const QString &AId, QWidget* AWidget);

// Pragmas are here to stop the compiler complaining about unused parameters in the default
// implementations. Removing the parameter names would work too but be too obscure.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
  virtual QWidget  *doCreateGui() { return nullptr; }   //!< Derived classes must reimplement this method when they use a custom GUI layout, e.g. created via an .ui file. The result has to be \c true if a gui is created.
  virtual bool      doCheckHasActiveCfg() = 0;          //!< Determines if the filter is active in the pipe. Derived classes should reimplement this function to determine the activity status of the filter.
  virtual void      doExportCustomConfig(QSettings *APreset, const bool AIncludeFlags) const {}
  virtual void      doImportCustomConfig(QSettings *APreset) {}
  virtual void      doUpdateGui() {}                          //!< Update for the children.
  virtual void      doRunFilter(ptImage *AImage) const = 0;   //!< Children should do the work.
  virtual void      doReset() {}                              //!< Reset for the children
  virtual void      doDefineControls() = 0;                   //!< Children know which controls they need.
#pragma GCC diagnostic pop

  ptFilterConfig    FConfig;
  QString           FFilterName;
  QString           FCaption;
  QString           FHelpUri;
  bool              FHasActiveCfg;
  bool              FIsSlow;
  ptToolBox*        FGuiContainer;

protected slots:
  void commonDispatch(const QString AId, const QVariant ANewValue);

private:
  void      performCommonConnect(const ptCfgItem &ACfgItem, QObject *AObject);
  void      requestPipeRun(const bool AUnconditional = false);
  void      updateGui(const bool ARequestPipeRun = true);
  void      createGui();

  QString   FUniqueName;
  QString   FCaptionPostfix;
  bool      FIsActive;
  int       FParentTabIdx;
  int       FIdxInParentTab;
  bool      FIsBlocked;
  bool      FPreventPipeRun;
};

//------------------------------------------------------------------------------

/*! Qt macro that defines \c operator|() for \c TFilterFlags. */
Q_DECLARE_OPERATORS_FOR_FLAGS(ptFilterBase::TFilterFlags)

/*! \typedef ptFilterFactoryMethod
  Type to encapsulate the function pointers to the static factory classes. */
typedef ptFilterBase* (*ptFilterFactoryMethod)();

/*! \class RegisterHelper
  Just a little helper to get the registering done automatically. */
class RegisterHelper {
public:
  RegisterHelper(const ptFilterFactoryMethod AMethod,
                 const QString               AName);
  ~RegisterHelper() {}
};

//==============================================================================

#endif // PTFILTERBASE_H
