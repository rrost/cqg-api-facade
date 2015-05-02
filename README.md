# CQG API Facade

CQG API Facade library is pure C++ Facade for CQG API which originally is COM/Automation based.
C++ COM/Automation programming is tricky even with MFC/ATL libraries supplied with Microsoft Visual C++, code looks ugly
being overloaded with many details related to COM itself rather than to actual problem to solve.

CQG API Facade is static library that encapsulates CQG API COM guts and exposes simple and clear pure C++ interface
making CQG API usage easy even for novice C++ developer without COM experience.

## CQG API Facade Test

CQG API Facade Test is dialog based MFC/ATL sample application demonstrating usage of CQG API Facade library.

## Excel QuoteBoard

QuoteBoard is typical user interface for displaying market data for several tickers.
It's a sheet with first column used to enter contract symbol, other columns will display various market data like
best bid/ask, volume, last trade etc.

Nice Quote Board samples for Microsoft Excel 2013 are provided:
 - CQGAPIQuoteBoard.xlsm, demonstrates usage of CQG API from VBA
 - CQGRTDQuoteBoard.xlsm, demonstrates usage of CQG RTD Server in Excel

## Versions supported:
- OS: Windows XP or higher
- Compiler: Visual Studio 2010 or higher.
- CQG: CQG API 4.0, CQGIC 13.4 or higher.

## About CQG API

CQG API is created to make CQG market data and trade routing functionality available to customer applications.
The CQG API is an automation server component that can be used from any application written on C++, C#, VBA/Excel
or any other language, which supports automation technologies.

CQG API can be used to retrieve real-time market data on instruments, account and position data, and perform
order placing & tracking.

CQG API is supplied as a part of the CQGIC package. In order for CQG API to work, CQGIC should be started on the computer.

## More info: 
- API Resources http://partners.cqg.com/api-resources
- API Tech Docs: http://partners.cqg.com/api-resources/technical-documentation
- API Samples: http://partners.cqg.com/api-resources/cqg-data-and-trading-apis
