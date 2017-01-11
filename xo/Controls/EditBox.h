#pragma once

namespace xo {
class Doc;
class DomNode;
namespace controls {

// A single-line text edit box
class XO_API EditBox {
public:
	struct State {
		int      CaretPos       = 0; // UTF-8 code point position in our string
		bool     IsBlinkVisible = false;
		uint64_t TimerID        = 0;
	};

	static void     InitializeStyles(Doc* doc);
	static DomNode* AppendTo(DomNode* node);
};
}
}