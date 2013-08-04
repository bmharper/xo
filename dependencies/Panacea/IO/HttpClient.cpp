#include "pch.h"
#include "HttpClient.h"
#include "../Strings/fmt.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool AbcHttpCookie::ParseCookieFromServer( const char* s, size_t _len, AbcHttpCookie& cookie )
{
	// return fmt.Sprintf("%v=%v; Expires=%v", c.Name, c.Value, c.Expires.Format(http.TimeFormat))
	// Cookie=Dookie
	intp len = (intp) _len;
	intp keyStart = -1;
	intp valStart = -1;
	bool haveEquals = false;
	bool havePrimary = false;
	for ( intp i = 0; i <= len; i++ )
	{
		if ( i == len || s[i] == ';' )
		{
			if ( !haveEquals )
				return false;
			if ( keyStart == -1 || valStart == -1 )
				return false;
			intp keyLen = valStart - keyStart - 1;
			intp valLen = i - valStart;
			if ( len >= 2 && i == len && s[i - 1] == '\n' && s[i - 2] == '\r' )
				valLen -= 2;
			if ( keyLen <= 0 )
				return false;
			if ( !havePrimary )
			{
				if ( valLen >= 0 )
				{
					havePrimary = true;
					cookie.Name.SetExact( s + keyStart, keyLen );
					cookie.Value.SetExact( s + valStart, valLen );
				}
				else
					return false;
			}
			else
			{
				if ( strncmp( s + keyStart, "Expires", 7 ) == 0 )
				{
					if ( valLen != 29 )
						return false;
					cookie.Expires = AbcDate::ParseHttp( s + valStart );
				}
				else if ( strncmp( s + keyStart, "Path", 4 ) == 0 )
				{
					cookie.Path.SetExact( s + valStart, valLen );
				}
				else
				{
					// unrecognized field
				}
			}
			haveEquals = false;
			keyStart = -1;
			valStart = -1;
		}
		else if ( s[i] == '=' && !haveEquals )
		{
			haveEquals = true;
			valStart = i + 1;
		}
		else if ( s[i] != ' ' && keyStart == -1 )
		{
			keyStart = i;
		}
	}
	return havePrimary;
}

void AbcHttpCookie::ParseCookiesFromBrowser( const char* s, size_t _len, podvec<AbcHttpCookie>* cookies )
{
	// Cookie: CUSTOMER=WILE_E_COYOTE; PART_NUMBER=ROCKET_LAUNCHER_0001
	intp len = (intp) _len;
	intp start = 0;
	intp eq = 0;
	intp pos = 0;
	for ( ; true; pos++ )
	{
		if ( pos >= len || s[pos] == ';' )
		{
			intp name_len = eq - start;
			intp value_len = pos - eq - 1;
			if ( name_len <= 0 || value_len <= 0 || eq == 0 )
				return;

			AbcHttpCookie& c = cookies->add();
			c.Expires = AbcDate::NullDate();
			c.Name.SetExact( s + start, (int) name_len );
			c.Value.SetExact( s + eq + 1, (int) value_len );

			if ( pos >= len )
				return;
			
			eq = 0;
			start = pos + 2;
		}
		else if ( s[pos] == '=' && eq == 0 )
		{
			eq = pos;
		}
	}
	// unreachable
	AbcAssert(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcHttpRequest::AbcHttpRequest()
{
	Method = "GET";
	Enable100Continue = false;
}
AbcHttpRequest::~AbcHttpRequest() {}

void AbcHttpRequest::AddCookie( const XStringA& name, const XStringA& value )
{
	intp ihead = 0;
	for ( ; ihead < Headers.size(); ihead++ )
	{
		if ( Headers[ihead].Key == "Cookie" )
			break;
	}

	if ( ihead == Headers.size() )
		Headers += AbcHttpHeaderItem( "Cookie", fmt("%v=%v", name, value) );
	else
		Headers[ihead].Value += fmt("; %v=%v", name, value);
}

void AbcHttpRequest::AddCookie( const AbcHttpCookie& cookie )
{
	AddCookie( cookie.Name, cookie.Value );
}

AbcHttpHeaderItem* AbcHttpRequest::HeaderByName( const XStringA& name, bool createIfNotExist )
{
	const AbcHttpHeaderItem* item = HeaderByName( name );
	if ( !createIfNotExist || item != NULL )
		return const_cast<AbcHttpHeaderItem*>(item);

	Headers += AbcHttpHeaderItem( name, "" );
	return &Headers.back();
}

const AbcHttpHeaderItem* AbcHttpRequest::HeaderByName( const XStringA& name ) const
{
	for ( intp i = 0; i < Headers.size(); i++ )
	{
		if ( Headers[i].Key == name )
			return &Headers[i];
	}
	return NULL;
}

void AbcHttpRequest::RemoveHeader( const XStringA& name )
{
	for ( intp i = 0; i < Headers.size(); i++ )
	{
		if ( Headers[i].Key == name )
		{
			Headers.erase(i);
			return;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcHttpResponse::AbcHttpResponse() {}
AbcHttpResponse::~AbcHttpResponse() {}

XStringA AbcHttpResponse::StatusLine()
{
	return Headers.size() > 0 ? Headers[0].Value : "";
}

XStringA AbcHttpResponse::StatusCode()
{
	if ( Headers.size() == 0 )
		return "";
	// Headers[0] is something like
	// HTTP/1.1 200 OK
	int firstSpace = Headers[0].Value.Find(' ');
	if ( firstSpace == -1 )
		return "";
	else
		return Headers[0].Value.Mid( firstSpace + 1 );
}

int AbcHttpResponse::StatusCodeInt()
{
	return atoi( StatusCode() );
}

XStringA AbcHttpResponse::HeaderValue( XStringA key )
{
	for ( intp i = 0; i < Headers.size(); i++ )
	{
		if ( Headers[i].Key == key )
			return Headers[i].Value;
	}
	return fmt( "Header value not set for %v", key );
}

bool AbcHttpResponse::FirstSetCookie( AbcHttpCookie& cookie )
{
	for ( intp i = 0; i < Headers.size(); i++ )
	{
		if ( Headers[i].Key == "Set-Cookie" )
			return AbcHttpCookie::ParseCookieFromServer( Headers[i].Value, Headers[i].Value.Length(), cookie );
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t AbcHttpClient::CurlMyRead( void *ptr, size_t size, size_t nmemb, void *data )
{
	intp tot = size * nmemb;
	byte** rdata = (byte**) data;
	memcpy( ptr, *rdata, tot );
	*rdata += tot;
	return tot;
}

size_t AbcHttpClient::CurlMyWrite( void *ptr, size_t size, size_t nmemb, void *data )
{
	AbcHttpResponse* r = (AbcHttpResponse*) data;
	intp tot = size * nmemb;
	for ( intp i = 0; i < tot; i++ )
		r->Body += (char) ((byte*)ptr)[i];
	return tot;
}

size_t AbcHttpClient::CurlMyHeaders( void *ptr, size_t size, size_t nmemb, void *data )
{
	AbcHttpResponse* r = (AbcHttpResponse*) data;
	intp tot = size * nmemb;
	intp mytot = tot;
	intp valStart = 0;
	const char* line = (const char*) ptr;

	// discard the closing \r\n from the header line
	if ( mytot >= 2 && line[mytot - 2] == '\r' && line[mytot - 1] == '\n' )
		mytot -= 2;

	if ( mytot != 0 )
	{
		AbcHttpHeaderItem& item = r->Headers.add();
		for ( intp i = 0; i < mytot; i++ )
		{
			if ( valStart == 0 && line[i] == ':' )
			{
				valStart = i + 1;
				item.Key.SetExact( line, i );
				break;
			}
		}
	
		// skip whitespace at start of value (ie Content-Length: 123) - skip the space before the 123.
		for ( ; valStart < mytot && line[valStart] == ' '; valStart++ )
		{}

		item.Value.SetExact( line + valStart, mytot - valStart );
	}
	return tot;
}

AbcHttpResponse AbcHttpClient::Get( const XStringA& url )
{
	AbcHttpConnection c;
	return c.Get( url );
}

AbcHttpResponse AbcHttpClient::Post( const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body )
{
	AbcHttpConnection c;
	return c.Post( url, contentType, bodyBytes, body );
}

AbcHttpResponse AbcHttpClient::Perform( const XStringA& method, const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body )
{
	AbcHttpConnection c;
	return c.Perform( method, url, contentType, bodyBytes, body );
}

void AbcHttpClient::Perform( const AbcHttpRequest& request, AbcHttpResponse& response )
{
	AbcHttpConnection c;
	c.Perform( request, response );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AbcHttpConnection::AbcHttpConnection()
{
	CurlC = NULL;
}

AbcHttpConnection::~AbcHttpConnection()
{
	Close();
}

void AbcHttpConnection::Close()
{
	if ( CurlC != NULL )
	{
		curl_easy_cleanup( CurlC );
		CurlC = NULL;
	}
}

AbcHttpResponse AbcHttpConnection::Get( const XStringA& url )
{
	return Perform( "GET", url, "", 0, NULL );
}

AbcHttpResponse AbcHttpConnection::Post( const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body )
{
	return Perform( "POST", url, contentType, bodyBytes, body );
}

AbcHttpResponse AbcHttpConnection::Perform( const XStringA& method, const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body )
{
	AbcHttpRequest request;
	request.Method = method;
	request.Body.SetExact( (const char*) body, (int) bodyBytes );
	request.Url = url;
	if ( contentType != "" )
		request.Headers += AbcHttpHeaderItem( "Content-Type", contentType );
	
	AbcHttpResponse response;
	Perform( request, response );
	return response;
}

void AbcHttpConnection::Perform( const AbcHttpRequest& request, AbcHttpResponse& response )
{
	response = AbcHttpResponse();
	if (	request.Method != "GET" &&
			request.Method != "HEAD" &&
			request.Method != "POST" &&
			request.Method != "PUT" &&
			request.Method != "DELETE" &&	// untested
			request.Method != "TRACE" &&	// untested
			request.Method != "CONNECT" &&	// untested
			request.Method != "PATCH" &&	// untested
			request.Method != "OPTIONS"		// untested
		)
	{
		AbcPanic( "AbcHttpClient: Invalid HTTP verb " + request.Method );
	}

	CURLcode res = CURLE_COULDNT_CONNECT;
	if ( CurlC == NULL )
	{
		CurlC = curl_easy_init();
		if ( CurlC == NULL)
			AbcPanic( "curl_easy_init failed" );
	}
	// reset any state that we may have set on the last request
	curl_easy_setopt( CurlC, CURLOPT_CUSTOMREQUEST, NULL );
	curl_easy_setopt( CurlC, CURLOPT_HEADER, NULL );
	curl_easy_setopt( CurlC, CURLOPT_POSTFIELDSIZE, 0 );
	curl_easy_setopt( CurlC, CURLOPT_POSTFIELDS, NULL );
	curl_easy_setopt( CurlC, CURLOPT_UPLOAD, 0 );
	curl_easy_setopt( CurlC, CURLOPT_INFILESIZE, 0 );

	byte* readData = (byte*) request.Body.GetRawBufferConst();

	// initialize with this new request's parameters
	curl_easy_setopt( CurlC, CURLOPT_URL, (const char*) request.Url );
	curl_easy_setopt( CurlC, CURLOPT_READFUNCTION, AbcHttpClient::CurlMyRead );
	curl_easy_setopt( CurlC, CURLOPT_WRITEFUNCTION, AbcHttpClient::CurlMyWrite );
	curl_easy_setopt( CurlC, CURLOPT_HEADERFUNCTION, AbcHttpClient::CurlMyHeaders );
	curl_easy_setopt( CurlC, CURLOPT_READDATA, &readData );
	curl_easy_setopt( CurlC, CURLOPT_WRITEDATA, &response );
	curl_easy_setopt( CurlC, CURLOPT_HEADERDATA, &response );
	curl_slist* headers = NULL;
	if ( request.Method == "POST" )
	{
		curl_easy_setopt( CurlC, CURLOPT_POSTFIELDSIZE, request.Body.Length() );
		curl_easy_setopt( CurlC, CURLOPT_POSTFIELDS, (const void*) request.Body.GetRawBufferConst() );
		//curl_easy_setopt( CurlC, CURLOPT_POST, 1 );
	}
	else if ( request.Method == "PUT" )
	{
		curl_easy_setopt( CurlC, CURLOPT_UPLOAD, 1 );
		curl_easy_setopt( CurlC, CURLOPT_INFILESIZE_LARGE, (curl_off_t) request.Body.Length() );
	}
	for ( intp i = 0; i < request.Headers.size(); i++ )
		headers = curl_slist_append( headers, fmt( "%v: %v", request.Headers[i].Key, request.Headers[i].Value ) );
	
	if (	request.Method == "POST" ||
			request.Method == "PUT" )
	{
		if ( !request.Enable100Continue )
		{
			// Curl will inject an "Expect: 100-continue" header for POST and PUT data, because
			// this is part of the recommendation of HTTP 1.1: You first do a handshake, and possibly wait
			// for a 302: Redirect, before transmitting your POST or PUT data. I have never had a need for that,
			// and it doesn't seem very REST-ish.
			headers = curl_slist_append( headers, fmt( "%v: %v", "Expect", "" ) );
		}
		else
		{
			// I think you'll have to do two roundtrips here to support this concept
			AbcPanic( "This technique is not implemented" );
		}
	}
	
	// The following technique is untested
	if (	request.Method == "HEAD" ||
			request.Method == "DELETE" ||
			request.Method == "TRACE" ||
			request.Method == "CONNECT" ||
			request.Method == "PATCH" ||
			request.Method == "OPTIONS"
		)
	{
		curl_easy_setopt( CurlC, CURLOPT_CUSTOMREQUEST, request.Method );
	}

	if ( headers )
		curl_easy_setopt( CurlC, CURLOPT_HTTPHEADER, headers );

	// Fire off the request
	res = curl_easy_perform( CurlC );

	if ( headers )
		curl_easy_setopt( CurlC, CURLOPT_HTTPHEADER, NULL );
	curl_slist_free_all( headers );
}

