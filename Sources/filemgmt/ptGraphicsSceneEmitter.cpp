/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2011 Bernd Schoeler <brjohn@brother-john.net>
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

#include "ptGraphicsSceneEmitter.h"

//==============================================================================

ptGraphicsSceneEmitter* ptGraphicsSceneEmitter::m_Instance = NULL;

//==============================================================================

ptGraphicsSceneEmitter* ptGraphicsSceneEmitter::GetInstance() {
  if (m_Instance == NULL) {
    m_Instance = new ptGraphicsSceneEmitter;
  }

  return m_Instance;
}

//==============================================================================

void ptGraphicsSceneEmitter::DestroyInstance() {
  delete m_Instance;
  m_Instance = NULL;
}

//==============================================================================

ptGraphicsSceneEmitter::ptGraphicsSceneEmitter(): QObject(NULL) {}
ptGraphicsSceneEmitter::~ptGraphicsSceneEmitter() {}

//==============================================================================

bool ptGraphicsSceneEmitter::ConnectItemsChanged(const QObject* receiver, const char *method) {
  ptGraphicsSceneEmitter* Instance = GetInstance();
  return Instance->connect(Instance, SIGNAL(itemsChanged()), receiver, method);
}

//==============================================================================

void ptGraphicsSceneEmitter::EmitItemsChanged() {
  emit GetInstance()->itemsChanged();
}

//==============================================================================

bool ptGraphicsSceneEmitter::ConnectThumbnailAction(const QObject* receiver, const char *method) {
  ptGraphicsSceneEmitter* Instance = GetInstance();
  return Instance->connect(Instance, SIGNAL(thumbnailAction(ptThumbnailAction,QString)),
                           receiver, method);
}

//==============================================================================

void ptGraphicsSceneEmitter::EmitThumbnailAction(const ptThumbnailAction action,
                                                 const QString location)
{
  ptGraphicsSceneEmitter* Instance = GetInstance();
  emit Instance->thumbnailAction(action, location);
}

//==============================================================================

