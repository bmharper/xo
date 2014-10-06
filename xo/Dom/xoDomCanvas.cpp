#include "pch.h"
#include "xoDomCanvas.h"
#include "../Canvas/xoCanvas2D.h"
#include "../Image/xoImage.h"
#include "../xoDoc.h"

xoDomCanvas::xoDomCanvas( xoDoc* doc, xoInternalID parentID ) : xoDomNode( doc, xoTagCanvas, parentID )
{
}

xoDomCanvas::~xoDomCanvas()
{
	//Doc->Images.GetOrNull()
	//delete Canvas2D;
	if ( ImageName != "" )
		Doc->Images.Delete( ImageName.Z );
}

void xoDomCanvas::CloneSlowInto( xoDomEl& c, uint cloneFlags ) const
{
	xoDomNode::CloneSlowInto( c, cloneFlags );

	xoDomCanvas& cc = static_cast<xoDomCanvas&>(c);
	/*
	if ( Canvas2D != nullptr )
	{
		// ignore potential error (out of memory)
		Canvas2D->CloneInto( *cc.GetCanvas2D() );
	}
	*/
	cc.ImageName = ImageName;
}

bool xoDomCanvas::SetImageSizeOnly( uint width, uint height )
{
	if ( (width == 0 || height == 0) && ImageName == "" )
		return true;

	xoImage* img = Doc->Images.Get( ImageName.Z );
	if ( img == nullptr )
	{
		img = new xoImage();
		if ( !img->Alloc( xoTexFormatRGBA8, width, height ) )
		{
			delete img;
			return false;
		}
		ImageName = Doc->Images.SetAnonymous( img );
		return true;
	}
	else
	{
		return img->Alloc( xoTexFormatRGBA8, width, height );
	}
}

bool xoDomCanvas::SetSize( uint width, uint height )
{
	//xoCanvas2D* c2d = GetCanvas2D();
	//if ( c2d->Width() == width && c2d->Height() == height )
	//	return true;
	//
	//if ( !c2d->Resize( width, height ) )
	//	return false;

	if ( !SetImageSizeOnly( width, height ) )
		return false;

	//StyleParsef( "width: %dpx; height: %dpx", width, height ); -- so much cleaner, but sigh.. we need to build some sort of DSL or something.

	HackSetStyle( xoStyleAttrib::MakeWidth( xoSize::Pixels((float) width) ) );
	HackSetStyle( xoStyleAttrib::MakeHeight( xoSize::Pixels((float) height) ) );

	return true;
}

xoCanvas2D* xoDomCanvas::GetCanvas2D()
{
	return new xoCanvas2D( Doc->Images.Get( ImageName.Z ) );
}

const char* xoDomCanvas::GetCanvasImageName() const
{
	return ImageName.Z;
}
