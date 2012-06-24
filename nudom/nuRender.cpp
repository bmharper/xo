#include "pch.h"
#include "nuDoc.h"
#include "nuRender.h"
#include "nuRenderGL.h"

void nuRenderer::Render( nuRenderGL* gl, nuRenderDomEl* root, int width, int height )
{
	GL = gl;

	gl->PreRender( width, height );

	RenderNode( root );

	gl->PostRenderCleanup();
}

void nuRenderer::RenderNode( nuRenderDomEl* node )
{
	nuStyle& style = node->Style;
	float bottom = nuPosToReal( node->Pos.Bottom );
	float top = nuPosToReal( node->Pos.Top );
	float left = nuPosToReal( node->Pos.Left );
	float right = nuPosToReal( node->Pos.Right );
	nuVx_PTC corners[4];
	corners[0].Pos = NUVEC3(left, top, 0);
	corners[1].Pos = NUVEC3(left, bottom, 0);
	corners[2].Pos = NUVEC3(right, bottom, 0);
	corners[3].Pos = NUVEC3(right, top, 0);

	float radius = node->BorderRadius;

	float width = right - left;
	float height = bottom - top;
	float mindim = min(width, height);
	mindim = max(mindim, 0.0f);
	radius = min(radius, mindim / 2);

	//NUTRACE( "node %f\n", left );

	auto bg = style.Get( nuCatBackground );
	if ( bg && bg->Color.a != 0 )
	{
		for ( int i = 0; i < 4; i++ )
			corners[i].Color = bg->Color.GetRGBA();

		if ( radius != 0 )
		{
			GL->ActivateProgram( GL->PRect );
			glUniform4f( GL->VarRectBox, left, top, right, bottom );
			glUniform1f( GL->VarRectRadius, radius );
			GL->DrawQuad( corners );
		}
		else
		{
			GL->ActivateProgram( GL->PFill );
			GL->DrawQuad( corners );
		}
	}

	for ( intp i = 0; i < node->Children.size(); i++ )
	{
		RenderNode( node->Children[i] );
	}
}




		/*
		uint16 indices[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
		//uint16 indices[12 * 4];
		nuVx_PTC tri[12];

		GL->ActivateProgram( GL->PCurve );

		const float A = radius;
		const float B = SQRT_2 * 0.5 * A;
		const float C = A - B;
		const float M1 = 0.5 * (A + C);
		const float M2 = 0.5 * (0 + C);
		const float ME = -0.101 * A;
		for ( int icorner = 0; icorner < 1; icorner++ )
		{
			tri[0].UV = NUVEC2(1,1);
			tri[1].UV = NUVEC2(0.5,0);
			tri[2].UV = NUVEC2(0,0);
			tri[0].Pos = NUVEC3(A,0,0);
			tri[1].Pos = NUVEC3(M1 + ME, M2 + ME,0);
			tri[2].Pos = NUVEC3(C,C,0);

			tri[3].UV = NUVEC2(1,1);
			tri[4].UV = NUVEC2(0.5,0);
			tri[5].UV = NUVEC2(0,0);
			tri[3].Pos = NUVEC3(C,C,0);
			tri[4].Pos = NUVEC3(M2 + ME, M1 + ME,0);
			tri[5].Pos = NUVEC3(0,A,0);

			tri[6].UV = NUVEC2(1,1);
			tri[7].UV = NUVEC2(0.5,0.5);
			tri[8].UV = NUVEC2(0,0);
			tri[6].Pos = NUVEC3(A,0,0);
			tri[7].Pos = NUVEC3(C,C,0);
			tri[8].Pos = NUVEC3(A,A,0);

			tri[9].UV = NUVEC2(1,1);
			tri[10].UV = NUVEC2(0.5,0.5);
			tri[11].UV = NUVEC2(0,0);
			tri[9].Pos = NUVEC3(C,C,0);
			tri[10].Pos = NUVEC3(0,A,0);
			tri[11].Pos = NUVEC3(A,A,0);

			GL->DrawTriangles( 12, tri, indices );
		}
		*/
