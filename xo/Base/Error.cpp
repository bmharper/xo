#include "pch.h"
#include "Asserts.h"
#include "Error.h"

namespace xo {

static XO_NORETURN void Die() {
	XO_DEBUG_ASSERT(false);
	*((int*) 0) = 1;
}

static const char* NoErrorMsg = "";

StaticError::StaticError(const char* msg) {
	Msg = msg;
}

Error::~Error() {
	Free();
}

Error::Error(const Error& e) {
	if (e.IsStatic() || e.Msg == 0)
		Msg = e.Msg;
	else
		Set(e.MsgString());
}

Error::Error(Error&& e) {
	std::swap(Msg, e.Msg);
}

Error::Error(const char* msg) {
	Set(msg);
}

Error::Error(const std::string& msg) : Error(msg.c_str()) {
}

// This doesn't seem to be meaningful
//bool Error::operator==(const Error& e) const
//{
//	if (IsStatic() && e.IsStatic())
//		return Msg == e.Msg;
//	return strcmp(MsgString(), e.MsgString()) == 0;
//}

bool Error::operator==(const StaticError& e) const {
	if (IsStatic())
		return (Msg & ~1) == reinterpret_cast<uintptr_t>(&e);
	return false;
}

Error Error::operator=(const Error& e) {
	Free();
	if (e.IsStatic() || e.Msg == 0)
		Msg = e.Msg;
	else
		Set(e.Message());

	return *this;
}

void Error::Set(const char* msg) {
	char* copy = (char*) malloc(strlen(msg) + 1);
	if (!copy)
		Die();

	strcpy(copy, msg);
	Msg = reinterpret_cast<uintptr_t>(copy);
}

const char* Error::MsgString() const {
	if (Msg == 0)
		return NoErrorMsg;

	if (IsStatic())
		return reinterpret_cast<StaticError*>(Msg & ~1)->Msg;
	else
		return reinterpret_cast<const char*>(Msg);
}

void Error::Free() {
	if (!IsStatic())
		free(reinterpret_cast<char*>(Msg));
}

} // namespace xo
