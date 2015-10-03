/// @file CQGAPIFacade.cpp
/// @brief Simple C++ facade for CQG API, v0.12 - implementation.
/// @copyright Licensed under the MIT License.
/// @author Rostislav Ostapenko (rostislav.ostapenko@gmail.com)
/// @date 16-Feb-2015

#include "stdafx.h"

#include "CQGAPIFacade.h"

#include <memory>
#include <string>
#include <exception>
#include <stdexcept>

#include <afx.h>
#include <atlbase.h>
#include <atlcom.h>

#pragma message ("Please make sure that the path of CQGCEL-4_0.dll on your system corresponds to the one given in CQGAPIFacade.cpp file.")
#import "D:\CQGIC\CQGNet\Bin\CQGCEL-4_0.dll" raw_interfaces_only, raw_native_types, no_namespace, named_guids, auto_search

namespace cqg
{

/// @class CCOMInitializer
/// @brief COM subsystem initialization & finalization wrapper
static struct CCOMInitializer
{
   CCOMInitializer()
   {
      // Initialize COM
      ::CoInitialize(NULL);
   }

   ~CCOMInitializer()
   {
      // Finalize COM
      ::CoUninitialize();
   }
} sc_initCOM;

/// @brief Returns latest COM error description
/// @param spClass [in] - Pointer to COM object which raised error
/// @param descr [out] - Error description
/// @return Result code
template <class IFaceT>
STDMETHODIMP GetCOMErrorString(
   const ATL::CComPtr<IFaceT>& spIFace,
   CString& error)
{
   error.Empty();

   ATL::CComPtr<ISupportErrorInfo> spSupportErrorInfo;
   HRESULT hr = spIFace->QueryInterface(__uuidof(ISupportErrorInfo), (void**)&spSupportErrorInfo);

   if(SUCCEEDED(hr))
   {
      hr = spSupportErrorInfo->InterfaceSupportsErrorInfo(__uuidof(IFaceT));
      if(SUCCEEDED(hr))
      {
         ATL::CComPtr<IErrorInfo> spErrorInfo;
         hr = GetErrorInfo(0, &spErrorInfo);

         if(SUCCEEDED(hr))
         {
            ATL::CComBSTR errDesc;
            hr = spErrorInfo->GetDescription(&errDesc);
            error = errDesc;
         }
      }
   }

   return hr;
}

/// @brief Gets COM error description depending on passed result code
/// @param spIFace [in] pointer to object which is the source of the result code
/// @param hr [in] result code
/// @return COM error string
template <class IFaceT>
CString GetCOMError(const ATL::CComPtr<IFaceT>& spIFace, HRESULT hr)
{
   if(SUCCEEDED(hr))
   {
      return CString();
   }

   CString errMessage;
   HRESULT hres = GetCOMErrorString(spIFace, errMessage);

   if(SUCCEEDED(hres))
   {
      errMessage.Insert(0, "COM error occurred. Description: ");
   }
   else
   {
      char* errDesc = NULL;

      ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL,
         hr,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         (LPTSTR)&errDesc,
         0,
         NULL);

      errMessage = errDesc;
      ::LocalFree(errDesc);
   }

   return errMessage;
}

/// @brief Throws runtime exception depending on passed result code
/// @param spIFace - Pointer to object which is the source of the result code
/// @param hr - Result code
template <class IFaceT>
void CheckCOMError(const ATL::CComPtr<IFaceT>& spIFace, HRESULT hr)
{
   if(SUCCEEDED(hr))
   {
      return;
   }

   throw std::runtime_error(std::string(GetCOMError(spIFace, hr).GetString()));
}

template <class ICollectionT, typename ItemT>
class CComCollection
{
public:

   CComCollection(ICollectionT* collection): m_isEnd(true)
   {
      if(collection)
      {
         HRESULT hr = collection->get__NewEnum(&m_collection);
         CheckCOMError<ICollectionT>(collection, hr);

         Reset();
      }
   }

   bool IsEnd() const
   {
      return m_isEnd;
   }

   void Reset()
   {
      if(m_collection)
      {
         HRESULT hr = m_collection->Reset();
         CheckCOMError(m_collection, hr);
         m_isEnd = false;
      }
   }

   ATL::CComVariant GetNext()
   {
      m_isEnd = true;

      ATL::CComVariant result;
      if(m_collection)
      {
         HRESULT hr = m_collection->Next(1, &result, NULL);
         CheckCOMError(m_collection, hr);
         m_isEnd = (hr == S_FALSE);
      }

      return result;
   }

private:
   bool m_isEnd;
   ATL::CComPtr<IEnumVARIANT> m_collection;
};

bool GetQuote(ICQGQuote* quote, QuoteInfo& quoteInfo)
{
   if(!quote)
   {
      ATLASSERT(0);
      return false;
   }

   VARIANT_BOOL valid = VARIANT_FALSE;
   quote->get_IsValid(&valid);

   if(valid == VARIANT_FALSE)
   {
      return false;
   }

   eQuoteType type;
   quote->get_Type(&type);

   quoteInfo.type = QuoteInfo::Unknown;
   if(type == qtAsk) quoteInfo.type = QuoteInfo::Ask;
   else if(type == qtBid) quoteInfo.type = QuoteInfo::Bid;
   else if(type == qtTrade) quoteInfo.type = QuoteInfo::Trade;
   else if(type == qtYesterdaySettlement) quoteInfo.type = QuoteInfo::Close;
   else if(type == qtDayHigh) quoteInfo.type = QuoteInfo::High;
   else if(type == qtDayLow) quoteInfo.type = QuoteInfo::Low;
   else
   {
      return false;
   }

   quote->get_Price(&quoteInfo.price);
   quote->get_Volume(&quoteInfo.volume);

   return true;
}

void GetAllQuotes(ICQGQuotes* quotes, SymbolInfo& symInfo)
{
   if(!quotes)
   {
      return;
   }

   CComCollection<ICQGQuotes, ICQGQuote> q(quotes);
   while(!q.IsEnd())
   {
      ATL::CComVariant v = q.GetNext();
      if(q.IsEnd()) break;

      ATL::CComQIPtr<ICQGQuote> spQuote = v.pdispVal;

      QuoteInfo quote;
      if(GetQuote(spQuote, quote))
      {
         symInfo.lastQuotes.push_back(quote);
      }
   }
}

int GetWorkingOrders(ICQGOrders* orders)
{
   if(!orders)
   {
      return 0;
   }

   int count = 0;
   CComCollection<ICQGOrders, ICQGOrder> q(orders);
   while(!q.IsEnd())
   {
      ATL::CComVariant v = q.GetNext();
      if(q.IsEnd()) break;

      ATL::CComQIPtr<ICQGOrder> spOrder = v.pdispVal;
      VARIANT_BOOL state = VARIANT_FALSE;
      spOrder->get_IsFinal(&state);

      if(state == VARIANT_FALSE) ++count;
   }

   return count;
}

void GetAccountInfo(ICQGAccount* acc, ICQGAccountSummary* accSum, AccountInfo& account)
{
   if(!acc)
   {
      ATLASSERT(0);
      return;
   }

   acc->get_FcmID(&account.fcmID);

   ATL::CComBSTR strFcmAccountID;
   acc->get_FcmAccountID(&strFcmAccountID);
   account.fcmAccountID = strFcmAccountID;

   acc->get_GWAccountID(&account.gwAccountID);

   ATL::CComBSTR strGWAccountName;
   acc->get_GWAccountName(&strGWAccountName);
   account.gwAccountName = strGWAccountName;

   ATL::CComBSTR strCurrency;
   acc->get_ReportingCurrency(&strCurrency);
   account.currency = strCurrency;

   account.balance = 0.0;
   account.ote = 0.0;
   account.profitLoss = 0.0;

   accSum->Balance(0, &account.balance);
   accSum->OTE(0, &account.ote);
   accSum->ProfitLoss(0, &account.profitLoss);
}

void GetPositionInfo(ICQGPosition* pos, PositionInfo& position)
{
   if(!pos)
   {
      ATLASSERT(0);
      return;
   }

   ATL::CComBSTR strSymbol;
   pos->get_InstrumentName(&strSymbol);
   position.symbol = strSymbol;

   eOrderSide side = osdUndefined;
   pos->get_Side(&side);
   position.longPosition = side == osdBuy;

   long qty = 0;
   pos->get_Quantity(&qty);
   position.quantity = qty;

   position.averagePrice = InvalidPrice;
   position.ote = 0.0;
   position.profitLoss = 0.0;

   pos->get_AveragePrice(&position.averagePrice);
   pos->get_OTE(&position.ote);
   pos->get_ProfitLoss(&position.profitLoss);
}

class CQGCELWrapper;

typedef ATL::IDispEventImpl<1, CQGCELWrapper,
   &__uuidof(_ICQGCELEvents), &__uuidof(__CQG), 4, 0> ICQGCELDispEventImpl;

/// @class CCQGCELWrapper
class CQGCELWrapper : public ICQGCELDispEventImpl
{
public:

   friend struct IAPIFacadeImpl;

   /// @brief Initializes CQG API Facade
   CQGCELWrapper(IAPIEvents* events): m_events(events)
   {
      initializeCQGCEL();
   }

   /// @brief Finalizes CQG API Facade
   ~CQGCELWrapper()
   {
      finalizeCQGCEL();
   }

   /// @brief This map is used to declare handler function for specified event.
   ///        You can use "OLE/COM Object Viewer" to open CQGCEL-4_0.dll and get event ids.
   ///        The handler signature as well as the event ids can be found in the type library
   ///        or shown by "OLE/COM Object Viewer".
   BEGIN_SINK_MAP(CQGCELWrapper)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 10, OnDataError)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 2,  OnGWConnectionStatusChanged)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 3,  OnDataConnectionStatusChanged)

      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 7,  OnAccountChanged)

      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 4,  OnInstrumentSubscribed)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 5,  OnInstrumentChanged)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 12, OnIncorrectSymbol)

      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 17, OnOrderChanged)

      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 24, OnTimedBarsResolved)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 25, OnTimedBarsAdded)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 26, OnTimedBarsUpdated)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 57, OnTimedBarsInserted)
      SINK_ENTRY_EX(1, __uuidof(_ICQGCELEvents), 58, OnTimedBarsRemoved)
   END_SINK_MAP()

private:

   /// @brief Handle event of some abnormal discrepancy
   ///        between data expected and data received from CQG.
   ///        This event also can be fired when CQGCEL startup fails.
   /// @param obj [in] Object where the error occurred
   /// @param errorDescription [in] String describing the error
   /// @return S_OK
   STDMETHOD(OnDataError)(
      LPDISPATCH /*obj*/,
      BSTR errorDescription)
   {
      ATLTRACE("CQGCEL::OnDataError\n");

      if(m_events)
      {
         m_events->OnError(CString(errorDescription));
      }

      return S_OK;
   }

   /// @brief Fired when some changes occurred
   ///        in the connection with the CQG Gateway.
   /// @param newStatus [in] New connection status
   /// @return S_OK
   STDMETHOD(OnGWConnectionStatusChanged)(
      eConnectionStatus newStatus)
   {
      ATLTRACE("CQGCEL::OnGWConnectionStatusChanged\n");

      const bool connected = (newStatus == csConnectionUp);
      if(connected)
      {
         // Subscribe to accounts, positions & order events
         HRESULT hr = m_spCQGCEL->put_AccountSubscriptionLevel(aslAccountUpdatesAndOrders);
         CheckCOMError(m_spCQGCEL, hr);
      }

      if(m_events)
      {
         m_events->OnTradingConnection(connected);
      }

      return S_OK;
   }

   /// @brief Fired when some changes occurred
   ///        in the connection with the CQG data server.
   /// @param newStatus [in] New connection status
   /// @return S_OK
   STDMETHOD(OnDataConnectionStatusChanged)(
      eConnectionStatus newStatus)
   {
      ATLTRACE("CQGCEL::OnDataConnectionStatusChanged\n");

      if(m_events)
      {
         m_events->OnMarketDataConnection(newStatus == csConnectionUp);
      }

      return S_OK;
   }

   /// @brief Fired when new account is added, changed or removed.
   /// @param change [in] Account change type
   /// @param account [in] Changed account instance
   /// @param position [in] Changed position instance
   /// @return S_OK
   STDMETHOD(OnAccountChanged)(
      eAccountChangeType change,
      ICQGAccount* account,
      ICQGPosition* position)
   {
      ATLTRACE("CQGCEL::OnAccountChanged\n");

      if(!m_events)
      {
         return S_OK;
      }

      if(change == actAccountsReloaded)
      {
         m_events->OnAccountsReloaded();
      }
      else if(change == actPositionsReloaded)
      {
         m_events->OnPositionsReloaded();
      }
      else if(change == actAccountChanged || change == actPositionAdded || change == actPositionChanged)
      {
         ATL::CComPtr<ICQGAccountSummary> spAccSum;
         HRESULT hr = account->get_Summary(&spAccSum);
         CheckCOMError<ICQGAccount>(account, hr);

         AccountInfo accountInfo;
         GetAccountInfo(account, spAccSum, accountInfo);

         if(change == actAccountChanged)
         {
            m_events->OnAccountChanged(accountInfo);
         }
         else
         {
            PositionInfo positionInfo;
            GetPositionInfo(position, positionInfo);

            m_events->OnPositionChanged(accountInfo, positionInfo, change == actPositionAdded);
         }
      }

      return S_OK;
   }

   /// @brief Fired when new instrument is resolved and subscribed.
   /// @param symbol [in] Requested symbol
   /// @param instrument [in] Subscribed instrument object
   /// @return S_OK
   STDMETHOD(OnInstrumentSubscribed)(
      BSTR symbol,
      ICQGInstrument* instrument)
   {
      ATLTRACE("CQGCEL::OnInstrumentSubscribed\n");

      if(m_events)
      {
         ATL::CComBSTR strSymbol;
         HRESULT hr = instrument->get_FullName(&strSymbol);
         CheckCOMError<ICQGInstrument>(instrument, hr);

         SymbolInfo symInfo;
         symInfo.fullName = strSymbol;

         ATL::CComPtr<ICQGQuotes> quotes;
         hr = instrument->get_Quotes(&quotes);
         CheckCOMError<ICQGInstrument>(instrument, hr);

         GetAllQuotes(quotes, symInfo);

         m_events->OnSymbolSubscribed(CString(symbol), symInfo);
      }

      return S_OK;
   }

   /// @brief Fired when any of instrument quotes or
   ///        dynamic instrument properties are changed.
   /// @param instrument [in] Changed instrument object
   /// @param quotes [in] Changed quotes collection
   /// @param props [in] Changed properties collection
   /// @return S_OK
   STDMETHOD(OnInstrumentChanged)(
      ICQGInstrument* instrument,
      ICQGQuotes* quotes,
      ICQGInstrumentProperties* /*props*/)
   {
      ATLTRACE("CQGCEL::OnInstrumentChanged\n");

      if(m_events)
      {
         ATL::CComBSTR str;
         HRESULT hr = instrument->get_FullName(&str);
         CheckCOMError<ICQGInstrument>(instrument, hr);

         SymbolInfo symInfo;
         symInfo.fullName = str;

         GetAllQuotes(quotes, symInfo);

         m_events->OnSymbolQuote(symInfo);
      }

      return S_OK;
   }

   /// @brief This event is fired when a not tradable symbol name has been 
   ///         passed to the NewInstrument method.
   /// @param wrongSymbol [in] Requested symbol
   /// @return S_OK
   STDMETHOD(OnIncorrectSymbol)(BSTR wrongSymbol)
   {
      ATLTRACE("CQGCEL::OnIncorrectSymbol\n");

      if(m_events)
      {
         m_events->OnSymbolError(CString(wrongSymbol));
      }

      return S_OK;
   }

   /// @brief Fired when new order was added, order status was 
   ///        changed or the order was removed.
   /// @param change [in] The type of the occurred change
   /// @param order [in] CQGOrder object representing the order to which the change refers
   /// @param oldProperties [in] CQGOrderProperties collection representing 
   ///        the old values of the changed order properties
   /// @param fill [in] CQGFill object representing the last fill of the order
   /// @param cqgerr [in] CQGError object to describe the last error, if any
   /// @return S_OK
   STDMETHOD(OnOrderChanged)(
      eChangeType /*change*/,
      ICQGOrder* order,
      ICQGOrderProperties* /*oldProperties*/,
      ICQGFill* fill,
      ICQGError* cqgerr)
   {
      ATLTRACE("CQGCEL::OnOrderChanged\n");

      if(m_events)
      {
         OrderInfo orderInfo;

         ATL::CComBSTR strGuid;
         order->get_GUID(&strGuid);
         orderInfo.orderGuid = strGuid;

         ATL::CComBSTR strSymbol;
         order->get_InstrumentName(&strSymbol);
         orderInfo.symbol = strSymbol;

         ATL::CComPtr<ICQGAccount> spAcc;
         order->get_Account(&spAcc);
         spAcc->get_GWAccountID(&orderInfo.gwAccountID);

         eOrderSide side = osdUndefined;
         order->get_Side(&side);
         orderInfo.buy = side == osdBuy;

         VARIANT_BOOL state = VARIANT_FALSE;
         order->get_IsFinal(&state);
         orderInfo.final = state == VARIANT_TRUE;

         long qty = 0;
         order->get_Quantity(&qty);
         orderInfo.quantity = qty;

         long filledQty = 0;
         order->get_FilledQuantity(&filledQty);
         orderInfo.filledQty = filledQty;

         ATL::CComBSTR description;
         order->get_Description(&description);
         orderInfo.description = description;

         ATL::CComBSTR originOrderID;
         order->get_OriginalOrderID(&originOrderID);
         orderInfo.gwOrderID = originOrderID;

         if(checkValid(fill))
         {
            long legCount = 0;
            fill->get_LegCount(&legCount);

            eFillStatus status = fsNormal;
            fill->get_Status(&status);

            orderInfo.orderFills.reserve(legCount);

            for(long i = 0; i < legCount; ++i)
            {
               FillInfo fillInfo;

               fillInfo.canceled = (status == fsCanceled) || (status == fsBusted);

               ATL::CComBSTR strSymbol;
               fill->get_InstrumentName(i, &strSymbol);
               fillInfo.symbol = strSymbol;

               fill->get_Price(i, &fillInfo.fillPrice);
               fill->get_Quantity(i, &fillInfo.fillQty);

               orderInfo.orderFills.push_back(fillInfo);
            }
         }

         if(checkValid(cqgerr))
         {
            ATL::CComBSTR errorDesc;
            cqgerr->get_Description(&errorDesc);
            orderInfo.error = errorDesc;
         }

         m_events->OnOrderChanged(orderInfo);
      }

      return S_OK;
   }

   STDMETHOD(OnTimedBarsResolved)(
      ICQGTimedBars* cqgTimedBars,
      ICQGError* cqgerr)
   {
      ATLTRACE("CQGCEL::OnTimedBarsResolved\n");

      if(m_events)
      {
         ATL::CComBSTR requestID;
         Bars bars;

         cqgTimedBars->get_Id(&requestID);
         bars.requestGuid = CString(requestID);

         if(checkValid(cqgerr))
         {
            ATL::CComBSTR errorDesc;
            cqgerr->get_Description(&errorDesc);
            bars.error = errorDesc;
         }

         eRequestStatus status;
         cqgTimedBars->get_Status(&status);
         if(bars.error.IsEmpty() && status != rsSuccess)
         {
            bars.error = "Bars request failed, cancelled or pending.";
         }

         long barCount = 0;
         cqgTimedBars->get_Count(&barCount);

         bars.bars.reserve(barCount);

         for(long i = 0; i < barCount; ++i)
         {
            ATL::CComPtr<ICQGTimedBar> spBar;
            cqgTimedBars->get_Item(i, &spBar);

            BarInfo bar;
            spBar->get_Timestamp(&bar.timestamp.m_dt);
            spBar->get_Open(&bar.open);
            spBar->get_High(&bar.high);
            spBar->get_Low(&bar.low);
            spBar->get_Close(&bar.close);

            bars.bars.push_back(bar);
         }

         m_events->OnBarsReceived(bars);
      }

      return S_OK;
   }

   STDMETHOD(OnTimedBarsAdded)(
      ICQGTimedBars* /*cqgTimedBars*/)
   {
      ATLTRACE("CQGCEL::OnTimedBarsAdded\n");
      return S_OK;
   }

   STDMETHOD(OnTimedBarsUpdated)(
      ICQGTimedBars* /*cqgTimedBars*/,
      long /*barIndex*/)
   {
       ATLTRACE("CQGCEL::OnTimedBarsUpdated\n");
       return S_OK;
   }

   STDMETHOD(OnTimedBarsInserted)(
      ICQGTimedBars* /*cqgTimedBars*/,
      long /*barIndex*/)
   {
      ATLTRACE("CQGCEL::OnTimedBarsInserted\n");
      return S_OK;
   }

   STDMETHOD(OnTimedBarsRemoved)(
      ICQGTimedBars* /*cqgTimedBars*/,
      long /*barIndex*/)
   {
      ATLTRACE("CQGCEL::OnTimedBarsRemoved\n");
      return S_OK;
   }

private:

   /// @brief Initializes CQGCEL object, starts the CQGCEL and subscribes to events.
   void initializeCQGCEL()
   {
      // Create an instance of CQG API
      HRESULT hr = m_spCQGCEL.CoCreateInstance(__uuidof(CQGCEL), NULL, CLSCTX_INPROC_SERVER);
      if(FAILED(hr))
      {
         throw std::runtime_error("Unable to create CQGCEL COM object. "
            "Please register it again and restart application.");
      }

      ATLASSERT(m_spCQGCEL);

      // Configure CQGCEL behavior
      ATL::CComPtr<ICQGAPIConfig> spConf;
      hr = m_spCQGCEL->get_APIConfiguration(&spConf);
      CheckCOMError(m_spCQGCEL, hr);

      hr = spConf->put_ReadyStatusCheck(rscOff);
      CheckCOMError(spConf, hr);

      hr = spConf->put_UsedFromATLClient(VARIANT_TRUE);
      CheckCOMError(spConf, hr);

      hr = spConf->put_CollectionsThrowException(VARIANT_FALSE);
      CheckCOMError(spConf, hr);

      hr = spConf->put_TimeZoneCode(tzCentral);
      CheckCOMError(spConf, hr);

      hr = spConf->put_UseOrderSide(VARIANT_TRUE);
      CheckCOMError(spConf, hr);

      // Default is dsQuotesAndBBA - receive best bid/best ask and trade quotes
      // To switch to trades only market data notifications replace X with dsQuotes
      // To switch to trades & full DOM market data notifications replace X with dsQuotesAndDOM
      // hr = spConf->put_DefaultInstrumentSubscriptionLevel();
      // CheckCOMError(spConf, hr);

      // Switch full position notifications
      hr = spConf->put_DefPositionSubscriptionLevel(pslSnapshotAndUpdates);
      CheckCOMError(spConf, hr);

      // Now advise the connection, to get events
      ATLVERIFY(SUCCEEDED(ICQGCELDispEventImpl::DispEventAdvise(m_spCQGCEL)));

      // Start CQGCEL
      hr = m_spCQGCEL->Startup();
      CheckCOMError(m_spCQGCEL, hr);
   }

   /// @brief Unsubscribes from events, shuts down the CQGCEL and finalizes CQGCEL object.
   void finalizeCQGCEL() throw()
   {
      m_events = NULL;
      if (m_spCQGCEL)
      {
         ATLVERIFY(SUCCEEDED(ICQGCELDispEventImpl::DispEventUnadvise(m_spCQGCEL)));
         ATLVERIFY(SUCCEEDED(m_spCQGCEL->Shutdown()));
         m_spCQGCEL.Release();
      }
   }

   template <typename Interface>
   bool checkValid(Interface* obj)
   {
      if(!obj)
      {
         return false;
      }

      VARIANT_BOOL valid = VARIANT_FALSE;
      m_spCQGCEL->IsValid(ATL::CComVariant(obj), &valid);
      return (valid == VARIANT_TRUE);
   }

   ATL::CComPtr<ICQGCEL> m_spCQGCEL; ///< CQGCEL object.
   IAPIEvents* m_events;             ///< User API events listener.
}; // class CQGCELWrapper


#define CHECK_CEL_INIT(res)                 \
m_lastError.Empty();                        \
if(!IsValid())                              \
{                                           \
   ATLASSERT(0);                            \
   m_lastError = "CQGCVEL not initialized"; \
   return res;                              \
}

#define RETURN_CEL_RESULT(hr)                     \
m_lastError = GetCOMError(m_api->m_spCQGCEL, hr); \
return m_lastError.IsEmpty();

#define RETURN_CEL_OBJ_RESULT(obj, hr, res) \
m_lastError = GetCOMError(obj, hr);         \
return res;

#define CHECK_CEL_OBJ_RESULT(obj, hr, res) \
if(hr != S_OK)                             \
{                                          \
   RETURN_CEL_OBJ_RESULT(obj, hr, res)     \
}

/// @class IAPIFacadeImpl
struct IAPIFacadeImpl: IAPIFacade
{

   virtual bool IsValid()
   {
      return m_api.get() != NULL;
   }

   virtual CString GetLastError()
   {
      return m_lastError;
   }

   virtual bool Initialize(IAPIEvents* events)
   {
      m_lastError.Empty();

      if(m_api.get() != NULL)
      {
         ATLASSERT(0);
         m_lastError = "CQGCEL already initialized";
         return false;
      }

      try
      {
         m_api.reset(new CQGCELWrapper(events));
      }
      catch(std::exception& ex)
      {
         m_lastError = CString("Unable to initialize CQGCEL: ") + ex.what();
         return false;
      }
      catch(...)
      {
         m_lastError = "Unable to initialize CQGCEL: Unknown exception";
         return false;
      }

      return true;
   }

   virtual bool RequestSymbol(const CString& symbol)
   {
      CHECK_CEL_INIT(false);
      RETURN_CEL_RESULT(m_api->m_spCQGCEL->NewInstrument(ATL::CComBSTR(symbol)));
   }

   virtual CString RequestBars(const BarsRequest& barsRequest)
   {
      CHECK_CEL_INIT(CString());

      ATL::CComPtr<ICQGTimedBarsRequest> spRequest;
      HRESULT hr = m_api->m_spCQGCEL->CreateTimedBarsRequest(&spRequest);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, CString());

      ATL::CComBSTR symbol(barsRequest.symbol);
      hr = spRequest->put_Symbol(symbol);
      CHECK_CEL_OBJ_RESULT(spRequest, hr, CString());

      hr = spRequest->put_RangeStart(
         ATL::CComVariant(barsRequest.rangeStart.m_dt, VT_DATE));
      CHECK_CEL_OBJ_RESULT(spRequest, hr, CString());

      hr = spRequest->put_RangeEnd(
         ATL::CComVariant(barsRequest.rangeEnd.m_dt, VT_DATE));
      CHECK_CEL_OBJ_RESULT(spRequest, hr, CString());

      hr = spRequest->put_IntradayPeriod(barsRequest.intradayPeriodInMinutes);
      CHECK_CEL_OBJ_RESULT(spRequest, hr, CString());

      ATL::CComPtr<ICQGTimedBars> spTimedBars;
      hr = m_api->m_spCQGCEL->RequestTimedBars(spRequest, &spTimedBars);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, CString());

      ATL::CComBSTR requestID;
      spTimedBars->get_Id(&requestID);
      return CString(requestID);
   }

   virtual bool LogonToGateway(const CString& user, const CString& password)
   {
      CHECK_CEL_INIT(false);
      RETURN_CEL_RESULT(m_api->m_spCQGCEL->GWLogon(ATL::CComBSTR(user), ATL::CComBSTR(password)));
   }

   virtual COleDateTime GetLineTime()
   {
      COleDateTime invalidTime;
      invalidTime.SetStatus(COleDateTime::invalid);

      ATL::CComPtr<ICQGEnvironment> spEnvironment;
      HRESULT hr = m_api->m_spCQGCEL->get_Environment(&spEnvironment);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, invalidTime);

      DATE lineTime;
      hr = spEnvironment->get_LineTime(&lineTime);
      CHECK_CEL_OBJ_RESULT(spEnvironment, hr, invalidTime);

      if(lineTime == 0.0) return invalidTime;

      return COleDateTime(lineTime);
   }

   virtual bool GetAccounts(Accounts& accounts)
   {
      accounts.clear();

      CHECK_CEL_INIT(false);

      ATL::CComPtr<ICQGAccounts> spAccounts;
      HRESULT hr = m_api->m_spCQGCEL->get_Accounts(&spAccounts);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, false);

      long count = 0;
      hr = spAccounts->get_Count(&count);
      CHECK_CEL_OBJ_RESULT(spAccounts, hr, false);

      accounts.reserve(count);

      for(long i = 0; i < count; ++i)
      {
         ATL::CComPtr<ICQGAccount> spAcc;
         hr = spAccounts->get_ItemByIndex(i, &spAcc);
         CHECK_CEL_OBJ_RESULT(spAccounts, hr, false);

         ATL::CComPtr<ICQGAccountSummary> spAccSum;
         if(spAcc)
         {
            hr = spAcc->get_Summary(&spAccSum);
            CHECK_CEL_OBJ_RESULT(spAcc, hr, false);
         }

         AccountInfo account;
         GetAccountInfo(spAcc, spAccSum, account);
         accounts.push_back(account);
      }

      return true;
   }

   typedef ATL::CComPtr<ICQGAccount> ICQGAccountPtr;

   ICQGAccountPtr getAccount(const ID& gwAccountID)
   {
      ICQGAccountPtr spAccount;

      ATL::CComPtr<ICQGAccounts> spAccounts;
      HRESULT hr = m_api->m_spCQGCEL->get_Accounts(&spAccounts);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, spAccount);

      hr = spAccounts->get_Item(gwAccountID, &spAccount);
      CHECK_CEL_OBJ_RESULT(spAccounts, hr, spAccount);

      return spAccount;
   }

   virtual bool GetPositions(const ID& gwAccountID, Positions& positions)
   {
      positions.clear();

      CHECK_CEL_INIT(false);

      ICQGAccountPtr spAccount = getAccount(gwAccountID);
      if(!spAccount) return false;

      ATL::CComPtr<ICQGPositions> spPositions;
      HRESULT hr = spAccount->get_Positions(&spPositions);
      CHECK_CEL_OBJ_RESULT(spAccount, hr, false);

      long count = 0;
      hr = spPositions->get_Count(&count);
      CHECK_CEL_OBJ_RESULT(spPositions, hr, false);

      positions.reserve(count);

      for(long i = 0; i < count; ++i)
      {
         ATL::CComPtr<ICQGPosition> spPos;
         hr = spPositions->get_ItemByIndex(i, &spPos);
         CHECK_CEL_OBJ_RESULT(spPositions, hr, false);

         PositionInfo position;
         GetPositionInfo(spPos, position);
         positions.push_back(position);
      }

      return true;
   }

   typedef ATL::CComPtr<ICQGOrders> ICQGOrdersPtr;

   virtual int GetAllWorkingOrdersCount(const ID& gwAccountID)
   {
      CHECK_CEL_INIT(0);

      ICQGOrdersPtr spOrders;

      if(gwAccountID == ID())
      {
         HRESULT hr = m_api->m_spCQGCEL->get_Orders(&spOrders);
         CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, 0);
      }
      else
      {
         ICQGAccountPtr spAccount = getAccount(gwAccountID);
         if(!spAccount) return 0;

         HRESULT hr = spAccount->get_Orders(&spOrders);
         CHECK_CEL_OBJ_RESULT(spAccount, hr, 0);
      }

      return GetWorkingOrders(spOrders);
   }

   virtual int GetInternalWorkingOrdersCount(const ID& gwAccountID)
   {
      CHECK_CEL_INIT(0);

      ICQGOrdersPtr spOrders;

      if(gwAccountID == ID())
      {
         HRESULT hr = m_api->m_spCQGCEL->get_InternalOrders(&spOrders);
         CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, 0);
      }
      else
      {
         ICQGAccountPtr spAccount = getAccount(gwAccountID);
         if(!spAccount) return 0;

         HRESULT hr = spAccount->get_InternalOrders(&spOrders);
         CHECK_CEL_OBJ_RESULT(spAccount, hr, 0);
      }

      return GetWorkingOrders(spOrders);
   }

   virtual CString PlaceOrder(
      OrderType type,
      const ID& gwAccountID,
      const CString& symbolFullName,
      bool buy,
      Quantity quantity,
      const CString& description,
      const OrderPrice& price,
      const OrderPrice& stopLimitPrice)
   {
      CHECK_CEL_INIT((CString()));

      ATL::CComPtr<ICQGAccounts> spAccounts;
      HRESULT hr = m_api->m_spCQGCEL->get_Accounts(&spAccounts);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, (CString()));

      ATL::CComPtr<ICQGAccount> spAccount;
      hr = spAccounts->get_Item(gwAccountID, &spAccount);
      CHECK_CEL_OBJ_RESULT(spAccounts, hr, (CString()));

      ATL::CComPtr<ICQGInstruments> spInstruments;
      hr = m_api->m_spCQGCEL->get_Instruments(&spInstruments);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, (CString()));

      ATL::CComPtr<ICQGInstrument> spInstrument;
      spInstruments->get_Item(ATL::CComVariant(symbolFullName.GetString()), &spInstrument);
      CHECK_CEL_OBJ_RESULT(spInstruments, hr, (CString()));

      const eOrderType ordType = 
         type == Limit ? otLimit :
            type == Stop ? otStop :
               type == StopLimit ? otStopLimit : otMarket;

      const OrderPrice& limitPrice = 
         type == Limit ? price : 
            type == StopLimit ? stopLimitPrice : OrderPrice();

      const OrderPrice& stopPrice =
         type == Stop || type == StopLimit ? price : OrderPrice();

      // Create order
      ATL::CComPtr<ICQGOrder> spOrder;
      hr = m_api->m_spCQGCEL->CreateOrder(ordType, spInstrument, spAccount,
         quantity, buy ? osdBuy : osdSell, 
         limitPrice.initialized() ? limitPrice.price() : 0.0,
         stopPrice.initialized() ? stopPrice.price() : 0.0,
         L"", &spOrder);

      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, (CString()));

      // Set order description
      hr = spOrder->put_Description(ATL::CComBSTR(description));
      CHECK_CEL_OBJ_RESULT(spOrder, hr, (CString()));

      // Place order
      hr = spOrder->Place();
      CHECK_CEL_OBJ_RESULT(spOrder, hr, (CString()));

      // Return order guid
      ATL::CComBSTR orderGuid;
      spOrder->get_GUID(&orderGuid);
      return CString(orderGuid);
   }

   virtual bool CancelOrder(const CString& orderGuid)
   {
      CHECK_CEL_INIT(false);

      ATL::CComPtr<ICQGOrders> spOrders;
      HRESULT hr = m_api->m_spCQGCEL->get_Orders(&spOrders);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, false);

      ATL::CComBSTR bstrOrderGuid(orderGuid);
      ATL::CComPtr<ICQGOrder> spOrder;
      hr = spOrders->get_ItemByGuid(bstrOrderGuid, &spOrder);
      if(hr == S_FALSE)
      {
         m_lastError = "Order with given guid not found.";
         return false;
      }

      CHECK_CEL_OBJ_RESULT(spOrders, hr, false);

      if(!spOrder)
      {
         m_lastError = "CQGOrder object is NULL.";
         return false;
      }

      VARIANT_BOOL canBeCanceled = VARIANT_FALSE;
      spOrder->get_CanBeCanceled(&canBeCanceled);
      if(canBeCanceled == VARIANT_FALSE)
      {
         m_lastError = "Order cannot be cancelled.";
         return false;
      }

      hr = spOrder->Cancel();
      CHECK_CEL_OBJ_RESULT(spOrder, hr, false);

      return true;
   }

   virtual bool CancelAllOrders(
      const ID& gwAccountID,
      const CString& symbolFullName)
   {
      CHECK_CEL_INIT(false);

      ATL::CComPtr<ICQGAccount> spAccount;
      ATL::CComPtr<ICQGInstrument> spInstrument;
      HRESULT hr = S_OK;

      if(gwAccountID)
      {
         ATL::CComPtr<ICQGAccounts> spAccounts;
         hr = m_api->m_spCQGCEL->get_Accounts(&spAccounts);
         CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, false);

         hr = spAccounts->get_Item(gwAccountID, &spAccount);
         CHECK_CEL_OBJ_RESULT(spAccounts, hr, false);
      }

      if(!symbolFullName.IsEmpty())
      {
         ATL::CComPtr<ICQGInstruments> spInstruments;
         hr = m_api->m_spCQGCEL->get_Instruments(&spInstruments);
         CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, false);

         spInstruments->get_Item(ATL::CComVariant(symbolFullName.GetString()), &spInstrument);
         CHECK_CEL_OBJ_RESULT(spInstruments, hr, false);
      }

      hr = m_api->m_spCQGCEL->CancelAllOrders(spAccount, spInstrument, VARIANT_FALSE, VARIANT_FALSE, osdUndefined);
      CHECK_CEL_OBJ_RESULT(m_api->m_spCQGCEL, hr, false);

      return true;
   }

   std::auto_ptr<CQGCELWrapper> m_api;
   CString m_lastError;

}; // class IAPIFacadeImpl

IAPIFacadePtr IAPIFacade::Create()
{
   return IAPIFacadePtr(new IAPIFacadeImpl());
}

FacadeVersion IAPIFacade::GetVersion()
{
   const FacadeVersion version = { 0, 11 };
   return version;
}

} // namespace cqg
