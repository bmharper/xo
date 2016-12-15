#pragma once

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4345) // POD constructed with () is default-initialized
#endif

namespace xo {

class Pool;

// This has no constructors or destructors so that we can put it in unions, etc. We know that there is no implicit
// memory management going on here.
class XO_API StringRaw {
public:
	char* Z;

	size_t   Length() const;
	void     CloneFastInto(StringRaw& b, Pool* pool) const;
	void     Discard();
	uint32_t GetHashCode() const;
	size_t   Index(const char* find) const;      // Find the first occurrence of 'find', or -1 if none
	size_t   RIndex(const char* find) const;     // Find the last occurrence of 'find', or -1 if none
	bool     EndsWith(const char* suffix) const; // Returns true if the string ends with 'suffix'

	bool operator==(const char* b) const;
	bool operator!=(const char* b) const { return !(*this == b); }
	bool operator==(const StringRaw& b) const;
	bool operator!=(const StringRaw& b) const { return !(*this == b); }

	bool operator<(const StringRaw& b) const;

protected:
	static StringRaw Temp(char* b);

	void Alloc(size_t chars);
	void Free();
};

// This is the classic thing you'd expect from a string. The destructor will free the memory.
class XO_API String : public StringRaw {
public:
	String();
	String(const String& b);
	String(const StringRaw& b);
	String(const char* z, size_t maxLength = -1); // Calls Set()
	~String();

	void             Set(const char* z, size_t maxLength = -1); // checks maxLength against strlen(z) and clamps automatically
	void             Resize(size_t newLength);
	void             ReplaceAll(const char* find, const char* replace);
	cheapvec<String> Split(const char* splitter) const;
	String           SubStr(size_t start, size_t end) const; // Returns [start .. end - 1]

	String& operator=(const String& b);
	String& operator=(const StringRaw& b);
	String& operator=(const char* b);
	String& operator+=(const StringRaw& b);
	String& operator+=(const char* b);

	static String Join(const cheapvec<String>& parts, const char* joiner);
};

XO_API String operator+(const char* a, const StringRaw& b);
XO_API String operator+(const StringRaw& a, const char* b);
XO_API String operator+(const StringRaw& a, const StringRaw& b);

// Use this when you need a temporary 'String' object, but you don't need any heap allocs or frees
class XO_API TempString : public String {
public:
	TempString(const char* z);
	~TempString();
};

void XO_API Itoa(int64_t value, char* buf, int base);
}

namespace ohash {
inline ohash::hashkey_t gethashcode(const xo::String& k) { return (ohash::hashkey_t) k.GetHashCode(); }
}

#ifdef _WIN32
#pragma warning(pop)
#endif
