#pragma once
#include "DocGroup.h"

#if XO_PLATFORM_LINUX_DESKTOP
namespace xo {

class XO_API DocGroupLinux : public DocGroup {
public:
	DocGroupLinux();
	~DocGroupLinux() override;

protected:
	void InternalTouchedByOtherThread() override;
};
} // namespace xo
#endif