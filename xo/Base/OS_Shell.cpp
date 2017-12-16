#include "pch.h"
#include "OS_Shell.h"

namespace xo {
namespace shell {

XO_API void LaunchURL(const std::string& url) {
#if XO_PLATFORM_WIN_DESKTOP
	ShellExecute(0, 0, towide(url).c_str(), 0, 0, SW_SHOW);
#elif XO_PLATFORM_LINUX_DESKTOP
	std::string escaped;
	for (auto cp : utfz::cp(url)) {
		escaped += '\\';
		utfz::encode(escaped, cp);
	}
	auto cmd = tsf::fmt("xdg-open \"%v\"", url);
	system(cmd.c_str());
#endif
}

} // namespace shell
} // namespace xo
