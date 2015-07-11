/// @file CQGAPIFacadeTestDlg.h
/// @brief CQG API Facade usage sample application - main dialog class header.
/// @copyright Licensed under the MIT License.
/// @author Rostislav Ostapenko (rostislav.ostapenko@gmail.com)
/// @date 16-Feb-2015

#pragma once

#include "CQGAPIFacade.h"


// CCQGAPIFacadeTestDlg dialog
class CCQGAPIFacadeTestDlg : public CDialog, public cqg::IAPIEvents
{
// Construction
public:
   CCQGAPIFacadeTestDlg(CWnd* pParent = NULL);

// Dialog Data
   enum { IDD = IDD_CQGAPIFACADETEST_DIALOG };

// Implementation
private:
   CWnd* m_console;          ///< Console edit box.
   cqg::IAPIFacadePtr m_api; ///< API Facade.
   CString m_stpOrderGuid;   ///< Stop order guid.
   cqg::ID m_gwAccID;        ///< Main GW account ID used.

   /// @brief Writes string to console.
   /// @param msg [in] mesage to console.
   void write(const CString& msg);

   /// @brief Writes string to console and adds new line.
   /// @param msg [in] mesage to console.
   void writeLn(const CString& msg);

   /// @brief Prints working orders to console.
   void printWorkingOrders();

   /// @brief Dialog initialization.
   virtual BOOL OnInitDialog();

   /// @name IAPIEvents implementation.
   /// @{

   virtual void OnError(const CString& error);
   virtual void OnMarketDataConnection(const bool connected);
   virtual void OnTradingConnection(const bool connected);
   virtual void OnSymbolSubscribed(const CString& requestedSymbol, const cqg::SymbolInfo& symbol);
   virtual void OnSymbolError(const CString& symbol);
   virtual void OnSymbolQuote(const cqg::SymbolInfo& symbol);
   virtual void OnAccountsReloaded();
   virtual void OnPositionsReloaded();
   virtual void OnAccountChanged(const cqg::AccountInfo& account);
   virtual void OnPositionChanged(const cqg::AccountInfo& account, const cqg::PositionInfo& position, const bool newPosition);
   virtual void OnOrderChanged(const cqg::OrderInfo& order);

   /// @}

   // Generated message map functions.
   DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedCancelAll();
};
