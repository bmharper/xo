#pragma once

#include "xoDefs.h"
#include "xoStyle.h"
#include "Render/xoRenderStack.h"
#include "Text/xoGlyphCache.h"
#include "Text/xoFontStore.h"

/* A box that has been laid out.
*/
class XOAPI xoLayoutEl
{
public:
	xoBox Box;
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
class XOAPI xoLayout
{
public:

	void Layout( const xoDoc& doc, u32 docWidth, u32 docHeight, xoRenderDomNode& root, xoPool* pool );

protected:

	struct NodeState
	{
		// Immutable
		xoBox	ParentContentBox;
		bool	ParentContentBoxHasWidth;
		bool	ParentContentBoxHasHeight;

		// Mutable
		xoPos	PosMaxX;			// Right-most edge of the current line
		xoPos	PosMaxY;			// Bottom of the current line
		xoPos	PosX;
		xoPos	PosY;
		xoPos	PosBaselineY;		// First text element sets this, and it is thereafter fixed until the next line
	};

	struct Word
	{
		xoPos	Width;
		int32	Start;
		int32	End;
		int32	Length() const { return End - Start; }
	};

	struct TextRunState
	{
		const xoDomText*	Node;
		xoRenderDomText*	RNode;
		podvec<Word>		Words;
		int					GlyphCount;		// Number of non-empty glyphs
		bool				GlyphsNeeded;
	};

	const xoDoc*				Doc;
	u32							DocWidth, DocHeight;
	xoPool*						Pool;
	xoRenderStack				Stack;
	float						PtToPixel;
	float						EpToPixel;
	xoFontTableImmutable		Fonts;
	fhashset<xoGlyphCacheKey>	GlyphsNeeded;

	TextRunState				TempText;

	xoPos	ComputeDimension( xoPos container, bool isContainerDefined, xoStyleCategories cat );
	xoPos	ComputeDimension( xoPos container, bool isContainerDefined, xoSize size );
	xoBox	ComputeBox( xoBox container, bool widthDefined, bool heightDefined, xoStyleCategories cat );
	xoBox	ComputeBox( xoBox container, bool widthDefined, bool heightDefined, xoStyleBox box );
	xoBox	ComputeSpecifiedPosition( const NodeState& s );
	void	ComputeRelativeOffset( const NodeState& s, xoBox& box );

	void	LayoutInternal( xoRenderDomNode& root );
	void	RenderGlyphsNeeded();
	void	RunNode( NodeState& s, const xoDomNode& node, xoRenderDomNode* rnode );
	void	RunText( NodeState& s, const xoDomText& node, xoRenderDomText* rnode );
	void	GenerateTextWords( NodeState& s, TextRunState& ts );
	void	GenerateTextOutput( NodeState& s, TextRunState& ts );
	void	NextLine( NodeState& s );
	xoPoint	PositionBlock( NodeState& s, xoBox& marginBox );
	void	OffsetRecursive( xoRenderDomNode* rnode, xoPoint offset );

	static bool				IsSpace( int ch );
	static bool				IsLinebreak( int ch );
	static xoGlyphCacheKey	MakeGlyphCacheKey( xoRenderDomText* rnode );

};




