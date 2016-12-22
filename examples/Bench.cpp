#include "../xo/xo.h"

void xoMain(xo::SysWnd* wnd)
{
	xo::Doc* doc = wnd->Doc();

	int nx = 100;
	int ny = 100;
	for (int y = 0; y < ny; y++)
	{
		auto line = doc->Root.AddNode(xo::TagDiv);
		line->StyleParse("width: 100%; height: 4px; margin: 1px; display: block;");
		for (int x = 0; x < nx; x++)
		{
			auto div = line->AddNode(xo::TagDiv);
			div->StyleParse("width: 1%; height: 100%; border-radius: 2px; margin: 1px; display: inline;");
			div->StyleParsef("background: #%02x%02x%02x%02x", (uint8_t)(x * 5), 0, 0, 255);
			char txt[2] = "a";
			txt[0] = 'a' + x % 26;
			div->AddText(txt);
		}
	}
}
