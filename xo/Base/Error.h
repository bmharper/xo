#pragma once

namespace xo {

// See comment above Error for instructions on using StaticError
class XO_API StaticError {
public:
	const char* Msg;

	explicit StaticError(const char* msg);
};

/* Universal error type.

An empty error message is synonymous with success.

To return an OK status, return a default error object: Error().

This error type is designed to hold three different types of errors:
1. No error (operation succeeded)
2. Predefined error
3. Runtime-defined error

Types 1 and 2 are intended to be performant, in the sense that you can return
an Error from a function, and be sure that performance will be similar to that
of returning an integer error code. The third type, a runtime error message,
needs to allocate space for the error message on the heap, so it inevitably
is slower. These types of errors tend to be the exception though, so it seems
to be the right trade-off to make.

To define a type 2 error (ie predefined error), do the following:

	Inside a header file:
		extern XO_YOURLIBNAME_API StaticError ErrMyCustomError;

	Inside the corresponding .cpp file:
		XO_YOURLIBNAME_API StaticError ErrMyCustomError = StaticError("Something well known went wrong");

	Replace XO_YOURLIBNAME_API with the appropriate macro for the shared library
	that is exposing this error. For example, in the 'pal' library, this is XO_PAL_API.

	If you only need a predefined error to be visible from a .cpp file, then you can just write:
		StaticError ErrMyCustomError("my error");

	If your error is not being exported from a shared library, then don't include the XO_YOURLIBNAME_API macro.

To use that predefined error, simply use it wherever you need an "Error" object.
*/
class XO_API Error {
public:
	// We use the bottom-most bit of "Msg" to indicate a pointer to a StaticError object.
	// This means StaticError objects, as well as our internally allocated strings, must be
	// 4 byte aligned.
	enum {
		StaticBit = 1, // 'Msg' is a *StaticError
	};

	Error() {}
	Error(const Error& e);
	Error(Error&& e);
	Error(const StaticError& s) {
		Msg = (reinterpret_cast<uintptr_t>(&s) | StaticBit);
	}
	explicit Error(const char* msg);
	explicit Error(const std::string& msg);
	~Error();

	template <typename... Args>
	static Error Fmt(const char* fs, const Args&... args) { return Error(tsf::fmt(fs, args...)); }

	bool        OK() const { return Msg == 0; }
	const char* Message() const { return MsgString(); }
	bool        operator==(const StaticError& e) const;
	bool        operator!=(const StaticError& e) const { return !(*this == e); }
	Error       operator=(const Error& e);

	// Disabling these, because I don't see how they are meaningful
	//bool        operator==(const Error& e) const;
	//bool        operator!=(const Error& e) const { return !(*this == e); }

private:
	// This is either a pointer to a heap-allocated char* string,
	// or a pointer to a StaticError object OR-ed with StaticBit.
	uintptr_t Msg = 0;

	void        Set(const char* msg);
	const char* MsgString() const;
	void        Free();

	bool IsStatic() const { return (Msg & StaticBit) != 0; }
};

} // namespace xo