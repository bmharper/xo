#pragma once

#include "nuDefs.h"
#include "nuStyle.h"
#include "Render/nuRenderStack.h"
#include "Text/nuGlyphCache.h"
#include "Text/nuFontStore.h"

/* A box that has been laid out.
*/
class NUAPI nuLayoutEl
{
public:
	nuBox Box;
};

/* This performs box layout.

Inside the class we make some separation between text and non-text layout,
because we will probably end up splitting the text stuff out into a separate
class, due to the fact that it gets complex if you're doing it properly
(ie non-latin fonts, bidirectional, asian, etc).

Hidden things that would bite you if you tried to multithread this:
* We get kerning data from Freetype for each glyph pair, and I'm not sure
that is thread safe.

*/
class NUAPI nuLayout
{
public:

	void Layout( const nuDoc& doc, u32 docWidth, u32 docHeight, nuRenderDomNode& root, nuPool* pool );

protected:

	struct NodeState
	{
		// Immutable
		//nuBox	PositionedAncestor;
		nuBox	ParentContentBox;
		bool	ParentContentBoxHasWidth;
		bool	ParentContentBoxHasHeight;

		// Mutable
		nuPos	PosMaxX;			// Used to accumulate width when unspecified
		nuPos	PosMaxY;			// Used to accumulate height when unspecified
		nuPos	PosLineX;			// Starts equal to PosX.
		nuPos	PosLineY;			// Starts equal to PosY.
		nuPos	PosX, PosY;
	};

	struct Word
	{
		nuPos	Width;
		int32	Start;
		int32	End;
		int32	Length() const { return End - Start; }
	};

	struct TextRunState
	{
		const nuDomText*	Node;
		nuRenderDomText*	RNode;
		podvec<Word>		Words;
		int					GlyphCount;		// Number of non-empty glyphs
		bool				GlyphsNeeded;
	};

	const nuDoc*				Doc;
	u32							DocWidth, DocHeight;
	nuPool*						Pool;
	nuRenderStack				Stack;
	float						PtToPixel;
	nuFontTableImmutable		Fonts;
	fhashset<nuGlyphCacheKey>	GlyphsNeeded;

	TextRunState				TempText;

	nuPos	ComputeDimension( nuPos container, bool isContainerDefined, nuStyleCategories cat );
	nuPos	ComputeDimension( nuPos container, bool isContainerDefined, nuSize size );
	nuBox	ComputeBox( nuBox container, bool widthDefined, bool heightDefined, nuStyleCategories cat );
	nuBox	ComputeBox( nuBox container, bool widthDefined, bool heightDefined, nuStyleBox box );
	nuBox	ComputeSpecifiedPosition( const NodeState& s );
	void	ComputeRelativeOffset( const NodeState& s, nuBox& box );

	void	LayoutInternal( nuRenderDomNode& root );
	void	RenderGlyphsNeeded();
	void	RunNode( NodeState& s, const nuDomNode& node, nuRenderDomNode* rnode );
	void	RunText( NodeState& s, const nuDomText& node, nuRenderDomText* rnode );
	void	GenerateTextWords( NodeState& s, TextRunState& ts );
	void	GenerateTextOutput( NodeState& s, TextRunState& ts );
	void	NextLine( NodeState& s, nuPos textHeight );
	nuPoint	PositionBlock( NodeState& s, nuBox borderBox );
	void	OffsetRecursive( nuRenderDomNode* rnode, nuPoint offset );

	static bool				IsSpace( int ch );
	static bool				IsLinebreak( int ch );
	static nuGlyphCacheKey	MakeGlyphCacheKey( nuRenderDomText* rnode );

};




