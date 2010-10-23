////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender
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

#include "ptLensfun.h"
#include "ptDefines.h"
#include "ptError.h"
#include "ptSettings.h"

#include <cstdlib>
#include <glib/gstdio.h>

// Compare for sorting on make/model.
int CameraCompare(const void* a, const void* b) {
  if ((((const ptLensfunCamera*)a)->Make) ==
      (((const ptLensfunCamera*)b)->Make)) {
    return (((const ptLensfunCamera*)a)->Model) >
           (((const ptLensfunCamera*)b)->Model);
  }
  return (((const ptLensfunCamera*)a)->Make) >
         (((const ptLensfunCamera*)b)->Make);
}

// Compare for sorting on make/model.
int LensCompare(const void* a, const void* b) {
  if ((((const ptLensfunLens*)a)->Make) ==
      (((const ptLensfunLens*)b)->Make)) {
    return (((const ptLensfunLens*)a)->Model) >
           (((const ptLensfunLens*)b)->Model);
  }
  return (((const ptLensfunLens*)a)->Make) >
 (((const ptLensfunLens*)b)->Make);
}

// XXX JDLA Adapted for photivo
// MIKE: moved here and adopted from lensfun/database.cpp
lfError ptLensfun::LoadDir(const char* Directory)
{
    // Adapted JDLA. Qt system will be used later on here.

    GDir *dir = g_dir_open (Directory, 0, NULL);

    if (!dir) {
    ptLogError(ptError_FileOpen,"LensfunDatabase could not be opened.");
    exit(EXIT_FAILURE);
    }

    GPatternSpec *ps = g_pattern_spec_new ("*.xml");
    if (ps) {
      const gchar *fn;
      while ((fn = g_dir_read_name (dir))) {
        size_t sl = strlen (fn);
        if (g_pattern_match (ps, sl, fn, NULL)) {
          gchar *ffn = g_build_filename ("LensfunDatabase", fn, NULL);
          /* Ignore errors */
          m_Database->Load(ffn);
          g_free (ffn);
        }
      }
      g_pattern_spec_free (ps);
    }
    g_dir_close (dir);
    return LF_NO_ERROR;
}

ptLensfun::ptLensfun() {

  // Load also the LensfunDatabase.

  m_Database = lf_db_new();
  LoadDir(Settings->GetString("LensfunDatabaseDirectory").toAscii().data());

  // All cameras
  const lfCamera* const* CameraList = m_Database->GetCameras();

  // Count cameras.
  m_NrCameras = 0;
  for (;CameraList[m_NrCameras]; m_NrCameras++) {};

  // No alloc variant as the QString constructor is involved.
  m_Cameras = new ptLensfunCamera[m_NrCameras];

  // Populate
  for (uint16_t i=0;i<m_NrCameras; i++) {
    m_Cameras[i].Make    = QString(CameraList[i]->Maker).toUpper();
    m_Cameras[i].Model   = QString(CameraList[i]->Model).toUpper();
    m_Cameras[i].Variant = QString(CameraList[i]->Variant).toUpper();
    m_Cameras[i].Mount   = QString(CameraList[i]->Mount).toUpper();
    m_Cameras[i].Camera  = CameraList[i];
  }
  // Sort
  qsort(m_Cameras,m_NrCameras,sizeof(ptLensfunCamera),CameraCompare);

  // All lenses
  const lfLens* const* LensList = m_Database->GetLenses();

  // Count lenses, but each mount is a separate lens.
  m_NrLenses = 0;
  for (uint16_t i=0; LensList[i]; i++) {
    for (short j=0; LensList[i]->Mounts[j]; j++) {
      m_NrLenses++;
    }
  }

  // No alloc variant as the QString constructor is involved.
  m_Lenses = new ptLensfunLens[m_NrLenses];
  for (uint16_t i=0,j=0; LensList[i]; i++) {
    for (short k=0; LensList[i]->Mounts[k]; k++,j++) {
      m_Lenses[j].Make  = QString(LensList[i]->Maker).toUpper();
      m_Lenses[j].Model = QString(LensList[i]->Model).toUpper();
      m_Lenses[j].Mount = QString(LensList[i]->Mounts[k]).toUpper();
      m_Lenses[j].Lens  = LensList[i];
    }
  }
  // Sort
  qsort(m_Lenses,m_NrLenses,sizeof(ptLensfunLens),LensCompare);

}
