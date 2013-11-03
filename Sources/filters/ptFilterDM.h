/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Michael Munzert <mail@mm-log.com>
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
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

#ifndef PTFILTERDM_H
#define PTFILTERDM_H

//==============================================================================

#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>

#include <memory>

// forward
class QSettings;
class ptFilterBase;
class ptFilterFactory;
class ptFilterIterator;
class ptConstFilterIterator;

//==============================================================================

typedef QList<ptFilterBase*> TCacheGroup;
typedef QList<TCacheGroup>   TCacheGroupList;

//==============================================================================

/*! \class ptFilterDM
  Holds all created filters and provides access to them.
  */
class ptFilterDM {
public:
  /*! Constructor of the singleton instance*/
  static void CreateInstance();
  

  /*! \group Filter management */
  ///@{
  /*! Add a new filter of type \c AFilterId. For the other parameters \see ptFilterBase::init(). */
  ptFilterBase        *NewFilter(const QString &AFilterId,
                                 const QString &AUniqueName,
                                 const QString &AGuiNamePostfix = "");

  /*! Returns a pointer to the filter with the ID \c AUniqueName.
      \param AThrowOnNotFound
        When set to \c true raises an exception via ptInfo when the requested filter does not exist.
        This is the default. When set to \c false returns a \c nullptr instead of erroring out.
   */
  ptFilterBase        *GetFilterFromName(const QString AUniqueName,
                                         const bool AThrowOnNotFound = true) const;

  /*! Checks if filters are active in a given cache group.
      \param AGroupIdx
        The index number of the cache group.
      \return
        \c True when at least one filter is active in the cache group, \c false otherwise
        or when the index is invalid.
   */
  bool                isActiveCacheGroup(const int AGroupIdx);

  /*! Adds or removes a filter from the list of active filters. The performed action depends
      on AFilter->isActive(). */
  void                UpdateActivesList(ptFilterBase *AFilter);

  /*! Updates the tab/idx of \c AFilter in the ordered filter lists.*/
  void                UpdatePositions(ptFilterBase *AFilter);
  ///@}


  /*! \group Preset management */
  ///@{
  /*! Translates from old-style to new-style preset format.
      \param APreset
        A pointer to the \c QSettings object that is to be translated.
      \param AKeepUnknown
        When set to \c true original keys that don’t have a translated equivalent are kept.
        When set to \c false they are removed.
   */
  void          ToNewPreset(QSettings *APreset);

  /*! Same as \see ToNewPreset() but translating from new-style to old-style preset instead. */
  void          ToOldPreset(QSettings *APreset);

  /*! Read preset from a file. */
  bool          ReadPresetFile(const QString &AFileName, short &ANextPhase);

  /*! Writes preset to a file.
      \param AFileName
        Full path to the output file. If \c AFileName is omitted or empty
        the function presents a save file dialog to the user.
      \param AAppend
        If set to \c true the new data is appended to an existing preset file.
        If omitted or set to \c false an existing preset file will be overwritten.
      \param AIncludeFlags
        Specifies whether to include flags in the preset file.
        \see ptFilterBase::exportPreset()
      \param AFilter
        A pointer to the filter object that is to be saved. If \c AFilter is omitted
        or \c nullptr a full preset file for all filters is written.
      \return
        Returns \c true when writing the file succeeded, \c false otherwise.
   */
  bool          WritePresetFile(const QString       &AFileName     = "",
                                const bool           AAppend       = false,
                                const bool           AIncludeFlags = true,
                                const ptFilterBase  *AFilter       = nullptr);

  /*! Writes a job file for batch processing. */
  bool          WriteJobFile();

  /*! Saves a settings file and adds it to the batch list. */
  bool          SendToBatch(const QString &AFileName = "");
  ///@}

  /*! \group Iterators
      These functions provide STL-style iterator based access to the filter lists.
      The functions with \c active in their name return iterators to the ordered list
      of active filters. The others return iterators to the ordered list of all filters.
   */
  ///@{
  ptFilterIterator        activeBegin();
  ptFilterIterator        activeEnd();
  ptFilterIterator        begin();
  ptFilterIterator        end();
  ///@}

//-------------------------------------

private:
  ptFilterDM();
  ~ptFilterDM();

  /*! From the ordered lists returns the index of the first cache group containing a filter. Until
      all filters are ported we may have leading empty cache groups, i.e. AList.at(0).at(0) is
      not necessarily a valid begin position.
   */
  static int FindBeginTab(const TCacheGroupList &AList);

  /*! Same as \c FindBeginTab() but returning the index of the *last* non-empty cache group. */
  static int FindEndTab(const TCacheGroupList &AList);

  enum TListType { FullList, ActiveList };
  /*! Inserts a filter into one of the ordered lists at the appropriate position. */
  void InsertToList(const TListType AListType, ptFilterBase *AFilter);

  /*! \group Translation workers.
      Workers for \c ToNewPreset() and \c ToOldPreset(). */
  ///@{
  void TranslatePreset        (QSettings *APreset, const bool AOldToNew);
  void TranslateCurvesToNew   (QSettings *APreset, QStringList *AKeys);
  void TranslateCurvesToOld   (QSettings *APreset, QStringList *AKeys);
  void TranslateNormalToNew   (QSettings *APreset, QStringList *AKeys);
  void TranslateNormalToOld   (QSettings *APreset, QStringList *AKeys);
  void TranslateSpecialToNew  (QSettings *APreset, QStringList *AKeys);
  void TranslateSpecialToOld  (QSettings */*APreset*/);
  ///@}

  bool PerformWritePreset(const QString       &AFileName,
                          const bool           AAppend,
                          const bool           AIncludeFlags,
                          const bool           AIsJobFile,
                          const ptFilterBase  *AFilter);

  /*! Fill the data structure that maps between new-style and old-style config. */
  void FillNameMap();


  
  /*! Singleton instance. */
  static ptFilterDM            *FInstance;

  /*! Filter factory for fast access.*/
  ptFilterFactory              *FFactory;
  
  /*! Constructed filters.*/
  QHash<QString, ptFilterBase*> FFilters;

  /*! A nested list of pointers to the toolboxes on the processing tab, ordered by tab and
      index inside the tab.
   */
  TCacheGroupList               FOrderedFilters;

  /*! A nested list of pointers to all currently active filters. */
  TCacheGroupList               FActiveFilters;

  friend class ptFilterIterator;


private:
  /*! Contains the mapping data between old-style (key) and new-style (value) config names. */
  QHash<QString, QString>       FNameMap;
  /*! Contains the mapping data between old-style (key) and new-style (value) curve names. */
  QHash<QString, QString>       FCurveNameMap;
  QHash<QString, QString>       FCurveMap;

};


//==============================================================================


// Modelled after the QList STL-style iterators. Thanks Qt. :)
//
// The only difference between both iterator types is that operator* and operator-> return
// "const ptFilterBase" for ptConstFilterIterator and "ptFilterBase" for ptFilterIterator.
class ptFilterIterator {
public:
  inline ptFilterIterator(TCacheGroupList *AList, const bool AIsEndMarker = false);
  inline ptFilterIterator(const ptFilterIterator &other):
    FList(other.FList), FNode(other.FNode), FNodeGroup(other.FNodeGroup), FNodeIdx(other.FNodeIdx),
    FBeyondEnd(other.FBeyondEnd) {}

  inline ptFilterBase     *operator* () const { return FNode; }
  inline ptFilterBase     *operator->() const { return FNode; }

  bool operator==(const ptFilterIterator &other) const;
  bool operator!=(const ptFilterIterator &other) const;

  inline ptFilterIterator &operator++()    { next(); return *this; }
  inline ptFilterIterator  operator++(int) { ptFilterIterator n(*this); next(); return n; }
  inline ptFilterIterator &operator--()    { previous(); return *this; }
  inline ptFilterIterator  operator--(int) { ptFilterIterator n(*this); previous(); return n; }

private:
  void next();      // Advances the iterator to the next node.
  void previous();  // Retreats the iterator to the previous node.

  TCacheGroupList  *FList;
  ptFilterBase     *FNode;
  int               FNodeGroup;
  int               FNodeIdx;
  bool              FBeyondEnd;
};


//==============================================================================


/*! Global variable for fast access. Instantiation happens in \c photivoMain(). */
extern ptFilterDM* GFilterDM;

//==============================================================================

#endif // PTFILTERDM_H
