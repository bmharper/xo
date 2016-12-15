#pragma once
namespace xo {

// The default styles for tags are defined inside Doc::InitializeDefaultTagStyles()

// If you change this, remember to update TagNames
// I initially used a generator macro here, but that breaks IDE comprehension (VS 2013),
// and it's also convenient to have different strings for the dummy types.
enum Tag {
	TagNULL = 0,
	TagBody,
	TagDiv,
	TagText,
	TagLab,
	TagSpan,
	TagCanvas,
	TagEND,
	Tag_DummyRoot, // Not something you can create in a document. Used internally for debugging.
	Tag_DummyWord, // Not something you can create in a document. Used internally for debugging.
};

extern const char* TagNames[TagEND];
}
