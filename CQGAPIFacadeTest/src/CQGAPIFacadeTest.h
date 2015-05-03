/// @file CQGAPIFacadeTest.h
/// @brief CQG API Facade usage sample application - main header file.
/// @copyright Licensed under the MIT License.
/// @author Rostislav Ostapenko (rostislav.ostapenko@gmail.com)
/// @date 16-Feb-2015

#pragma once

#ifndef __AFXWIN_H__
   #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"   // main symbols


// CCQGAPIFacadeTestApp:
// See CQGAPIFacadeTest.cpp for the implementation of this class
//

class CCQGAPIFacadeTestApp : public CWinApp
{
public:
   CCQGAPIFacadeTestApp();

// Overrides
public:
   virtual BOOL InitInstance();

// Implementation

   DECLARE_MESSAGE_MAP()
};

extern CCQGAPIFacadeTestApp theApp;
