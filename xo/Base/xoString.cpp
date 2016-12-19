#include "pch.h"
#include "Defs.h"
#include "xoString.h"

namespace xo {

size_t StringRaw::Length() const {
	return Z == nullptr ? 0 : strlen(Z);
}

void StringRaw::CloneFastInto(StringRaw& b, Pool* pool) const {
	auto len = Length();
	if (len != 0) {
		b.Z = (char*) pool->Alloc(len, false);
		memcpy(b.Z, Z, len + 1);
	} else {
		b.Z = nullptr;
	}
}

void StringRaw::Discard() {
	Z = nullptr;
}

void StringRaw::Alloc(size_t chars) {
	Z = (char*) malloc(chars);
	XO_ASSERT(Z);
}

void StringRaw::Free() {
	free(Z);
	Z = nullptr;
}

uint32_t StringRaw::GetHashCode() const {
	if (Z == nullptr)
		return 0;
	// sdbm.
	uint32_t hash = 0;
	for (size_t i = 0; Z[i] != 0; i++) {
		// hash(i) = hash(i - 1) * 65539 + str[i]
		hash = (uint32_t) Z[i] + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

size_t StringRaw::Index(const char* find) const {
	const char* p = strstr(Z, find);
	if (p == nullptr)
		return -1;
	return p - Z;
}

size_t StringRaw::RIndex(const char* find) const {
	size_t pos  = 0;
	size_t last = -1;
	while (true) {
		const char* p = strstr(Z + pos, find);
		if (p == nullptr)
			return last;
		last = p - Z;
		pos  = last + 1;
	}
	XO_DIE_MSG("supposed to be unreachable");
	return last;
}

bool StringRaw::EndsWith(const char* suffix) const {
	size_t  suffix_len = strlen(suffix);
	ssize_t j          = Length() - suffix_len;
	if (j < 0)
		return false;
	for (size_t i = 0; i < suffix_len; i++, j++) {
		if (Z[j] != suffix[i])
			return false;
	}
	return true;
}

bool StringRaw::operator==(const char* b) const {
	return *this == Temp(const_cast<char*>(b));
}

bool StringRaw::operator==(const StringRaw& b) const {
	if (Z == nullptr) {
		return b.Z == nullptr || b.Z[0] == 0;
	}
	if (b.Z == nullptr) {
		// First case here is already handled
		return /* Z == nullptr || */ Z[0] == 0;
	}
	return strcmp(Z, b.Z) == 0;
}

bool StringRaw::operator<(const StringRaw& b) const {
	if (Z == nullptr) {
		return b.Z != nullptr && b.Z[0] != 0;
	}
	if (b.Z == nullptr) {
		return false;
	}
	return strcmp(Z, b.Z) < 0;
}

StringRaw StringRaw::Temp(char* b) {
	StringRaw r;
	r.Z = b;
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

String::String() {
	Z = nullptr;
}

String::String(const String& b) {
	Z     = nullptr;
	*this = b;
}

String::String(const StringRaw& b) {
	Z     = nullptr;
	*this = b;
}

String::String(const char* z, size_t maxLength) {
	Z = nullptr;
	Set(z, maxLength);
}

String::~String() {
	Free();
}

void String::Resize(size_t newLength) {
	Z            = (char*) ReallocOrDie(Z, newLength + 1);
	Z[newLength] = 0;
}

void String::Set(const char* z, size_t maxLength) {
	size_t newLength = 0;
	if (z != nullptr)
		newLength = strlen(z);

	if (maxLength != -1)
		newLength = Min(newLength, maxLength);

	if (Length() == newLength) {
		memcpy(Z, z, newLength);
	} else {
		Free();
		if (newLength != 0) {
			// add a null terminator, always, but don't assume that source has a null terminator
			Alloc(newLength + 1);
			memcpy(Z, z, newLength);
			Z[newLength] = 0;
		}
	}
}

void String::ReplaceAll(const char* find, const char* replace) {
	size_t      findLen    = strlen(find);
	size_t      replaceLen = strlen(replace);
	std::string self       = Z;
	size_t      pos        = 0;
	while ((pos = self.find(find, pos)) != std::string::npos) {
		self.replace(pos, findLen, replace);
		pos += replaceLen;
	}
	*this = self.c_str();
}

cheapvec<String> String::Split(const char* splitter) const {
	cheapvec<String> v;
	size_t           splitLen = strlen(splitter);
	size_t           pos      = 0;
	size_t           len      = Length();
	while (true) {
		const char* next = strstr(Z + pos, splitter);
		if (next == nullptr) {
			v += SubStr(pos, len);
			break;
		} else {
			v += SubStr(pos, next - Z);
			pos = next - Z + splitLen;
		}
	}
	return v;
}

String String::SubStr(size_t start, size_t end) const {
	auto len = Length();
	start    = Clamp<size_t>(start, 0, len);
	end      = Clamp<size_t>(end, 0, len);
	String s;
	s.Set(Z + start, end - start);
	return s;
}

String& String::operator=(const String& b) {
	Set(b.Z);
	return *this;
}

String& String::operator=(const StringRaw& b) {
	Set(b.Z);
	return *this;
}

String& String::operator=(const char* b) {
	Set(b);
	return *this;
}

String& String::operator+=(const StringRaw& b) {
	size_t len1 = Length();
	size_t len2 = b.Length();
	char*  newZ = (char*) malloc(len1 + len2 + 1);
	XO_ASSERT(newZ);
	memcpy(newZ, Z, len1);
	memcpy(newZ + len1, b.Z, len2);
	newZ[len1 + len2] = 0;
	free(Z);
	Z = newZ;
	return *this;
}

String& String::operator+=(const char* b) {
	*this += TempString(b);
	return *this;
}

String String::Join(const cheapvec<String>& parts, const char* joiner) {
	String r;
	if (parts.size() != 0) {
		size_t jlen  = joiner == nullptr ? 0 : (size_t) strlen(joiner);
		size_t total = 0;
		for (size_t i = 0; i < parts.size(); i++)
			total += parts[i].Length() + jlen;
		r.Alloc(total + 1);

		size_t rpos = 0;
		for (size_t i = 0; i < parts.size(); i++) {
			const char* part = parts[i].Z;
			if (part != nullptr) {
				for (size_t j = 0; part[j]; j++, rpos++)
					r.Z[rpos] = part[j];
			}
			if (i != parts.size() - 1) {
				for (size_t j = 0; j < jlen; j++, rpos++)
					r.Z[rpos] = joiner[j];
			}
		}
		r.Z[rpos] = 0;
	}
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XO_API String operator+(const char* a, const StringRaw& b) {
	String r = a;
	r += b;
	return r;
}

XO_API String operator+(const StringRaw& a, const char* b) {
	String r = a;
	r += b;
	return r;
}

XO_API String operator+(const StringRaw& a, const StringRaw& b) {
	String r = a;
	r += b.Z;
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TempString::TempString(const char* z) {
	Z = const_cast<char*>(z);
}

TempString::~TempString() {
	Z = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This is from http://www.strudel.org.uk/itoa/
template <typename TINT, typename TCH>
TCH* ItoaT(TINT value, TCH* result, int base) {
	if (base < 2 || base > 36) {
		*result = '\0';
		return result;
	}

	TCH *ptr = result, *ptr1 = result, tmp_char;
	TINT tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--   = *ptr1;
		*ptr1++  = tmp_char;
	}
	return result;
}

void XO_API Itoa(int64_t value, char* buf, int base) {
	ItoaT<int64_t, char>(value, buf, base);
}
}
