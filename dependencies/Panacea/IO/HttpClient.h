#pragma once

struct PAPI AbcHttpHeaderItem
{
	XStringA	Key;
	XStringA	Value;

	AbcHttpHeaderItem() {}
	AbcHttpHeaderItem( const XStringA& key, const XStringA& value ) : Key(key), Value(value) {}
};

struct PAPI AbcHttpCookie
{
	XStringA	Name;
	XStringA	Value;
	XStringA	Path;
	AbcDate		Expires;

	static void ParseCookiesFromBrowser( const char* s, size_t _len, podvec<AbcHttpCookie>* cookies );
	static bool ParseCookieFromServer( const char* s, size_t _len, AbcHttpCookie& cookie );
};

class PAPI AbcHttpRequest
{
public:
	podvec<AbcHttpHeaderItem>	Headers;
	XStringA					Method;				// default is GET
	XStringA					Url;
	XStringA					Body;
	bool						Enable100Continue;	// Enable the use of "HTTP 100: Continue" for PUT and POST requests (default = false). Setting this to true is not yet supported.

	AbcHttpRequest();
	~AbcHttpRequest();

	void						AddCookie( const XStringA& name, const XStringA& value );
	void						AddCookie( const AbcHttpCookie& cookie );
	AbcHttpHeaderItem*			HeaderByName( const XStringA& name, bool createIfNotExist );
	const AbcHttpHeaderItem*	HeaderByName( const XStringA& name ) const;
	void						RemoveHeader( const XStringA& name );
};

class PAPI AbcHttpResponse
{
public:
	podvec<AbcHttpHeaderItem>	Headers;
	XStringA					Body;

	AbcHttpResponse();
	~AbcHttpResponse();

	XStringA	HeaderValue( XStringA key ); 
	XStringA	StatusLine();		// Returns, for example, "HTTP/1.1 401 Unauthorized"
	XStringA	StatusCode();		// Returns, for example, "401 Unauthorized"
	int			StatusCodeInt();	// Returns, for example, the integer 401
	bool		FirstSetCookie( AbcHttpCookie& cookie );
};

// Static HTTP client functions
// The API here is purposefully identical to that of AbcHttpConnection
class PAPI AbcHttpClient
{
public:
	friend class AbcHttpConnection;

	static AbcHttpResponse	Get( const XStringA& url );
	static AbcHttpResponse	Post( const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body );
	static AbcHttpResponse	Perform( const XStringA& method, const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body );
	static void				Perform( const AbcHttpRequest& request, AbcHttpResponse& response );

private:
	static size_t CurlMyRead( void *ptr, size_t size, size_t nmemb, void *data );
	static size_t CurlMyWrite( void *ptr, size_t size, size_t nmemb, void *data );
	static size_t CurlMyHeaders( void *ptr, size_t size, size_t nmemb, void *data );
};

// Stateful HTTP connection.
// Use this when you want to reuse a TCP socket for multiple HTTP requests.
// Performing any action will keep the socket alive. If you want to explicitly close the socket,
// then you must call Close().
// The destructor calls Close().
// The API here is purposefully identical to that of AbcHttpClient
class PAPI AbcHttpConnection
{
public:
						AbcHttpConnection();
						~AbcHttpConnection();	// This calls Close()

	void				Close();
	AbcHttpResponse		Get( const XStringA& url );
	AbcHttpResponse		Post( const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body );
	AbcHttpResponse		Perform( const XStringA& method, const XStringA& url, const XStringA& contentType, size_t bodyBytes, const void* body );
	void				Perform( const AbcHttpRequest& request, AbcHttpResponse& response );

private:
	void*	CurlC;
};
