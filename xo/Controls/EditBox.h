#pragma once

#include "../Defs.h"

namespace xo {
class Doc;
class DomNode;
namespace controls {

/* A single-line text edit box

The text value of the edit box is accessible using GetText and SetText of the DOM node returned by AppendTo().
*/
class XO_API EditBox {
public:
	struct State {
		InternalID EditBoxID              = 0;
		InternalID CaretID                = 0;
		int        CaretPos               = 0; // UTF-8 code point position in our string. Caret sits after this character.
		bool       IsBlinkVisible         = false;
		bool       PlaceCaretOnNextRender = false;
		uint64_t   TimerID                = 0;
	};

	static void     InitializeStyles(Doc* doc);
	static DomNode* AppendTo(DomNode* node, const char* txt = nullptr);

private:
	static void PlaceCaretIndicator(State* s, Event& ev);
};
}
}