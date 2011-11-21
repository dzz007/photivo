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
*******************************************************************************
Photivo'ized singleton version of EcWin7 1.0.2
  EcWin7 - Support library for integrating Windows 7 taskbar features
  into any Qt application
  Copyright (C) 2010 Emanuele Colombo
Find original sources in ReferenceMaterial/ecwin directory or under
http://dukto.googlecode.com/
*******************************************************************************/

#include <cassert>

#include "ptEcWin7.h"

//==============================================================================

ptEcWin7* ptEcWin7::m_Instance = NULL;

//==============================================================================

void ptEcWin7::CreateInstance(QWidget* window) {
  assert(m_Instance == NULL);
  m_Instance = new ptEcWin7;
  m_Instance->init(window->winId());
}

//==============================================================================

ptEcWin7* ptEcWin7::GetInstance() {
  assert(m_Instance != NULL);
  return m_Instance;
}

//==============================================================================

void ptEcWin7::DestroyInstance() {
  delete m_Instance;
  m_Instance = NULL;
}

//==============================================================================

// Windows only GUID definitions
#if defined(Q_OS_WIN)
DEFINE_GUID(CLSID_TaskbarList,0x56fdf344,0xfd6d,0x11d0,0x95,0x8a,0x0,0x60,0x97,0xc9,0xa0,0x90);
DEFINE_GUID(IID_ITaskbarList3,0xea1afb91,0x9e28,0x4b86,0x90,0xE9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf);
#endif

// Constructor: variabiles initialization
ptEcWin7::ptEcWin7()
{
#ifdef Q_OS_WIN
    mTaskbar = NULL;
    mOverlayIcon = NULL;
#endif
}

ptEcWin7::~ptEcWin7() {}

// Init taskbar communication
void ptEcWin7::init(WId wid)
{
    mWindowId = wid;
#ifdef Q_OS_WIN
    mTaskbarMessageId = RegisterWindowMessage(L"TaskbarButtonCreated");
#endif
}

// Windows event handler callback function
// (handles taskbar communication initial message)
#ifdef Q_OS_WIN
bool ptEcWin7::winEvent(MSG * message, long * result)
{
    if (message->message == mTaskbarMessageId)
    {
        HRESULT hr = CoCreateInstance(CLSID_TaskbarList,
                                      0,
                                      CLSCTX_INPROC_SERVER,
                                      IID_ITaskbarList3,
                                      reinterpret_cast<void**> (&(mTaskbar)));
        *result = hr;
        return true;
    }
    return false;
}
#endif

// Set progress bar current value
void ptEcWin7::setProgressValue(int value, int max)
{
#ifdef Q_OS_WIN
    if (!mTaskbar) return;
    mTaskbar->SetProgressValue(mWindowId, value, max);
#endif
}

// Set progress bar current state (active, error, pause, ecc...)
void ptEcWin7::setProgressState(ToolBarProgressState state)
{
#ifdef Q_OS_WIN
    if (!mTaskbar) return;
    mTaskbar->SetProgressState(mWindowId, (TBPFLAG)state);
#endif
}

// Set new overlay icon and corresponding description (for accessibility)
// (call with iconName == "" and description == "" to remove any previous overlay icon)
void ptEcWin7::setOverlayIcon(QString iconName, QString description)
{
#ifdef Q_OS_WIN
    if (!mTaskbar) return;
    HICON oldIcon = NULL;
    if (mOverlayIcon != NULL) oldIcon = mOverlayIcon;
    if (iconName == "")
    {
        mTaskbar->SetOverlayIcon(mWindowId, NULL, NULL);
        mOverlayIcon = NULL;
    }
    else
    {
        mOverlayIcon = (HICON) LoadImage(GetModuleHandle(NULL),
                                 iconName.toStdWString().c_str(),
                                 IMAGE_ICON,
                                 0,
                                 0,
                                 0);
        mTaskbar->SetOverlayIcon(mWindowId, mOverlayIcon, description.toStdWString().c_str());
    }
    if ((oldIcon != NULL) && (oldIcon != mOverlayIcon))
    {
        DestroyIcon(oldIcon);
    }
#endif
}
