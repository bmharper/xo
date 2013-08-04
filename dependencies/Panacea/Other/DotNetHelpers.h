#pragma once
#ifndef PANACEA_DOTNET_HELPERS
#define PANACEA_DOTNET_HELPERS

/*

Property accessors
------------------

Examples:

	// Filename is the name of the exposed System::String property.
	// FileFilename is an XString member of native class C
	DNH_STRING_ACCESSOR( Filename, C->FileFilename );

*/

#define DNH_ACCESSOR_R_GET( proptype, propname, varname ) property proptype^ propname { proptype^ get() { return gcnew proptype(varname); } }
#define DNH_ACCESSOR_R( proptype, propname, varname )     property proptype^ propname { proptype^ get() { return gcnew proptype(varname); } \
	void set( proptype^ value ) { varname = value; } }

// Getter and a Setter function for a boolean state
#define DNH_ACCESSOR_TOGGLE( propname, obj, funcset, funcget ) property bool propname { bool get() { return obj->funcget(); } void set(bool v) { obj->funcset(v); } }

// Only a getter
#define DNH_GETTER_POD( proptype, propname, varname ) property proptype propname { \
	proptype get()				{ return varname; } }

// Only a setter
#define DNH_SETTER_POD( proptype, propname, varname ) property proptype propname { \
	void set( proptype value )	{ varname = value; } }

// Getter and setter
#define DNH_ACCESSOR_POD( proptype, propname, varname ) property proptype propname { \
	proptype get()				{ return varname; } \
	void set( proptype value )	{ varname = value; } }

// Getter and setter (static)
#define DNH_ACCESSOR_POD_STATIC( proptype, propname, varname ) property proptype propname { \
	static proptype get()				{ return varname; } \
	static void set( proptype value )	{ varname = value; } }

#define DNH_ACCESSOR_ENUM( proptype, native_enum_type, propname, native ) property proptype propname { \
	proptype get()				{ return (proptype) native; } \
	void set( proptype value )	{ native = (native_enum_type) value; } }

#define DNH_STRING_ACCESSOR( propname, varname ) DNH_ACCESSOR_R( String, propname, varname )
#define DNH_INT_ACCESSOR( propname, varname ) DNH_ACCESSOR_POD( int, propname, varname )
#define DNH_INT64_ACCESSOR( propname, varname ) DNH_ACCESSOR_POD( int64, propname, varname )
#define DNH_BOOL_ACCESSOR( propname, varname ) DNH_ACCESSOR_POD( bool, propname, varname )

#define DNH_INT_GETTER( propname, varname ) DNH_GETTER_POD( int, propname, varname )
#define DNH_INT64_GETTER( propname, varname ) DNH_GETTER_POD( int64, propname, varname )
#define DNH_BOOL_GETTER( propname, varname ) DNH_GETTER_POD( bool, propname, varname )

#define DNH_MAKE_WRAPPER_VALUE( self, native_type, native ) \
	internal: \
		native_type* native; \
		self( native_type* n ) { native = n; } \
		static self Wrap( native_type* n ) { return self(n); } \
	public: \
		static bool operator==( self a, self b ) { return a.native == b.native; } \
		static bool operator!=( self a, self b ) { return a.native != b.native; } \
		static property self Null { self get() { return self(NULL); } } \
		static self FromHandle( IntPtr h ) { return self( (native_type*) h.ToPointer() ); } \
		property IntPtr Handle { IntPtr get() { return IntPtr(native); } } \
		property bool IsNull { bool get() { return native == NULL; } } \
		void SetNull() { native = NULL; }

#define DNH_MAKE_WRAPPER_REF( self, native_type, native ) \
	internal: \
		native_type* native; \
		self( native_type* n ) { native = n; } \
		static self^ Wrap( native_type* n ) { return gcnew self(n); } \
	public: \
		static bool operator==( self^ a, self^ b ) { Object^ oa = a; Object^ ob = b; if (oa == ob) return true; if ((oa==nullptr) != (ob==nullptr)) return false; return a->native == b->native; } \
		static bool operator!=( self^ a, self^ b ) { return !(a == b); } \
		property IntPtr Handle { IntPtr get() { return IntPtr(native); } } \
		static self^ FromHandle( IntPtr h ) { return gcnew self( (native_type*) h.ToPointer() ); }


#endif
