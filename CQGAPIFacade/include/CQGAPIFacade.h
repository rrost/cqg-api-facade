/// @file CQGAPIFacade.h
/// @brief Simple C++ facade for CQG API, v0.13.
/// @copyright Licensed under the MIT License.
/// @author Rostislav Ostapenko (rostislav.ostapenko@gmail.com)
/// @date 16-Feb-2015

#pragma once

#include <afx.h>
#include <atlcomtime.h>

#include <limits>
#include <memory>
#include <vector>

namespace cqg
{

typedef double Price;
typedef double MoneyAmount;
typedef long Volume;
typedef long ID;
typedef unsigned Quantity;
typedef CString GWOrderID;

static const Price InvalidPrice = std::numeric_limits<Price>::infinity();
static const MoneyAmount InvalidMoneyAmount = std::numeric_limits<MoneyAmount>::infinity();
static const Volume InvalidVolume = -1;

/// @brief Quote info.
struct QuoteInfo
{
   enum Type { Unknown, Ask, Bid, Trade, Close, High, Low };

   Type type;             ///< Quote type.
   Price price;           ///< Quote price.
   Volume volume;         ///< Quote volume.
};

/// @brief Quotes container.
typedef std::vector<QuoteInfo> Quotes;

/// @brief Resolved symbol information.
struct SymbolInfo
{
   CString fullName;      ///< Full CQG symbol name.
   Quotes lastQuotes;     ///< Last symbol quotes - BBA & trade.
};

/// @brief Account information.
struct AccountInfo
{
   ID fcmID;               ///< FCM ID.
   CString fcmAccountID;   ///< FCM Account ID.
   ID gwAccountID;         ///< CQG Gateway Account ID.
   CString gwAccountName;  ///< CQG Gateway Account Name.
   CString currency;       ///< Account currency.
   MoneyAmount balance;    ///< Account current balance.
   MoneyAmount ote;        ///< Account current Open Trade Equity.
   MoneyAmount profitLoss; ///< Account current Profit/Loss.
};

/// @brief Account position information.
struct PositionInfo
{
   CString symbol;         ///< Full name of position symbol.
   bool longPosition;      ///< Is position long.
   Quantity quantity;      ///< Position quantity.
   Price averagePrice;     ///< Position average price.
   MoneyAmount ote;        ///< Position Open Trade Equity.
   MoneyAmount profitLoss; ///< Position Profit/Loss.
};

/// @brief Order fill information.
struct FillInfo
{
   bool canceled;           ///< True if fill has been canceled.
   CString symbol;          ///< Full name of fill symbol. Can differ from order symbol for spreads.
   Price fillPrice;         ///< Fill price.
   Volume fillQty;          ///< Fill quantity.
};

/// @brief Fills container.
typedef std::vector<FillInfo> Fills;

/// @brief Placed order information.
struct OrderInfo
{
   CString orderGuid;       ///< Unique order identifier.
   GWOrderID gwOrderID;     ///< Gateway order ID.
   CString symbol;          ///< Full name of order symbol.
   ID gwAccountID;          ///< CQG Gateway account ID of order.
   bool buy;                ///< True if order side is buy.
   bool final;              ///< True if order is not working anymore (completelly filled, cancelled or rejected).
   Quantity quantity;       ///< Order quantity.
   Quantity filledQty;      ///< Order filled quantity.
   CString error;           ///< Last order error description, empty if no error.
   Fills orderFills;        ///< Last order fills.
   CString description;     ///< Order description, provided by user. Will be kept by CQG Gateway.
};

/// @brief Used containers.
typedef std::vector<AccountInfo> Accounts;
typedef std::vector<PositionInfo> Positions;
typedef std::vector<SymbolInfo> Symbols;

/// @brief Timed bars request definition.
struct BarsRequest
{
   enum { UseAllSessions = 31 };

   CString symbol;               ///< Symbol to request bars.
   bool useIndexRange;           ///< True if startIndex/endIndex shall be used, false if startDate/endDate.
   COleDateTime startDate;       ///< Bars range start date/time (Line Time).
   COleDateTime endDate;         ///< Bars range end date/time (Line Time).
   long startIndex;              ///< Bars range start index.
   long endIndex;                ///< Bars range end index.
   long intradayPeriodInMinutes; ///< Intraday period in minutes for bars, e.g. 60 for hourly bars.
   long sessionsFilter;          ///< Sessions filter. Special value 31 means all sessions, 0 means primary only.
};

/// @brief Timed bar information.
struct BarInfo
{
   COleDateTime timestamp;  ///< Bar timestamp.
   Price open;              ///< Open price.
   Price high;              ///< High price.
   Price low;               ///< Low price.
   Price close;             ///< Close price.
};

typedef std::vector<BarInfo> BarInfos;

/// @brief Timed bars request result.
struct Bars
{
   CString requestGuid; ///< Timed bars request guid.
   CString error;       ///< Error description, empty if no error.
   long requestedCount; ///< Number of bars requested, may be greater than actually rceeived.
   BarInfos bars;       ///< Received bars.
};

/// @class IAPIEvents
/// @brief Interface for processing CQG API Facade events.
/// @note Must be implemented by user and passed to IAPIFacade::Initialize() to receive events.
struct IAPIEvents
{
   /// @brief Called when some error occured, e.g. CQGCEl is not able to start.
   /// @param error [in] error description.
   virtual void OnError(const CString& error) = 0;

   /// @brief Called when market data connection state is changed.
   /// @param connected [in] true if market data online, false if not.
   virtual void OnMarketDataConnection(const bool connected) = 0;

   /// @brief Called when trading server (CQG Gateway) connection state is changed.
   /// @param connected [in] true if trading server online, false if not.
   virtual void OnTradingConnection(const bool connected) = 0;

   /// @brief Called when requested symbol resolved and subscribed to market data.
   /// @param requestedSymbol [i] requested symbol name.
   ///        Note: it can differ from full name, e.g. "EP" will be resolved to something like "F.US.EPH5".
   /// @param symbol [in] symbol info.
   virtual void OnSymbolSubscribed(const CString& requestedSymbol, const SymbolInfo& symbol) = 0;

   /// @brief Called when requested symbol failed resolution.
   /// @param symbol [in] symbol failed resolution.
   virtual void OnSymbolError(const CString& symbol) = 0;

   /// @brief Called when subscribed symbol quote update occured.
   /// @param symbol [in] symbol info.
   virtual void OnSymbolQuote(const SymbolInfo& symbol) = 0;

   /// @brief Called when general accounts reloading occured.
   ///        Note: usually it's occurred on startup after connection to Gateway is up.
   virtual void OnAccountsReloaded() = 0;

   /// @brief Called when general positions reloading occured.
   ///        Note: usually it's occurred on startup after connection to Gateway is up.
   virtual void OnPositionsReloaded() = 0;

   /// @brief Called when general account update occured.
   /// @param account [in] account info.
   virtual void OnAccountChanged(const AccountInfo& account) = 0;

   /// @brief Called when position update for account occured.
   /// @param account [in] account info.
   /// @param position [in] position info.
   /// @param newPosition [in] true if new position added, false if existing position changed.
   virtual void OnPositionChanged(
      const AccountInfo& account,
      const PositionInfo& position,
      const bool newPosition) = 0;

   /// @brief Called when order status update occurred.
   /// @param order [in] order info.
   virtual void OnOrderChanged(const OrderInfo& order) = 0;

   /// @brief Called when order status update occurred.
   /// @param bars [in] received bars info.
   virtual void OnBarsReceived(const Bars& bars) = 0;

   /// @brief Destructor, must be virtual.
   virtual ~IAPIEvents() {}
};

struct IAPIFacade;

/// @brief Smart pointer holding CQG API Facade instance.
typedef std::auto_ptr<IAPIFacade> IAPIFacadePtr;

/// @class OrderPrice
/// @brief Simple wrapper for order price parameter with initialized flag.
class OrderPrice
{
public:

   OrderPrice(): m_price(), m_initialized(false)
   {}

   OrderPrice(const OrderPrice& rhs): m_price(rhs.m_price), m_initialized(rhs.m_initialized)
   {}

   OrderPrice& operator=(const OrderPrice& rhs)
   {
      m_price = rhs.m_price;
      m_initialized = rhs.m_initialized;
      return *this;
   }

   OrderPrice(Price newPrice): m_price(newPrice), m_initialized(true)
   {}

   OrderPrice& operator=(Price newPrice)
   {
      m_price = newPrice;
      m_initialized = true;
      return *this;
   }

   bool initialized() const { return m_initialized; }

   Price price() const { return m_price; }

private:
   Price m_price;
   bool m_initialized;
};

/// @brief Available order types
enum OrderType { Market, Limit, Stop, StopLimit };

/// @brief facade version numbers.
struct FacadeVersion
{
   unsigned char m_major;
   unsigned char m_minor;
};

/// @brief Checks is date/time object has valid status.
inline bool IsValidDateTime(const COleDateTime& dateTime)
{
   return dateTime.GetStatus() == COleDateTime::valid;
}

/// @class IAPIFacade
/// @brief Interface of CQG API Facade instance.
struct IAPIFacade
{
   /// @brief Creates IAPIFacade implementing instance.
   /// @return IAPIFacade instance.
   static IAPIFacadePtr Create();

   /// @brief Gets IAPIFacade version.
   /// @return IAPIFacade version.
   static FacadeVersion GetVersion();

   /// @brief Checks whether IAPIFacade instance is valid.
   /// @return True if valid, false otherwise.
   virtual bool IsValid() = 0;

   /// @brief Gets last error string description.
   /// @return Last error string.
   virtual CString GetLastError() = 0;

   /// @brief Initializes & starts CQG API, then subscribes to events.
   /// @param events [in] events listener.
   virtual bool Initialize(IAPIEvents* events) = 0;

   /// @brief Requests symbol resolution & market data.
   /// @param symbol [in] symbol to resolve
   ///        Note: it can differ from full name, e.g. "EP" will be re.solved to something like "F.US.EPH5".
   virtual bool RequestSymbol(const CString& symbol) = 0;

   /// @brief Requests timed bars.
   /// @param barsRequest [in] bars request definition.
   /// @return Placed bar request guid or empty string if failed.
   virtual CString RequestBars(const BarsRequest& barsRequest) = 0;

   /// @brief Performs logon to CQG Gateway with given user and password.
   /// @param user [in] user name.
   /// @param password [in] password.
   virtual bool LogonToGateway(const CString& user, const CString& password) = 0;

   /// @brief Gets current CQG Line Time.
   /// @return Current Line Time value or invalid date/time if error occurred or market data
   ///         connection is down.
   virtual COleDateTime GetLineTime() = 0;

   /// @brief Requests all available accounts.
   /// @param accounts [out] available accounts.
   virtual bool GetAccounts(Accounts& accounts) = 0;

   /// @brief Requests all open positions for given account.
   /// @param gwAccountID [in] Gateway account ID to get positions.
   /// @param positions [out] open positions.
   virtual bool GetPositions(const ID& gwAccountID, Positions& positions) = 0;

   /// @brief Requests number of all working orders for given account.
   /// @param gwAccountID [in] account ID. If zero orders for all accounts are counted.
   /// @return Number of all working orders.
   virtual int GetAllWorkingOrdersCount(const ID& gwAccountID = ID()) = 0;

   /// @brief Requests number of working orders placed by this CQG API instance for given account.
   /// @param gwAccountID [in] account ID. If zero orders for all accounts are counted.
   /// @return Number of internal working orders.
   virtual int GetInternalWorkingOrdersCount(const ID& gwAccountID = ID()) = 0;

   /// @brief Places DAY order.
   /// @param gwAccountID [in] Gateway account ID to place order.
   /// @param symbol [in] CQG symbol full name to place order.
   /// @param buy [in] is order side is buy (true) or sell (false).
   /// @param quantity [in] order quantity.
   /// @param description [in] order user description.
   /// @return Placed order guid or empty string if failed.
   virtual CString PlaceOrder(
      OrderType type,
      const ID& gwAccountID,
      const CString& symbolFullName,
      bool buy,
      Quantity quantity,
      const CString& description,
      const OrderPrice& price = OrderPrice(),
      const OrderPrice& stopLimitPrice = OrderPrice()) = 0;

   /// @brief Cancels order with given guid.
   /// @param orderGuid [in] order guid.
   /// @return True if order can be canceled, false otherwise.
   virtual bool CancelOrder(const CString& orderGuid) = 0;

   /// @brief Cancels all orders within given account and symbol.
   /// @param gwAccountID [in] account ID. If zero orders for all accounts are canceled.
   /// @param symbolFullName [in] symbol name. If empty orders for all symbols are canceled.
   /// @return True if orders cancel query successful, false otherwise.
   virtual bool CancelAllOrders(
      const ID& gwAccountID = ID(),
      const CString& symbolFullName = CString()) = 0;

   /// @brief Destructor, must be virtual
   virtual ~IAPIFacade() {}
};

} // namespace cqg
