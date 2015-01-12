#pragma once

// The default styles for tags are defined inside xoDoc::InitializeDefaultTagStyles()

// If you change this, remember to update xoTagNames
// I initially used a generator macro here, but that breaks IDE comprehension (VS 2013),
// and it's also convenient to have different strings for the dummy types.
enum xoTag {
	xoTagNULL = 0,
	xoTagBody,
	xoTagDiv,
	xoTagText,
	xoTagLab,
	xoTagSpan,
	xoTagCanvas,
	xoTagEND,
	xoTag_DummyRoot,	// Not something you can create in a document. Used internally for debugging.
	xoTag_DummyWord,	// Not something you can create in a document. Used internally for debugging.
};

extern const char* xoTagNames[xoTagEND];
