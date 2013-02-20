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

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>

#include "ptInfo.h"

#include "ptCfgItem.h"
#include "ptFilterBase.h"
#include "ptFilterFactory.h"
#include "ptFilterDM.h"
#include <ptCheck.h>
#include <ptChoice.h>
#include <ptWidget.h>
#include <ptInput.h>
#include <ptToolBox.h>
#include <ptCurveWindow.h>

#include <ptSettings.h>

//==============================================================================

const QString CCustomStores = "CustomStores";
const QString CIsBlocked    = "isBlocked";

//==============================================================================

ptFilterBase::~ptFilterBase() {
  if(FGuiContainer) {
    /* NOTE: Unfortunately Qt combines GUI parent and resource ownership. Might cause problems
    with destruction order when Photivo closes down. We don’t want the main UI trying to access
    a filter GUI widget that doesn’t exist anymore – and vice versa. Definitely needs thorough
    testing.*/
    FGuiContainer->setParent(nullptr);
    DelAndNull(FGuiContainer);
  }
}

//==============================================================================

/*! Export filter preset to a \c QSettings structure.
    \param AIni
      A valid pointer to a \c QSettings object.
    \param AIncludeFlags
      When \c true includes status flags (like isBlocked status) in the exported preset.
 */
void ptFilterBase::exportPreset(QSettings *APreset, const bool AIncludeFlags /*= true*/) const {
  APreset->beginGroup(this->FFilterName + "/" + this->uniqueName());
  APreset->remove("");   // remove potentially existing settings in AIni

  // store default TFilterConfig from FCfgItems list
  for (ptCfgItem hSetting: FCfgItems) {
    if (hSetting.Storeable) {
      APreset->setValue(hSetting.Id, makeStorageFriendly(FConfig->getValue(hSetting.Id)));
    }
  }

  // store additional custom config lists (e.g. used for curve anchors)
  // First find out if the available stores should go into the config file
  QStringList hStorableStores;
  for (QString hId: FConfig->simpleStoreIds()) {
    hStorableStores.append(hId);
  }

  // save list of store names and stores’ contents
  if (!hStorableStores.isEmpty()) {
    APreset->setValue(CCustomStores, hStorableStores);

    for (QString hId: hStorableStores) {
      TConfigStore *hList = FConfig->getSimpleStore(hId);
      APreset->beginGroup(hId);
      for (auto hItem = hList->constBegin(); hItem != hList->constEnd(); ++hItem) {
        APreset->setValue(hItem.key(), makeStorageFriendly(hItem.value()));
      }
      APreset->endGroup();
    }
  }

  // store additional custom config lists (e.g. used for curve anchors)
  QStringList hStoreIds = FConfig->storeIds();
  if (!hStoreIds.isEmpty()) {
    for (QString hId: hStoreIds) {
      ptStorable* hStore = FConfig->getStore(hId);
      if (!hStore) continue;
      TConfigStore hList = hStore->storeConfig(hId+"/");

      for (auto hItem = hList.constBegin(); hItem != hList.constEnd(); ++hItem) {
        APreset->setValue(hItem.key(), makeStorageFriendly(hItem.value()));
      }
    }
  }

  if (AIncludeFlags) {
    APreset->setValue(CIsBlocked, makeStorageFriendly(FIsBlocked));
  }

  doExportCustomConfig(APreset, AIncludeFlags);

  APreset->endGroup();
}

//==============================================================================

/*! Import preset from \c APreset. */
void ptFilterBase::importPreset(QSettings *APreset, const bool ARequestPipeRun /*=false*/) {
  FPreventPipeRun = !ARequestPipeRun;
  APreset->beginGroup(this->FFilterName + "/" + this->uniqueName());

  // *** default config store *** //
  TConfigStore hSettings;
  for (ptCfgItem hCfgItem: FCfgItems) {
    auto hValue = APreset->value(hCfgItem.Id); // returns invalid QVariant if not present in preset
    if (hValue.isValid()) {
      hSettings.insert(hCfgItem.Id, hCfgItem.validate(hValue));
    }
  }

  // *** custom stores *** //
  // Read list of store names, create and fill each one
  for (QString hId: APreset->value(CCustomStores).toStringList()) {
    TConfigStore *hList = FConfig->getSimpleStore(hId);
    if (!hList)
      hList = FConfig->newSimpleStore(hId);

    APreset->beginGroup(hId);
    for (QString hKey: APreset->allKeys()) {
      hList->insert(hKey, APreset->value(hKey));  // read one store item from preset
    }
    APreset->endGroup();
  }

  // *** complex stores ***
  for (QString hStoreId: FConfig->storeIds()) {
    APreset->beginGroup(hStoreId);
    TConfigStore hConfig;
    for (QString hKey: APreset->allKeys()) {
      hConfig.insert(hStoreId+"/"+hKey, APreset->value(hKey));
    }
    APreset->endGroup();
    FConfig->getStore(hStoreId)->loadConfig(hConfig, hStoreId+"/");
  }

  // flags and derived’s custom stuff
  if (flags() & FilterIsBlockable) {
    FIsBlocked = APreset->value(CIsBlocked, FIsBlocked).toBool() ||
                 Settings->GetStringList("HiddenTools").contains(FUniqueName);
  }
  doImportCustomConfig(APreset);

  APreset->endGroup();

  FConfig->update(hSettings);
  updateGui(ARequestPipeRun);
  FPreventPipeRun = false;
}

//==============================================================================

/*! Resets the filter to default values. */
void ptFilterBase::reset(const bool ARequestPipeRun /*=false*/) {
  // Call the children reset method
  doReset();

  FConfig->clearSimpleStores();
  createConfig();
  updateGui(ARequestPipeRun);
}

//==============================================================================

/*! Executes the filter on \c AImage. */
void ptFilterBase::runFilter(ptImage *AImage) const {
  doRunFilter(AImage);
}

//------------------------------------------------------------------------------
/*! A filter has an active config when it is configured to do processing on the image.
    That does not imply that the filter is really performing any processing. E.g. it might be
    blocked or hidden.
    \return \c True when the filter has an active config, \c false otherwise.
 */
bool ptFilterBase::hasActiveCfg() const {
  return FHasActiveCfg;
}

//------------------------------------------------------------------------------
/*! An active filter really performs operations on the image when the pipe runs.
    I.e. it has an active config, is not blocked and not hidden. If you need to know if a
    filter will process the image when the pipe runs use this function.
    \return \c True when the filter is ready for action, \c false otherwise.
    \see ptFilterDM’s FPipeActiveFilters list
 */
bool ptFilterBase::isActive() const {
  return FIsActive;
}

//------------------------------------------------------------------------------
/*! A blocked filter is prevented from doing any processing by the user.
    \return \c True when the filter is blocked, \c false otherwise.
 */
bool ptFilterBase::isBlocked() const {
  return FIsBlocked;
}

//------------------------------------------------------------------------------
/*! Blocks or unblocks the filter. Blocking is a user action triggered by the *Block*
    context menu entry. Never use \c setBlocked() if you need to prevent a filter from doing
    any processing programmatically.
    \return \c True when the function succeeded (only blockable filters can be blocked),
      \c false otherwise.
    \see flags()
 */
bool ptFilterBase::setBlocked(const bool AIsBlocked) {
  if (!(flags() & FilterIsBlockable))
    return false;

  if (AIsBlocked != FIsBlocked) {
    FIsBlocked          = AIsBlocked;
    bool hStatusChanged = this->checkActiveChanged(true);

    GFilterDM->UpdateActivesList(this);

    // GUI update (if we have a GUI) must happen before the possible pipe run.
    if (FGuiContainer)
      FGuiContainer->updateGui();

    if (hStatusChanged || FIsActive)
      requestPipeRun(true);
  }

  return true;
}

//------------------------------------------------------------------------------
/*! A hidden filter is not visible in the GUI and does not do any processing.
    \return \c True when the filter is hidden, \c false otherwise
 */
bool ptFilterBase::isHidden() const {
  return Settings->GetStringList("HiddenTools").contains(FUniqueName);
}

//------------------------------------------------------------------------------
/*! Hides or unhides the filter. Hiding is a user action triggered by the *Hide*
    context menu entry. Never use \c setHidden() if you need to prevent a filter from doing
    any processing programmatically.
    \return \c True when the function succeeded (only hideable filters can be hidden),
      \c false otherwise.
    \see flags(), the ptSettings "HiddenTools" list
*/
bool ptFilterBase::setHidden(const bool AIsHidden) {
  if (!(flags() & FilterIsHideable))
    return false;

  // update global list of hidden tools
  auto hHiddens  = Settings->GetStringList("HiddenTools");
  if (hHiddens.contains(FUniqueName) != AIsHidden) {
    if (AIsHidden) {
      if (!hHiddens.contains(FUniqueName))
        hHiddens.append(FUniqueName);
    } else {
      hHiddens.removeAll(FUniqueName);
    }
    Settings->SetValue("HiddenTools", hHiddens);

    bool hStatusChanged = this->checkActiveChanged(true);
    GFilterDM->UpdateActivesList(this);

    if (hStatusChanged || FIsActive)
      requestPipeRun(true);
  }

  return true;
}

//------------------------------------------------------------------------------
QString ptFilterBase::caption() const {
  return FCaption + FCaptionPostfix;
}

//------------------------------------------------------------------------------
bool ptFilterBase::hasHelp() const {
  return !FHelpUri.isEmpty();
}

//------------------------------------------------------------------------------
QString ptFilterBase::helpUri() const {
  return FHelpUri;
}

//==============================================================================

bool ptFilterBase::isFavourite() const {
  return Settings->GetStringList("FavouriteTools").contains(FUniqueName);
}

//==============================================================================

/*!
    \return \c True when the function succeeded (only favouritable filters can be set favourite),
      \c false otherwise.
    \see flags(), the ptSettings "FavouriteTools" list
 */
bool ptFilterBase::setFavourite(bool AIsFavourite) {
  if (!(flags() & FilterIsFavouriteable))
    return false;

  auto hFavs = Settings->GetStringList("FavouriteTools");
  if (!AIsFavourite) {
    hFavs.removeAll(FUniqueName);
  } else {
    if (!hFavs.contains(FUniqueName))
      hFavs.append(FUniqueName);
  }

  Settings->SetValue("FavouriteTools", hFavs);
  return true;
}

//------------------------------------------------------------------------------
bool ptFilterBase::isSlow() const {
  return FIsSlow;
}

//------------------------------------------------------------------------------
int ptFilterBase::idxInParentTab() const {
  return FIdxInParentTab;
}

//------------------------------------------------------------------------------
int ptFilterBase::parentTabIdx() const {
  return FParentTabIdx;
}

//==============================================================================

QString ptFilterBase::uniqueName() const {
  GInfo->Assert(!FUniqueName.isEmpty(),
                QString("No unique name set for filter \"%1\".") .arg(FFilterName), AT);

  return FUniqueName;
}

//==============================================================================

ptToolBox *ptFilterBase::gui() {
  if (!FGuiContainer)
    createGui();

  return FGuiContainer;
}

//==============================================================================

// Read .h regarding this function. ... I mean it!
QWidget *ptFilterBase::guiWidget() {
  return gui();
}

// Read .h regarding this function. ... I mean it!
bool ptFilterBase::canHide() const {
  return flags() & FilterIsHideable;
}

//==============================================================================

void ptFilterBase::setPos(int ATab, int AIdx) {
  FParentTabIdx   = ATab;
  FIdxInParentTab = AIdx;
  GFilterDM->UpdatePositions(this);
}

//==============================================================================

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
void ptFilterBase::init(const QString &AUniqueName, const QString &AGuiNamePostfix) {
  GInfo->Assert(!AUniqueName.isEmpty(),
                QString("Unique name for filter \"%1\" cannot be empty.").arg(FFilterName), AT);

  if (FGuiContainer)
    FGuiContainer->setObjectName(AUniqueName);

  this->setObjectName(AUniqueName);
  FUniqueName     = AUniqueName;
  FCaptionPostfix = AGuiNamePostfix;
}

//==============================================================================

/*!
    Derived classes must reimplement this function to return the appropriate flags.
    The default implementation enables all flags.
 */
ptFilterBase::TFilterFlags ptFilterBase::flags() const {
  return FilterIsBlockable | FilterHasDefault | FilterIsSaveable | FilterIsFavouriteable |
         FilterIsHideable;
}

//==============================================================================

ptFilterBase::ptFilterBase()
: /*QObject(),*/ptTempFilterBase(),
  FHasActiveCfg(false),
  FIsSlow(false),
  FConfig(make_unique<ptFilterConfig>()),
  FGuiContainer(nullptr),
  FIsActive(false),
  FParentTabIdx(-1),
  FIdxInParentTab(-1),
  FIsBlocked(false),
  FPreventPipeRun(false)
{}

//==============================================================================

/*! Init: Every derived class has to call it in its constructor.*/
void ptFilterBase::internalInit() {
  doDefineControls();

  createConfig();
  checkActiveChanged();
}

//------------------------------------------------------------------------------

/*! Returns a pointer to the `ptWidget` object identified by `AId`. If such a widget cannot be
    found, raises an exception via ptInfo. */
ptWidget *ptFilterBase::findPtWidget(const QString &AId, QWidget *AWidget) {
  ptWidget *hWidget = AWidget->findChild<ptWidget*>(AId);
  if (!hWidget)
    GInfo->Raise(QString("Widget \"%1\" not found.").arg(AId),
                 QString(FFilterName+"/"+FUniqueName+": "+AT).toAscii().data());
  return hWidget;
}

//------------------------------------------------------------------------------

/*! Check and possibly convert a `QVariant` to avoid problems in `QSettings` storage. */
QVariant ptFilterBase::makeStorageFriendly(const QVariant &AVariant) const {
  auto hVariant = AVariant;

  // Convert bool to int because that is more robust in a preset file
  if (hVariant.type() == QVariant::Bool)
    hVariant.convert(QVariant::Int);

  return hVariant;
}

//------------------------------------------------------------------------------

/*! Connects the commonDispach() slot to all applicable controls. The default implementation
    connects all controls from the common controls lists that have their \c UseCommonDispatch
    flag set to \c true.
*/
void ptFilterBase::connectCommonDispatch() {
  GInfo->Assert(!FGuiContainer, "The filter's ("+FFilterName+") GUI must be created first.", AT);

  for (auto hCfgItem: FCfgItems)
    performCommonConnect(hCfgItem, FGuiContainer->findChild<QWidget*>(hCfgItem.Id));
}

//==============================================================================

int ptFilterBase::cfgIdx(const QString &AId) const {
  for (int i = 0; i < FCfgItems.size(); ++i) {
    if (FCfgItems.at(i).Id == AId) {
      return i;
    }
  }
  return -1;
}

//==============================================================================

/*! Initializes a Qt Designer created GUI with the values from the config items list.
    Only needed when you create the GUI with Designer. Call once from \c doCreateGui().
    Do not use for reset-to-default-values, use \c reset() instead.
    Also connects the common dispatcher for controls from the config items list that have
    their UseCommonConnect flag set to \c true. I.e. you should *not* call both
    \c connectCommonDispatch() and \c initDesignerGui().
    \param AGuiBody
      A valid pointer to the widget containing the uninitialized GUI controls.
 */
void ptFilterBase::initDesignerGui(QWidget *AGuiBody) {
  for (ptCfgItem hCfgItem: FCfgItems) {
    ptWidget *hWidget = findPtWidget(hCfgItem.Id, AGuiBody);
    // init the widget with default values and connect signals/slots
    hWidget->init(hCfgItem);
    this->performCommonConnect(hCfgItem, hWidget);
  }
}

//==============================================================================

/*! Creates a Photivo custom widget and returns a pointer to the widget.
    \param ACfgItem
      The config data for the new widget. ACfgItem.Type determines the concrete type of the
      created object.
    \param AParent
      The new widget’s parent is set to \c AParent.
 */
ptWidget* ptFilterBase::createWidgetByType(const ptCfgItem &ACfgItem, QWidget *AParent) {
  switch (ACfgItem.Type) {
  // NOTE: Buttons will probably be removed
//      case ptCfgItem::Button: {
//        hGuiWidget = new QToolButton(FGuiContainer);
//        break;
//      }
  case ptCfgItem::Check:      return new ptCheck(ACfgItem, AParent);
  case ptCfgItem::Combo:      return new ptChoice(ACfgItem, AParent);
  case ptCfgItem::SpinEdit:   // fall through
  case ptCfgItem::Slider:     // fall through
  case ptCfgItem::HueSlider:  return new ptInput(ACfgItem, AParent);
  case ptCfgItem::CurveWin:   return new ptCurveWindow(ACfgItem, AParent);

  default:
    GInfo->Raise(QString("Unhandled ptCfgItem::TType value: %1.").arg(ACfgItem.Type), AT);
    break;
  }

  return nullptr;
}

//==============================================================================

/*! Common dispatcher interfacing from UI control to config entry. Implemented as
    a slot that connects to the control’s valueChanged() signal.
    \param Id
      The UI control’s ID string.
    \param ANewValue
      The new value as a \c QVariant.
 */
void ptFilterBase::commonDispatch(const QString AId, const QVariant ANewValue) {
  if (!ANewValue.isValid()) return;

  // find the right config item
  int hIdx = cfgIdx(AId);
  if (hIdx == -1) return;

  if ((FCfgItems.at(hIdx).Type < ptCfgItem::CFirstCustomType) &&
      (FConfig->getValue(AId) != ANewValue))
  { // item from default store
    FConfig->setValue(AId, ANewValue);
    requestPipeRun();

  } else if ((FCfgItems.at(hIdx).Type >= ptCfgItem::CFirstCustomType) &&
             (ANewValue.type() == QVariant::Map))
  { // item that has a custom store
    (*FConfig->getSimpleStore(FCfgItems.at(hIdx).Id)) = ANewValue.toMap();
    requestPipeRun();
  }
}

//==============================================================================

// Creates the \c FConfig object and initialises it with the default key/value pairs
// from the common controls lists.
void ptFilterBase::createConfig() {
  TConfigStore hDefaultStore;

  // NOTE: Not sure yet if buttons really need to be stored in settings.
  // If not add exclusion code to for loop.

  for (ptCfgItem hCfgItem: FCfgItems) {
    if (hCfgItem.Type < ptCfgItem::CFirstCustomType)
      hDefaultStore.insert(hCfgItem.Id, hCfgItem.Default);
     else
      FConfig->newSimpleStore(hCfgItem.Id, hCfgItem.Default.toMap());
  }

  doAddCustomConfig(hDefaultStore);
  FConfig->init(hDefaultStore);
}

//==============================================================================

/*! Helper method that performs the commonDispatch() connection for \c AObject
    if the object exists and \c ACfgItem.UseCommonDispatch is \c true. */
void ptFilterBase::performCommonConnect(const ptCfgItem &ACfgItem, QObject *AObject) {
  if (ACfgItem.UseCommonDispatch && AObject) {
    connect(AObject, SIGNAL(valueChanged(QString,QVariant)),
            this,    SLOT(commonDispatch(QString,QVariant)));
  }
}

//==============================================================================

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
bool ptFilterBase::checkActiveChanged(const bool ANoSignal /*= false*/) {
  bool hOldStatus     = FIsActive;
  FHasActiveCfg       = doCheckHasActiveCfg();
  FIsActive           = FHasActiveCfg && !FIsBlocked && !isHidden();

  if (FIsActive != hOldStatus) {
    GFilterDM->UpdateActivesList(this);
    if (!ANoSignal)
      emit activityChanged();
    return true;

  } else {
    return false;
  }
}

//==============================================================================

void Update(const QString GuiName);
/*! Must be called whenever the filter needs to run. */
void ptFilterBase::requestPipeRun(const bool AUnconditional) {
  if (FPreventPipeRun) return;
  if (AUnconditional || this->checkActiveChanged() || FIsActive)
    Update(FUniqueName);
}

//==============================================================================

/*! Updates all widgets with values from config. Also takes care of activity status and
    pipe run request. */
void ptFilterBase::updateGui(const bool ARequestPipeRun /*= true*/) {
  if (FGuiContainer) {
    // call the children.
    doUpdateGui();

    for(ptCfgItem hCfgItem: FCfgItems) {
      ptWidget* hWidget = FGuiContainer->findChild<ptWidget*>(hCfgItem.Id);

      if (!hWidget) {
        GInfo->Warning(QString("%1: Widget \"%2\" not found in GUI.")
                          .arg(uniqueName(), hCfgItem.Id), AT);

      } else if (hCfgItem.Type >= ptCfgItem::CFirstCustomType) {
        hWidget->setValue(*FConfig->getSimpleStore(hCfgItem.Id));

      } else {
        hWidget->setValue(FConfig->getValue(hCfgItem.Id));
      }
    }
  }
  else {
    // loading curves in job mode
    for(ptCfgItem hCfgItem: FCfgItems) {
      if (hCfgItem.Type == ptCfgItem::CurveWin)
        hCfgItem.Curve->setFromFilterConfig(*FConfig->getSimpleStore(hCfgItem.Id));
    }
  }

  if (ARequestPipeRun) {
    requestPipeRun();
  } else {
    checkActiveChanged();
  }
}

//==============================================================================

/*! Creates the GUI. The default implementation creates the \c FUIContainer widget,
    puts all controls from the common controls lists in a vertical layout and
    connects the common dispatcher where needed. */
void ptFilterBase::createGui() {
  GInfo->Assert(!FGuiContainer, "GUI already created.", AT);
  GInfo->Assert((bool)FConfig, "FSettings object must be instantiated first.", AT);

  auto hGuiBody = doCreateGui();

  // We create the gui, if the children did not do it.
  if (!hGuiBody) {
    hGuiBody     = new QWidget;
    auto hLayout = new QVBoxLayout(hGuiBody);

    for (ptCfgItem hCfgItem: FCfgItems) {
      ptWidget *hGuiWidget = createWidgetByType(hCfgItem, hGuiBody);
      if (hGuiWidget) {
        hLayout->addWidget(hGuiWidget);
        performCommonConnect(hCfgItem, hGuiWidget);
      }
    }
  }

  FGuiContainer = new ptToolBox(hGuiBody, this);
  if (Settings->GetStringList("HiddenTools").contains(FUniqueName)) {
    FGuiContainer->hide();
  }
}



//==============================================================================


/*!
  Makes a new filter type and its construction function known to the filter factory.
  Call this once for each filter type to make it available to Photivo.
 */
RegisterHelper::RegisterHelper(const ptFilterFactoryMethod AMethod, const QString AName) {
  ptFilterFactory::GetInstance()->RegisterFilter(AMethod, AName);
}

//==============================================================================
