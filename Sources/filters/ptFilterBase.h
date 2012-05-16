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

//==============================================================================

#include <memory>

#include <QObject>
#include <QList>
#include <QVariant>
#include <QFlags>

#include "ptFilterConfig.h"
#include <ptTempFilterBase.h>

// forward
class QWidget;
class QSettings;

class ptCfgItem;
class ptImage;
class ptWidget;
class ptToolBox;

//==============================================================================

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
  /*! Destructor */
  virtual ~ptFilterBase();


  /*! Returns the filter’s capabilities as an OR combination of \c TFilterFlag values.
      Derived classes must reimplement this function to return the appropriate flags.
      The default implementation enables all flags.
   */
  virtual TFilterFlags  flags() const;

  /*! Returns the GUI toolbox. */
  ptToolBox*            gui();

  /*! Export filter preset to a \c QSettings structure.
      \param AIni
        A valid pointer to a \c QSettings object.
      \param AIncludeFlags
        When \c true includes status flags (like isBlocked status) in the exported preset.
   */
  void                  exportPreset(QSettings *APreset, const bool AIncludeFlags = true) const;

  /*! Import preset from \c APreset. */
  void                  importPreset(QSettings *APreset, const bool ARequestPipeRun = false);

  /*! Initializes the filter. Is called automatically by `ptFilterDM::NewFilter()`.
      You probably never need to call this function manually.
      \param ACaption
        The text visible in the toolbox header. Caller is responsible for the translation to
        the user’s chosen language.
      \param AParentTabIdx
        Index of the processing tab the toolbox resides in.
      \param AIdxInParentTab
        Index in the toolbox list inside the parent tab.
   */
  void                  init(const QString &AUniqueName,
                             const QString &AGuiNamePostfix);

  /*! Resets the filter to default values. */
  void                  reset(const bool ARequestPipeRun = false);

  /*! Executes the filter on \c AImage. */
  void                  runFilter(ptImage *AImage) const;


  /*! \group Status getters and setters */
  ///@{
  /*! A filter has an active config when it is configured to do processing on the image.
      That does not imply that the filter is really performing any processing. E.g. it might be
      blocked or hidden.
      \return \c True when the filter has an active config, \c false otherwise.
   */
  bool                  hasActiveCfg() const { return FHasActiveCfg; }

  /*! An active filter really performs operations on the image when the pipe runs.
      I.e. it has an active config, is not blocked and not hidden. If you need to know if a
      filter will process the image when the pipe runs use this function.
      \return \c True when the filter is ready for action, \c false otherwise.
      \see ptFilterDM’s FPipeActiveFilters list
   */
  bool                  isActive() const { return FIsActive; }

  /*! A blocked filter is prevented from doing any processing by the user.
      \return \c True when the filter is blocked, \c false otherwise.
   */
  bool                  isBlocked() const { return FIsBlocked; }

  /*! Blocks or unblocks the filter. Blocking is a user action triggered by the *Block*
      context menu entry. Never use \c setBlocked() if you need to prevent a filter from doing
      any processing programmatically.
      \return \c True when the function succeeded (only blockable filters can be blocked),
        \c false otherwise.
      \see flags()
   */
  bool                  setBlocked(const bool AIsBlocked);

  /*! A hidden filter is not visible in the GUI and does not do any processing.
      \return \c True when the filter is hidden, \c false otherwise
   */
  bool                  isHidden() const;

  /*! Hides or unhides the filter. Hiding is a user action triggered by the *Hide*
      context menu entry. Never use \c setHidden() if you need to prevent a filter from doing
      any processing programmatically.
      \return \c True when the function succeeded (only hideable filters can be hidden),
        \c false otherwise.
      \see flags(), the ptSettings "HiddenTools" list
  */
  bool                  setHidden(const bool AIsHidden);
  ///@}


  /*! \group Other getters and setters */
  ///@{
  /*! \see init() */
  QString               caption()        const { return FCaption + FCaptionPostfix; }

  /*! Returns \c true if the filter has a help URL. */
  bool                  hasHelp()        const { return !FHelpUri.isEmpty(); }

  /*! Valid URL to the help page for this filter on photivo.org. */
  QString               helpUri()        const { return FHelpUri; }

  /*! Returns \c true when the filter is in the favourites list, \c false otherwise. */
  bool                  isFavourite()    const;

  /*! Adds or removes the filter from the list of favourites.
      \return \c True when the function succeeded (only favouritable filters can be set favourite),
        \c false otherwise.
      \see flags(), the ptSettings "FavouriteTools" list
   */
  bool                  setFavourite(const bool AIsFavourite);

  /*! If \c true the toolbox header will get an icon marking the filter as
      computationally expensive.
   */
  bool                  isSlow()         const { return FIsSlow; }

  /*! \see init() */
  int                   idxInParentTab() const { return FIdxInParentTab; }

  /*! \see init() */
  int                   parentTabIdx()   const { return FParentTabIdx; }

  void                  setPos(const int ATab, const int AIdx);

  /*! Returns the filter’s unique name. */
  QString               uniqueName() const;
  ///@}

  //---
  /*! Deprecated! These functions are strictly for compatibility with old ptGroupBox specific code.
      Will disappear once the old groupbox system is gone. DO NOT USE except for said compatibility
      stuff. You have been warned!
  */
  QWidget* guiWidget();
  bool canHide() const;
  //---


protected:
  /*! Creates a Photivo custom widget and returns a pointer to the widget.
      \param ACfgItem
        The config data for the new widget. ACfgItem.Type determines the concrete type of the
        created object.
      \param AParent
        The new widget’s parent is set to \c AParent.
   */
  static ptWidget* createWidgetByType(const ptCfgItem &ACfgItem, QWidget *AParent);
  

protected:
  /*! Constructor */
  ptFilterBase();


  /*! Returns \c true if the status has changed since the last run of the method
      and \c false otherwise. Also sets \c FIsActive.

      Derived classes should reimplement \c doCheckIsActive() to determine the activity
      status of the filter.
      \param ANoSignal
        When set to \c true the \c activityChanged() signal is not emitted, even when the
        activity status changed. This is useful in certain scenarios to update the GUI correctly.
        The default is \c false, i.e. the signal is emitted when the activity status changed.
        This parameter does not affect the return value.
   */
  bool              checkActiveChanged(const bool ANoSignal = false);

  /*! Connects the commonDispach() slot to all applicable controls. The default implementation
      connects all controls from the common controls lists that have their \c UseCommonDispatch
      flag set to \c true.
  */
  void              connectCommonDispatch();

  int               cfgIdx(const QString &AId) const;

  /*! Initializes a Qt Designer created GUI with the values from the config items list.
      Only needed when you create the GUI with Designer. Call once from \c doCreateGui().
      Do not use for reset-to-default-values, use \c reset() instead.
      Also connects the common dispatcher for controls from the config items list that have
      their UseCommonConnect flag set to \c true. I.e. you should *not* call both
      \c connectCommonDispatch() and \c initDesignerGui().
      \param AGuiBody
        A valid pointer to the widget containing the uninitialized GUI controls.
   */
  void              initDesignerGui(QWidget *AGuiBody);

  /*! Init: Every derived class has to call it in its constructor.*/
  void              internalInit();

// Pragmas are here to stop the compiler complaining about unused parameters in the default
// implementations. Removing the parameter names would work too but be too obscure.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
  /*! Derived classes must reimplement this method when they use a custom GUI layout,
      e.g. created via an .ui file. The result has to be \c true if a gui is created.
   */
  virtual QWidget  *doCreateGui() { return nullptr; }

  /*! Determines if the filter is active in the pipe.
      Derived classes should reimplement this function to determine the activity
      status of the filter.
   */
  virtual bool      doCheckHasActiveCfg() = 0;

  virtual void      doExportCustomConfig(QSettings *APreset, const bool AIncludeFlags) const {}

  virtual void      doImportCustomConfig(QSettings *APreset) {}

  /*! Update for the children. */
  virtual void      doUpdateGui() {}

  /*! Children should do the work. */
  virtual void      doRunFilter(ptImage *AImage) const = 0;

  /*! Reset for the children */
  virtual void      doReset() {}

  /*! Children know which controls they need. */
  virtual void      doDefineControls() = 0;

  /*! Children may need additional config entries.
      Derived classes may reimplement this method to add custom entries to \c FConfig
      or their own additional config data structure.
   */
  virtual void      doAddCustomConfig(TConfigStore &AConfig) {}
#pragma GCC diagnostic pop

  QString           FFilterName;
  QString           FCaption;
  QString           FHelpUri;
  bool              FHasActiveCfg;
  bool              FIsSlow;
  std::unique_ptr<ptFilterConfig>  FConfig;
  ptToolBox        *FGuiContainer;
  QList<ptCfgItem>  FCfgItems;


protected slots:
  /*! Common dispatcher interfacing from UI control to config entry. Implemented as
      a slot that connects to the control’s valueChanged() signal.
      \param Id
        The UI control’s ID string.
      \param ANewValue
        The new value as a \c QVariant.
   */
  void commonDispatch(const QString AId, const QVariant ANewValue);


private:
  /*! Creates the \c FConfig object and initialises it with the default key/value pairs
      from the common controls lists.
   */
  void            createConfig();

  /*! Helper method that performs the commonDispatch() connection for \c AObject
      if the object exists and \c ACfgItem.UseCommonDispatch is \c true. */
  void            performCommonConnect(const ptCfgItem &ACfgItem, QObject *AObject);

  /*! Must be called whenever the filter needs to run. */
  void            requestPipeRun(const bool AUnconditional = false);

  /*! Updates all widgets with values from config. Also takes care of activity status and
      pipe run request. */
  void            updateGui(const bool ARequestPipeRun = true);

  /*! Creates the GUI. The default implementation creates the \c FUIContainer widget,
      puts all controls from the common controls lists in a vertical layout and
      connects the common dispatcher where needed. */
  void            createGui();

  /*! QVariants read from preset files are often interpreted as strings even if they’re not.
      This function casts to the intended type or raises an error if not possible.
      \param AValue
        A reference to the QVariant to validate.
      \param AIntendedType
        The intended type.
   */
  void            ensureVariantType(QVariant &AValue, const QVariant::Type &AIntendedType);

  /*! Checks data type of a config item.
      \param ACfgItem
        The config item to check.
      \return
        The item’s variant type or raises an error if the type could not be determined.
   */
  QVariant::Type  variantType(const ptCfgItem &ACfgItem);

  QString         FUniqueName;
  QString         FCaptionPostfix;
  bool            FIsActive;
  int             FParentTabIdx;
  int             FIdxInParentTab;
  bool            FIsBlocked;


signals:
  /*! Emitted whenever the filter’s isActive() status changes. */
  void activityChanged();

};

//==============================================================================

/*! Qt macro that defines \c operator|() for \c TFilterFlags. */
Q_DECLARE_OPERATORS_FOR_FLAGS(ptFilterBase::TFilterFlags)

//==============================================================================



/*! \typedef ptFilterFactoryMethod
  Type to encapsulate the function
  pointers to the static factory classes.
  */
typedef ptFilterBase* (*ptFilterFactoryMethod)();

//==============================================================================

/*! \class RegisterHelper
  Just a little helper to get the registering done automatically.
  */
class RegisterHelper
{
public:
  RegisterHelper(const ptFilterFactoryMethod AMethod,
                 const QString               AName);
  ~RegisterHelper() {}
};

//==============================================================================

#endif // PTFILTERBASE_H
