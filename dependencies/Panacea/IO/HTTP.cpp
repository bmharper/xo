#include "pch.h"
#include "HTTP.h"
#include "Windows/Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Panacea 
{
namespace IO
{

XString HttpResult::DescribeError()
{
	switch ( Status )
	{
	case HttpStatusSysError:
		return L"System Error - " + IntToXString(SysError);
	case HttpStatusInternalError:
		switch ( InternalError )
		{
		case HttpInternalNone: return "Internal - None";
		case HttpInternalBadUrl: return "Internal - Bad Url";
		case HttpInternalInvalidSession: return "Internal - Invalid Session";
		case HttpInternalNoHost: return "Internal - No Host";
		case HttpInternalOpenRequestFailed: return "Internal - Open Request Failed";
		case HttpInternalSetOptionsFailed: return "Internal - SetOptions Failed";
		case HttpInternalSendRequestFailed: return "Internal - SendRequest Failed";
		case HttpInternalReceiveResponseFailed: return "Internal - ReceiveResponse Failed";
		default: ASSERT(false); return "Internal - Undefined!";
		}
	case HttpStatusOk: return "OK";
	default: ASSERT(false); return "Undefined!";
	}
}


Http::Http()
{
	Construct();
}

Http::Http( const XStringW& host_url, int port )
{
	Construct();
	Setup( host_url, port );
}

void Http::Construct()
{
	UserAgent = "Panacea.Http";
	HSession = NULL;
	HConnection = NULL;
	IsSSL = false;
	CheckCA = true;
	Port = PortHTTP;
}

Http::~Http()
{
	Close();
}

HttpResult Http::Setup( const XStringW& host, int explicitPort )
{
	XStringW chost, request;
	bool isSSL;
	int autoPort;
	if ( CrackUrl( host, isSSL, chost, request, autoPort ) )
	{
		Host = chost;
		// infer SSL from http:// or https://
		IsSSL = isSSL;
		Port = explicitPort != 0 ? explicitPort : autoPort;
		return HttpStatusOk;
	}
	else
	{
		// assume host is just a host name
		if ( host.Contains('/') ) return HttpInternalBadUrl;
		Host = host;
		// infer SSL from port
		IsSSL = explicitPort == PortHTTPS;
		return HttpStatusOk;
	}
}

void Http::Close()
{
	if ( HConnection ) WinHttpCloseHandle( HConnection );
	if ( HSession ) WinHttpCloseHandle( HSession );
	HConnection = NULL;
	HSession = NULL;
}

void Http::SetPort( int port )
{
	Port = port;
}

void Http::SetTimeouts( int nameResolutionMS, int connectMS, int sendMS, int receiveMS )
{
	if ( HSession != NULL )
	{
		WinHttpSetTimeouts( HSession, nameResolutionMS, connectMS, sendMS, receiveMS );
	}
}

void Http::EnableCertificateChecks( bool enabled )
{
	CheckCA = enabled;
}

void Http::EnableSSL( bool useSSL, bool setPortAuto )
{
	IsSSL = useSSL;
	if ( setPortAuto ) Port = IsSSL ? PortHTTPS : PortHTTP;
}

bool Http::ReadIEProxySettings( bool& enabled, XStringW& server )
{
	XStringW key = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
	enabled = Win::Registry::ReadDword( HKEY_CURRENT_USER, key, "ProxyEnable" ) != 0;
	server = Win::Registry::ReadStr( HKEY_CURRENT_USER, key, "ProxyServer" );
	return enabled && server != "";
}

bool Http::UseIEProxySettings()
{
	XStringW server;
	bool enabled;
	if ( ReadIEProxySettings( enabled, server ) )
	{
		Proxy = server;
		return true;
	}
	return false;
}

bool Http::UserHasIEProxySettings()
{
	XStringW server;
	bool enabled;
	return ReadIEProxySettings( enabled, server );
}

void Http::SetProxy( XStringW proxy )
{
	Proxy = proxy;
}

HttpResult Http::Connect( const XStringW& host, int port )
{
	Close();

	if ( host != "" )
	{
		HttpResult res = Setup( host, port );
		if ( !res.Ok() ) return res;
	}

	if ( Host == "" )
	{
		return HttpInternalNoHost;
	}

	HSession = WinHttpOpen( UserAgent, 
		Proxy.IsEmpty() ? WINHTTP_ACCESS_TYPE_DEFAULT_PROXY : WINHTTP_ACCESS_TYPE_NAMED_PROXY,
		Proxy.IsEmpty() ? WINHTTP_NO_PROXY_NAME : (LPCWSTR) Proxy,
		WINHTTP_NO_PROXY_BYPASS, 0 );

	if ( HSession == NULL ) return HttpResult::MakeSysErr( GetLastError() );

	HConnection = WinHttpConnect( HSession, Host, Port, 0 );

	if ( HConnection == NULL )
	{
		WinHttpCloseHandle( HSession );
		HSession = NULL;
		return HttpResult::MakeSysErr( GetLastError() );
	}

	return HttpStatusOk;
}

HttpResult Http::PostMultipartFormOnce( XStringW url, XStringA name, XStringA filename, size_t bytes, const void* data )
{
	Http net;
	XStringW request;
	HttpResult res = net.PrepareStatic( url, request );
	if ( !res.Ok() ) return res;

	return net.PostMultipartForm( request, name, filename, bytes, data );
}

HttpResult Http::PostMultipartFormOnce( XStringW url, const dvect<XStringA>& names, const dvect<XStringA>& filenames, const dvect<size_t>& sizes, const void** data )
{
	Http net;
	XStringW request;
	HttpResult res = net.PrepareStatic( url, request );
	if ( !res.Ok() ) return res;

	return net.PostMultipartForm( request, names, filenames, sizes, data );
}

HttpResult Http::GetOnceA( const XStringA& url, const char* userAgent )
{
	return GetOnce( XStringW::FromUtf8(url), userAgent );
}

HttpResult Http::GetOnce( const XStringW& url, const char* userAgent )
{
	Http net;
	XStringW request;
	if ( userAgent ) net.SetUserAgent( userAgent );
	HttpResult res = net.PrepareStatic( url, request );
	if ( !res.Ok() ) return res;

	return net.Send( request, SendGET, "", NULL );
}

HttpResult Http::Get( const XStringW& request )
{
	return Send( request, SendGET, "", NULL );
}

HttpResult Http::PostMultipartForm( XStringW request, const dvect<XStringA>& names, const dvect<XStringA>& filenames, const dvect<size_t>& sizes, const void** data )
{
	XStringA extraHead;
	AbCore::MemFile postBuf;
	ComposeMultipartForm( names, filenames, sizes, data, extraHead, &postBuf );
	return Send( request, SendPOST, extraHead, &postBuf );
}

HttpResult Http::PostMultipartForm( XStringW request, XStringA name, XStringA filename, size_t bytes, const void* data )
{
	XStringA extraHead;
	AbCore::MemFile postBuf;
	dvect<XStringA> names;
	dvect<XStringA> filenames;
	dvect<size_t> sizes;
	const void* datas[1];
	names += name;
	filenames += filename;
	sizes += bytes;
	datas[0] = data;
	ComposeMultipartForm( names, filenames, sizes, datas, extraHead, &postBuf );
	return Send( request, SendPOST, extraHead, &postBuf );
}

HttpResult Http::PrepareStatic( const XStringW& url, XStringW& request )
{
	if ( !CrackUrlRequestOnly( url, request ) ) return HttpInternalBadUrl;
	return Connect(url);
}

HttpResult Http::Send( const XStringW& request, SendMethod method, const XStringA& extraHead, AbCore::IFile* data )
{
	if ( HConnection == NULL ) return HttpInternalInvalidSession;

	HINTERNET req = NULL;

	bool sysErr = true;

	DWORD flags = IsSSL ? WINHTTP_FLAG_SECURE : 0;
	flags |= WINHTTP_FLAG_ESCAPE_DISABLE_QUERY;
	XStringW meth = method == SendGET ? "GET" : "POST";
	req = WinHttpOpenRequest( HConnection, meth, request, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags );

	if ( req == NULL )
	{
		return HttpInternalOpenRequestFailed;
	}

	HttpResult res = HttpStatusOk;

	bool optionsOk = true;
	if ( !CheckCA )
	{
		DWORD secFlags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
		if ( WinHttpSetOption( req, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags) ) == FALSE )
		{
			res = HttpInternalSetOptionsFailed;
			optionsOk = false;
		}
	}

	bool sendOk = false;
	if ( optionsOk )
		sendOk = method == SendGET ? TransmitGET( req ) : TransmitPOST( req, extraHead, data );

	if ( sendOk )
	{
		BOOL receiveOk = WinHttpReceiveResponse( req, 0 );
		if ( receiveOk )
		{
			while ( true )
			{
				DWORD bytes = 0;
				if ( WinHttpQueryDataAvailable( req, &bytes ) )
				{
					sysErr = false;
					if ( bytes > 0 )
					{
						size_t oldSize = res.Data.size();
						size_t total = oldSize + bytes;
						DWORD bytesRead = 0;
						if ( res.Data.capacity < (int) total )
							res.Data.reserve( (int) total );
						if ( WinHttpReadData( req, res.Data.data + oldSize, bytes, &bytesRead ) == FALSE )
						{
							sysErr = true;
							break;
						}
						res.Data.count += bytesRead;
					}
					else break;
				}
				else break;
			} // while(true)
		} // receiveOk
		else
		{
			res = HttpInternalReceiveResponseFailed;
		}
	} 
	else // sendOk
	{
		res = HttpInternalSendRequestFailed;
	}

	if ( sysErr ) res = HttpResult::MakeSysErr( GetLastError() );

	WinHttpCloseHandle( req );

	return res;
}

bool Http::TransmitGET( HINTERNET req )
{
	return FALSE != WinHttpSendRequest( req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 );
}

bool Http::TransmitPOST( HINTERNET req, const XStringA& extraHead, AbCore::IFile* data )
{
	AbCore::MemFile* mf = dynamic_cast< AbCore::MemFile* > ( data );
	void *dataAll = NULL;
	size_t dataSize = 0;
	if ( mf )
	{
		dataAll = mf->Data;
		dataSize = mf->Length();
	}
	else if ( data )
	{
		dataSize = data->Length();
		dataAll = malloc( dataSize );
		data->Seek( 0 );
		data->Read( dataAll, dataSize );
	}

	return FALSE != WinHttpSendRequest( req, 
																			extraHead.ToWide(), extraHead.Length(),
																			dataAll, (DWORD) dataSize, 
																			(DWORD) dataSize, 0 );
}

/*
This is a dump received from firefox 2. At the end of every boundary line is a \r\n pair.

-----------------------------227111267920526
Content-Disposition: form-data; name="textfield"

stuff
-----------------------------227111267920526
Content-Disposition: form-data; name="file"; filename="f1"
Content-Type: application/octet-stream

-mclaren
-----------------------------227111267920526
Content-Disposition: form-data; name="file2"; filename="f2"
Content-Type: application/octet-stream

-renault-
alonso
-----------------------------227111267920526
Content-Disposition: form-data; name="Submit"

Submit
-----------------------------227111267920526--
*/
bool Http::ComposeMultipartForm( const dvect<XStringA>& names, const dvect<XStringA>& filenames, const dvect<size_t>& sizes, 
																	const void** data, XStringA& extraHead, AbCore::IFile* composition )
{
	XStringA boundary = GenBoundary();
	XStringA __boundary		= "--" + boundary;
	XStringA __boundary__	= "--" + boundary + "--";
	extraHead = "Content-Type: multipart/form-data; boundary=" + boundary;

	// PHP ignores parts WITH Content-Type: application/octet-stream and WITHOUT filename=
	for ( int i = 0; i < names.size(); i++ )
	{
		bool isfile = filenames.size() > i && filenames[i] != "";
		XStringA bodyHead = 
			__boundary + "\r\n"
			"Content-Disposition: form-data; name=\"**formItemName**\"";
		if ( isfile )
			bodyHead += "; filename=\"**formFileName**\"\r\nContent-Type: application/octet-stream\r\n";
		else
			bodyHead += "\r\n";
		bodyHead += "\r\n";
		bodyHead.Replace( "**formItemName**", names[i] );
		if ( isfile ) bodyHead.Replace( "**formFileName**", filenames[i] );

		composition->Write( (LPCSTR) bodyHead, bodyHead.Length() );
		composition->Write( data[i], sizes[i] );
		composition->Write( "\r\n", 2 );
	}

	//XStringA bodyTail = "\r\n" + __boundary__ + "\r\n"; 
	XStringA bodyTail = __boundary__ + "\r\n"; 

	composition->Write( (LPCSTR) bodyTail, bodyTail.Length() );

	return true;
}


//Dump from from CURL
//POST /Telemetry/send-crash-dump.php?host=right_here_right_now HTTP/1.1
//User-Agent: curl/7.18.0 (i386-pc-win32) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3
//Host: localhost:12345
//Accept: */*
//Content-Length: 382
//Expect: 100-continue
//Content-Type: multipart/form-data; boundary=----------------------------f202a5f29056
//
//------------------------------f202a5f29056
//Content-Disposition: form-data; name="upload"; filename="albion.log"
//Content-Type: application/octet-stream
//
//GUI_MSG_BOX: There is nothing selected.
//GUI_MSG_BOX: There is nothing selected.
//
//------------------------------f202a5f29056
//Content-Disposition: form-data; name="press"
//
//OK
//------------------------------f202a5f29056--


/* This was the original version that worked ok.
bool Http::ComposeMultipartForm( const XStringA& name, size_t bytes, const void* data, 
																XStringA& extraHead, dvect<BYTE>& sendbuf )
{
	XStringA boundary = GenBoundary();
	XStringA __boundary = "--" + boundary;
	XStringA __boundary__ = "--" + boundary + "--";
	extraHead = "Content-Type: multipart/form-data; boundary=" + boundary; 
	XStringA bodyHead = 
		__boundary + "\r\n"
		"Content-Disposition: form-data; name=\"**formFileName**\";\r\n"
		"Content-Type: application/octet-stream\r\n"
		"\r\n";
	bodyHead.Replace( "**formFileName**", name );

	XStringA bodyTail = "\r\n" + __boundary__ + "\r\n"; 

	size_t total = bodyHead.Length() + bytes + bodyTail.Length();

	sendbuf.resize( (int) total );
	BYTE* pos = sendbuf.data;
	memcpy( pos, (LPCSTR) bodyHead, bodyHead.Length() );		pos += bodyHead.Length();
	memcpy( pos, data, bytes );									pos += bytes;
	memcpy( pos, (LPCSTR) bodyTail, bodyTail.Length() );		pos += bodyTail.Length();

	return true;
}
*/

bool Http::CrackUrlRequestOnly( const XStringW& url, XStringW& request )
{
	bool isSSL;
	XStringW host;
	int port;
	return CrackUrl( url, isSSL, host, request, port );
}

bool Http::CrackUrl( const XStringW& url, bool& isSSL, XStringW& host, XStringW& request, int& port )
{
	if ( url.Length() < 8 ) return false;

	isSSL = _wcsnicmp( url, L"https://", 8 ) == 0;
	bool plain = _wcsnicmp( url, L"http://", 7 ) == 0;
	if ( !isSSL && !plain ) return false;
	
	int hostStart = plain ? 7 : 8;
	int firstSlash = url.Find( '/', hostStart );
	if ( firstSlash < 0 )
	{
		host = url.Mid( hostStart );
		request = L"/";
	}
	else
	{
		host = url.Mid( hostStart, firstSlash - hostStart );
		request = url.Mid( firstSlash );
	}
	int colon = host.Find(':');
	if ( colon >= 0 )
	{
		port = ParseInt( host.Mid(colon + 1) );
		host = host.Left(colon);
	}
	else
	{
		port = isSSL ? PortHTTPS : PortHTTP;
	}
	return host.Length() > 0;
}

XStringA Http::GenBoundary()
{
	XStringA b;
	char range[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	// sizeof(char*) includes null terminator.
	int rangeL = (sizeof(range) / sizeof(char)) - 1;
	double time = AbcRTC();
	int* vt = reinterpret_cast<int*> ( &time );
	Random rnd( *vt );
	for ( int i = 0; i < 30; i++ )
	{
		int n = rnd.Int( 0, rangeL - 1 ); 
		b += range[n];
	}
	return b;
}

}
}
