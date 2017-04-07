#pragma once
namespace xo {


/* Built-in tags.

At present there is no way to define custom tags, but I think there should be, provided
they are namespaced, for example <xo.button>Hello</xo.button>. That might be a convenient
way to create custom objects, instead of having to call a C++ function.

If you add a new tag here, remember to update the following things:
* TagNames
* Doc::InitializeDefaultTagStyles()
* RenderStack::Initialize()

The default styles for tags are defined inside Doc::InitializeDefaultTagStyles()

I initially used a generator macro here, but that breaks IDE comprehension (VS 2013),
and it's also convenient to have different strings for the dummy types.
*/
enum Tag {
	TagNULL = 0,
	TagBody,
	TagDiv,
	TagText,
	TagLab,
	TagSpan,
	TagCanvas,
	TagImg, // Not used, nor implemented.
	TagEND,
	Tag_DummyRoot, // Not something you can create in a document. Used internally for debugging.
	Tag_DummyWord, // Not something you can create in a document. Used internally for debugging.
};

extern const char* TagNames[TagEND];
}
