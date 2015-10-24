/// @file CQGAPIFacadeTestDlg.cpp
/// @brief CQG API Facade usage sample application - main dialog implementation.
/// @copyright Licensed under the MIT License.
/// @author Rostislav Ostapenko (rostislav.ostapenko@gmail.com)
/// @date 16-Feb-2015

#include "stdafx.h"
#include "CQGAPIFacadeTest.h"
#include "CQGAPIFacadeTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CCQGAPIFacadeTestDlg dialog
CCQGAPIFacadeTestDlg::CCQGAPIFacadeTestDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CCQGAPIFacadeTestDlg::IDD, pParent)
   , m_console(NULL)
   , m_api(cqg::IAPIFacade::Create())
   , m_gwAccID()
{
}

BEGIN_MESSAGE_MAP(CCQGAPIFacadeTestDlg, CDialog)
   ON_BN_CLICKED(IDC_CANCEL_ALL, &CCQGAPIFacadeTestDlg::OnBnClickedCancelAll)
END_MESSAGE_MAP()

// CCQGAPIFacadeTestDlg message handlers
BOOL CCQGAPIFacadeTestDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   m_console = GetDlgItem(IDC_CONSOLE);
   ASSERT(m_console);

   const cqg::FacadeVersion version = cqg::IAPIFacade::GetVersion();

   CString verStr;
   verStr.Format("CQG API Facade v%d.%d",
      version.m_major,
      version.m_minor);

   writeLn(verStr);
   writeLn("Copyright (c) 2015 by Rostislav Ostapenko (rostislav.ostapenko@gmail.com)");
   writeLn("---------------------------------------------------------------------------------------------");

   // Initialize CQG API
   m_api->Initialize(this);
   if(m_api->IsValid())
   {
      writeLn("CQG API initialized successfully!");
      printWorkingOrders();
   }
   else
   {
      writeLn("CQG API Initialization failed!");
      writeLn(m_api->GetLastError());
   }

   return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCQGAPIFacadeTestDlg::write(const CString& msg)
{
   ASSERT(m_console);

   CString text;
   m_console->GetWindowText(text);
   text += msg;
   m_console->SetWindowText(text);
}

void CCQGAPIFacadeTestDlg::writeLn(const CString& msg)
{
   write(msg + "\r\n");
}

void CCQGAPIFacadeTestDlg::printWorkingOrders()
{
   CString str;
   str.Format("Working orders count (all/internal): %d/%d",
      m_api->GetAllWorkingOrdersCount(m_gwAccID),
      m_api->GetInternalWorkingOrdersCount(m_gwAccID));

   writeLn(str);
}

CString CCQGAPIFacadeTestDlg::requestBars(const CString& symbol, int barsCount)
{
   const cqg::BarsRequest barsReq =
   {
      symbol,
      true,
      COleDateTime(),
      COleDateTime(),
      0,
      -barsCount,
      30, // 30 minutes bars
      cqg::BarsRequest::UseAllSessions 
   };

   const CString reqID = m_api->RequestBars(barsReq);
   if(!reqID.IsEmpty()) m_barReqSymbols[reqID] = symbol;

   return reqID;
}

void CCQGAPIFacadeTestDlg::OnError(const CString& error)
{
   AfxMessageBox("CEL Error: " + error);

   writeLn("CEL Error: " + error);
   PostQuitMessage(0);
}

void CCQGAPIFacadeTestDlg::OnMarketDataConnection(const bool connected)
{
   writeLn(CString("Real time market data connection is ") + (connected ? "UP" : "DOWN"));
   if(!connected)
   {
      AfxMessageBox("CQGIC exited or disconnected, please re-run.");
      PostQuitMessage(0);
   }

   const COleDateTime lineTime = m_api->GetLineTime();
   const CString lineTimeStr = cqg::IsValidDateTime(lineTime) ? lineTime.Format("%Y-%m-%d %H:%M:%S") : "N/A";

   writeLn("Current Line Time: " + lineTimeStr);

   writeLn("Requesting hour EP and CLE bars...");

   const CString barReqID1 = requestBars("EP", 48);
   const CString barReqID2 = requestBars("CLE", 48);

   if(barReqID1.IsEmpty() || barReqID2.IsEmpty())
   {
      writeLn("Unable to request bars: " + m_api->GetLastError());
   }
   else
   {
      writeLn("Bar request ID: " + barReqID1);
      writeLn("Bar request ID: " + barReqID2);
   }
}

void CCQGAPIFacadeTestDlg::OnTradingConnection(const bool connected)
{
   writeLn(CString("Gateway trading server connection is ") + (connected ? "UP" : "DOWN"));

   static bool first = true;
   if(!connected && first)
   {
      first = false;
      m_api->LogonToGateway("", "");
   }
   else if(connected) first = true;
}

void CCQGAPIFacadeTestDlg::OnSymbolSubscribed(const CString& requestedSymbol, const cqg::SymbolInfo& symbol)
{
   writeLn("Symbol " + requestedSymbol + " successfully resolved as " + symbol.fullName);

   const cqg::Quotes& quotes = symbol.lastQuotes;
   for(size_t i = 0; i < quotes.size(); ++i)
   {
      const char* const typeStr[] = { "???", "ask", "bid", "trade", "close", "high", "low" };

      CString str;
      str.Format("Last %s at price %g, volume %d",
         typeStr[quotes[i].type],
         quotes[i].price,
         quotes[i].volume);

      writeLn(str);
   }

   // Place order after symbol resolved.
   // We must be ready for trade, trading connection up and accounts loaded.
   cqg::Accounts accs;
   if(!m_api->GetAccounts(accs))
   {
      writeLn("Unable to get accounts: " + m_api->GetLastError());
      return;
   }

   if(accs.empty())
   {
      writeLn("No accounts available, ensure trading server connection is UP");
      return;
   }

   m_gwAccID = accs.front().gwAccountID;

   // Print out available accounts
   for(size_t i = 0; i < accs.size(); ++i)
   {
      CString str;
      str.Format("Account: FCM[%d, %s], GW[%d, %s], %s, Balance: %g, OTE: %g, P/L: %g",
         accs[i].fcmID,
         accs[i].fcmAccountID.GetString(),
         accs[i].gwAccountID,
         accs[i].gwAccountName.GetString(),
         accs[i].currency.GetString(),
         accs[i].balance,
         accs[i].ote,
         accs[i].profitLoss);

      writeLn(str);

      cqg::Positions pos;
      if(!m_api->GetPositions(accs[i].gwAccountID, pos))
      {
         writeLn("Unable to get positions: " + m_api->GetLastError());
      }

      // Print out account positions
      for(size_t j = 0; j < pos.size(); ++j)
      {
         CString positionStr;
         positionStr.Format("Position %s %s, size %d, avg price %g, OTE: %g, P/L: %g",
            pos[j].longPosition ? "long" : "short",
            pos[j].symbol.GetString(),
            pos[j].quantity,
            pos[j].averagePrice,
            pos[j].ote,
            pos[j].profitLoss);

         writeLn(positionStr);
      }
   }

   const CString guid = m_api->PlaceOrder(cqg::Market, accs.front().gwAccountID, symbol.fullName, true, 1, "My Dirty Order");
   if(guid.IsEmpty())
   {
      writeLn("Unable to place order: " + m_api->GetLastError());
      return;
   }

   writeLn("Placed MKT order on " + symbol.fullName + 
      ", account " + accs.front().gwAccountName + 
      ", order GUID: " + guid);

   const CString lmtGuid = m_api->PlaceOrder(cqg::Limit, accs.front().gwAccountID, symbol.fullName, false, 1, "My Dirty Order", 51.90);
   if(lmtGuid.IsEmpty())
   {
      writeLn("Unable to place order: " + m_api->GetLastError());
      return;
   }

   writeLn("Placed LMT order on " + symbol.fullName + 
      ", account " + accs.front().gwAccountName + 
      ", order GUID: " + lmtGuid);

   const CString stpGuid = m_api->PlaceOrder(cqg::StopLimit, accs.front().gwAccountID,
      symbol.fullName, true, 1, "Stop to Cancel", 60.54, 60.50);
   if(stpGuid.IsEmpty())
   {
      writeLn("Unable to place order: " + m_api->GetLastError());
      return;
   }

   m_stpOrderGuid = stpGuid;

   writeLn("Placed STP order on " + symbol.fullName + 
      ", account " + accs.front().gwAccountName + 
      ", order GUID: " + stpGuid);

   const CString stpGuid2 = m_api->PlaceOrder(cqg::StopLimit, accs.front().gwAccountID,
      symbol.fullName, true, 1, "Order To Cancel 2", 60.54, 60.50);
   if(stpGuid2.IsEmpty())
   {
      writeLn("Unable to place order: " + m_api->GetLastError());
      return;
   }

   writeLn("Placed STP order on " + symbol.fullName + 
      ", account " + accs.front().gwAccountName + 
      ", order GUID: " + stpGuid2);
}

void CCQGAPIFacadeTestDlg::OnSymbolError(const CString& symbol)
{
   writeLn("Unable to resolve: " + symbol);
}

void CCQGAPIFacadeTestDlg::OnSymbolQuote(const cqg::SymbolInfo& symbol)
{
   const cqg::Quotes& quotes = symbol.lastQuotes;
   for(size_t i = 0; i < quotes.size(); ++i)
   {
      const char* const typeStr[] = { "???", "ask", "bid", "trade", "close", "high", "low" };

      // Log trades/close/high/low only, logging BBA will slow down the program a lot
      if(quotes[i].type > cqg::QuoteInfo::Trade)
      {

         CString str;
         str.Format("[QUOTE] %s at price %g, volume %d",
            typeStr[quotes[i].type],
            quotes[i].price,
            quotes[i].volume);

         writeLn(str);
      }
   }
}

void CCQGAPIFacadeTestDlg::OnAccountsReloaded()
{
   writeLn("Accounts reloaded");

   // Now we are ready to trade, request symbol market data, then place order
   ATLVERIFY(m_api->RequestSymbol("CLE"));
}

void CCQGAPIFacadeTestDlg::OnPositionsReloaded()
{
   writeLn("Positions reloaded");
}

void CCQGAPIFacadeTestDlg::OnAccountChanged(const cqg::AccountInfo& account)
{
   CString accountStr;
   accountStr.Format("Account updated: %s, ID %d",
      account.gwAccountName.GetString(),
      account.gwAccountID);

   writeLn(accountStr);
}

void CCQGAPIFacadeTestDlg::OnPositionChanged(const cqg::AccountInfo& account,
   const cqg::PositionInfo& position, const bool /*newPosition*/)
{
   CString positionStr;
   positionStr.Format("[POSITION] Account %s, %s %s, size %d, avg price %g, OTE: %g, P/L: %g",
      account.gwAccountName.GetString(),
      position.longPosition ? "long" : "short",
      position.symbol.GetString(),
      position.quantity,
      position.averagePrice,
      position.ote,
      position.profitLoss);

   writeLn(positionStr);
}

void CCQGAPIFacadeTestDlg::OnOrderChanged(const cqg::OrderInfo& order)
{
   printWorkingOrders();

   CString orderStr;
   orderStr.Format("[ORDER] %s: %s, filled qty %d of %d, description: %s, GW ID: %s, GUID: %s",
      order.symbol.GetString(),
      order.final ? "closed" : "working",
      order.filledQty,
      order.quantity,
      order.description.GetString(),
      order.gwOrderID.GetString(),
      order.orderGuid.GetString());

   writeLn(orderStr);

   if(!order.error.IsEmpty())
   {
      writeLn("[ORDER] " + order.error);
   }

   if(order.orderGuid == m_stpOrderGuid)
   {
      if(!m_api->CancelOrder(m_stpOrderGuid))
      {
         writeLn("[ORDER] Unable to cancel order " + m_stpOrderGuid + ": " + m_api->GetLastError());
      }
      else
      {
         writeLn("[ORDER] cancel requested for " + m_stpOrderGuid);
         m_stpOrderGuid.Empty();
      }
   }
}

void CCQGAPIFacadeTestDlg::OnBarsReceived(const cqg::Bars& receivedBars)
{
   const CString symbol = m_barReqSymbols[receivedBars.requestGuid];
   m_barReqSymbols.erase(receivedBars.requestGuid);

   if(!receivedBars.error.IsEmpty())
   {
      writeLn("[BARS] Error requesting symbol " + symbol + ": " + receivedBars.error);
      return;
   }

   writeLn("[BARS] " + symbol + " request succeed: " + receivedBars.requestGuid);

   if(receivedBars.bars.size() < 48)
   {
      writeLn("[BARS] Result contains less bars then required, re-requesting...");
      requestBars(symbol, receivedBars.requestedCount * 2);
      return;
   }

   // Use only last 48 bars.
   const cqg::BarInfos& bars = receivedBars.bars;
   for(size_t i = bars.size() - 48; i < bars.size(); ++i)
   {
      CString ohlcStr;
      ohlcStr.Format(" OHLC %g/%g/%g/%g", bars[i].open, bars[i].high, bars[i].low, bars[i].close);

      writeLn(" - " + bars[i].timestamp.Format("%Y-%m-%d %H:%M:%S") + ohlcStr);
   }
}

void CCQGAPIFacadeTestDlg::OnBnClickedCancelAll()
{
   printWorkingOrders();

   if(!m_api->CancelAllOrders())
   {
      writeLn("[ORDER] Unable to cancel all orders: " + m_api->GetLastError());
   }
}
