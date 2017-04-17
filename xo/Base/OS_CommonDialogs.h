#pragma once

namespace xo {
class Doc;
namespace osdialogs {

struct BrowseFile {
	typedef std::pair<std::string, std::string> NameFilterPair; // Name, Filter. Filter is semicolon-separated list of wildcards, for example: *.jpeg;*.jpg

	const Doc*                  Parent = nullptr;
	bool                        IsOpen = true;
	std::string                 Filename;
	std::vector<NameFilterPair> FileTypes;
	size_t                      FileTypeIndex = -1; // Default file type, from FileTypes
};

XO_API bool BrowseForFile(BrowseFile& bf);
XO_API bool BrowseForFileOpen(const Doc* doc, const std::vector<std::pair<std::string, std::string>>& types, std::string& filename);
XO_API bool BrowseForFileSave(const Doc* doc, const std::vector<std::pair<std::string, std::string>>& types, std::string& filename);

} // namespace osdialogs
} // namespace xo
