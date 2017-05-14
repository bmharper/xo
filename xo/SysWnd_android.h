#pragma once
#include "SysWnd.h"

namespace xo {

class XO_API SysWndAndroid : public SysWnd {
public:
	Box RelativeClientRect; // Set by XoLib_init

	SysWndAndroid();
	~SysWndAndroid() override;

	Error Create(uint32_t createFlags) override;
	Box   GetRelativeClientRect() override;
};
} // namespace xo
