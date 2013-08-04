#pragma once

#if _WIN32_WINNT < 0x0501
#error _WIN32_WINNT must be at least 0x0501 (XP) for Panacea.HTTP
#endif

#include <Winsock2.h>
#include <Winhttp.h>

namespace Panacea 
{
	namespace IO
	{
		enum HttpStatus
		{
			HttpStatusSysError = 0,
			HttpStatusInternalError = 1,
			HttpStatusOk = 1000
			// Amend DescribeError() if you alter this list
		};

		enum HttpInternalErrors
		{
			HttpInternalNone = 0,
			HttpInternalBadUrl,
			HttpInternalInvalidSession,
			HttpInternalNoHost,
			HttpInternalOpenRequestFailed,
			HttpInternalSetOptionsFailed,
			HttpInternalSendRequestFailed,
			HttpInternalReceiveResponseFailed,
			// Amend DescribeError() if you alter this list
		};

		static const int PortHTTP = 80;
		static const int PortHTTPS = 443;

		class PAPI HttpResult
		{
		public:
			HttpResult( HttpStatus stat, const int size = 0, const void* data = NULL )
			{
				Status = stat;
				SysError = 0;
				InternalError = HttpInternalNone;
				if ( size > 0 )
				{
					Data.resize( size );
					memcpy( Data.data, data, size );
				}
			}
			HttpResult( HttpInternalErrors er )
			{
				Status = HttpStatusInternalError;
				InternalError = er;
			}
			static HttpResult MakeSysErr( DWORD err )
			{
				HttpResult res( HttpStatusSysError );
				res.SysError = err;
				return res;
			}
			static HttpResult MakeInternalErr( HttpInternalErrors er )
			{
				return er;
			}

			XStringA GetStrA() const
			{
				XStringA str;
				for ( int i = 0; i < Data.size(); i++ )
				{
					if ( Data[i] == 0 ) break;
					str += Data[i];
				}
				return str;
			}

			XStringW GetStrW() const
			{
				return GetStrA().ToWide();
			}

			bool Ok() const { return Status == HttpStatusOk; }

			XString DescribeError();

			HttpStatus Status;
			DWORD SysError;
			HttpInternalErrors InternalError;
			dvect<BYTE> Data;
		};

		/** HTTP mechanism. 
		This wraps the WinHttp library, and always operates synchronously.
		**/
		class PAPI Http
		{
		public:
			Http();
			
			/// Sets up, but does not connect
			Http( const XStringW& host_url, int explicitPort = 0 );

			~Http();

			enum SendMethod
			{
				SendGET,
				SendPOST
			};

			/// Open the connection
			HttpResult Connect( const XStringW& host = "", int explicitPort = 0 );

			/// Explicitly enable/disable SSL
			void EnableSSL( bool useSSL, bool setPortAuto = true );

			/// Control whether we pay heed to certificate CA checks. Turn this off to use self-signed certificates.
			void EnableCertificateChecks( bool enabled );

			/// Explicitly set the port
			void SetPort( int port );

			/** Use IE's proxy settings (from HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings:ProxyServer).
			
			We only turn the proxy on if the registry keys ProxyEnabled != 0 and ProxyServer != "".
			
			@return True if we found valid, enabled, proxy settings.
			**/
			bool UseIEProxySettings();

			/// Returns true if the user has a non-empty and enabled IE proxy config.
			bool ReadIEProxySettings( bool& enabled, XStringW& server );

			/// Returns the result of ReadIEProxySettings().
			bool UserHasIEProxySettings();

			/// Set the proxy explicitly, or disable a proxy by setting the proxy to an empty string.
			void SetProxy( XStringW proxy );

			/** Set timeouts.
			See WinHttpSetTimeouts.
			**/
			void SetTimeouts( int nameResolutionMS, int connectMS, int sendMS, int receiveMS );

			/// Close the connection
			void Close();

			/// Returns true if our HSession is not NULL.
			bool IsSessionOpen() { return HSession != NULL; }

			void SetUserAgent( const XStringA& ua ) { UserAgent = ua.ToWide(); }

			HttpResult Get( const XStringW& request );
			HttpResult GetA( const XStringA& request ) { return Get( XStringW::FromUtf8(request) ); }
			
			/** Post single item.
			**/
			HttpResult PostMultipartForm( XStringW request, XStringA name, XStringA filename, size_t bytes, const void* data );

			/** Post multiple items.
			**/
			HttpResult PostMultipartForm( XStringW request, const dvect<XStringA>& names, const dvect<XStringA>& filenames, const dvect<size_t>& sizes, const void** data );

			static HttpResult GetOnce( const XStringW& url, const char* userAgent = NULL );
			static HttpResult GetOnceA( const XStringA& url, const char* userAgent = NULL );
			static HttpResult PostMultipartFormOnce( XStringW url, XStringA name, XStringA filename, size_t bytes, const void* data );
			static HttpResult PostMultipartFormOnce( XStringW url, const dvect<XStringA>& names, const dvect<XStringA>& filenames, const dvect<size_t>& sizes, const void** data );

			template< typename TCH, typename TStr >
			static TStr TUrlEscape( const TStr& str )
			{
				TStr res;
				res.AllocLL( str.Length() );
				for ( int i = 0; i < str.Length(); i++ )
				{
					int ch = str[i];
					if (	(ch >= '0' && ch <= '9') ||
								(ch >= 'a' && ch <= 'z') ||
								(ch >= 'A' && ch <= 'Z') )
					{
						res += ch;
					}
					else
					{
						TCH buf[3] = {0,0,0};
						ByteToHex<TCH, true>( ch, buf );
						res += '%';
						res += buf[0];
						res += buf[1];
					}
				}
				return res;
			}

			static XStringA UrlEscape( const XStringA& str ) { return TUrlEscape<char, XStringA>( str ); }
			static XStringW UrlEscape( const XStringW& str ) { return TUrlEscape<wchar_t, XStringW>( str ); }

			/// Construct a query string (every key/value pair is escaped).
			static XStringA MakeQuery( const AStrAStrMap& params )
			{
				XStringA str;
				for ( AStrAStrMap::iterator it = params.begin(); it != params.end(); it++ )
				{
					str += UrlEscape(it->first) + "=" + UrlEscape(it->second) + "&";
				}
				str.Chop();
				return str;
			}

			/// Construct a query string (every key/value pair is escaped).
			static XStringW MakeQuery( const WStrWStrMap& params )
			{
				XStringW str;
				for ( WStrWStrMap::iterator it = params.begin(); it != params.end(); it++ )
				{
					str += UrlEscape(it->first) + L"=" + UrlEscape(it->second) + L"&";
				}
				str.Chop();
				return str;
			}

			static bool CrackUrl( const XStringW& url, bool& isSSL, XStringW& host, XStringW& request, int& port );
			static bool CrackUrlRequestOnly( const XStringW& url, XStringW& request );

		protected:

			static bool TransmitGET( HINTERNET req );
			static bool TransmitPOST( HINTERNET req, const XStringA& extraHead, AbCore::IFile* data );
			static bool ComposeMultipartForm( const dvect<XStringA>& names, const dvect<XStringA>& filenames, const dvect<size_t>& sizes, 
				const void** data, XStringA& extraHead, AbCore::IFile* composition );

			static XStringA GenBoundary();

			/// Send synchronously
			HttpResult Send( const XStringW& request, SendMethod method, const XStringA& extraHead, AbCore::IFile* data );

			HttpResult PrepareStatic( const XStringW& url, XStringW& request );

			void Construct();
			HttpResult Setup( const XStringW& host, int port );

			HINTERNET HSession, HConnection;
			XStringW Host;
			XStringW Proxy;
			XStringW UserAgent;
			bool IsSSL;
			bool CheckCA;
			int Port;

		};

	}
}
