// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#define _CRT_SECURE_NO_DEPRECATE 1
#define _USE_32BIT_TIME_T

#define WIN32_LEAN_AND_MEAN                 // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

// Exclude rarely-used stuff from Windows headers
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

// Turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include "targetver.h"
#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <atlcomtime.h>     // COleDateTime
