/// @file CQGAPIFacadeTest.cpp
/// @brief CQG API Facade usage sample application - defines the class behaviors for the application.
/// @copyright Licensed under the MIT License.
/// @author Rostislav Ostapenko (rostislav.ostapenko@gmail.com)
/// @date 16-Feb-2015

#include "stdafx.h"
#include "CQGAPIFacadeTest.h"
#include "CQGAPIFacadeTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCQGAPIFacadeTestApp
BEGIN_MESSAGE_MAP(CCQGAPIFacadeTestApp, CWinApp)
END_MESSAGE_MAP()


// CCQGAPIFacadeTestApp construction
CCQGAPIFacadeTestApp::CCQGAPIFacadeTestApp()
{
}


// The one and only CCQGAPIFacadeTestApp object
CCQGAPIFacadeTestApp theApp;

// COM module required for ATL stuff
CComModule _Module;

// CCQGAPIFacadeTestApp initialization
BOOL CCQGAPIFacadeTestApp::InitInstance()
{
   CWinApp::InitInstance();

   CCQGAPIFacadeTestDlg dlg;
   m_pMainWnd = &dlg;
   dlg.DoModal();

   // Since the dialog has been closed, return FALSE so that we exit the
   // application, rather than start the application's message pump.
   return FALSE;
}
