#include "../xo/xo.h"

/*
This sample was created when developing the layout concepts
*/

void InitDOM(xo::Doc* doc);

void xoMain(xo::SysWnd* wnd) {
	wnd->OnWindowClose.push_back([]() {
		// This just demonstrates how to clean up global resources when an application exits
		xo::Trace("KitchenSink application is closing\n");
	});
	xo::Global()->FontStore->AddFontDirectory("C:\\temp\\fonts");
	int left   = -1400;
	int width  = 1300;
	int top    = 60;
	int height = 700;
	wnd->SetPosition(xo::Box(left, top, left + width, top + height), xo::SysWnd::SetPosition_Move | xo::SysWnd::SetPosition_Size);
	InitDOM(wnd->Doc());
}

void DoBorder(xo::Doc* doc) {
	auto root = &doc->Root;
	root->StyleParse("background: #fff");
	root->Parse(
		"<div style='border: 1px #bfb; background: #dfd; border-radius: 5px; width: 50px; height: 50px; margin: 2px'></div>"
	    "<div style='border: #007e; border: 2px 20px 2px 20px; border-radius: 0px 0px 0px 40px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'></div>"
	    "<div style='border: #007e; border: 20px 2px 20px 2px; border-radius: 5px 5px 5px 5px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'></div>"
	    "<div style='border: #007e; border: 2px 20px 2px 20px; border-radius: 5px 5px 5px 5px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'></div>"
	    "<div style='border: #007e; border: 2px 20px 5px 15px; border-radius: 100px 30px 20px 10px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'></div>"
	    "<div style='border: #007e; border: 1px 2px 3px 4px; border-radius: 5px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'></div>"
	    "<div style='border: #070e; border: 1px 1px 2px 3px; border-radius: 0px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'>aaaaa</div>"
	    "<div style='border: #070e; border: 1px 1px 2px 3px; border-radius: 2px; width: 200ep; height: 200ep; background: #fffe; margin: 2px'>aaaaa</div>"
	    "<div style='border: 5px #070e; border-radius: 8px; width: 100ep; height: 100ep; background: #fffe; margin: 1px'>b</div>"
	    "<div style='border: 1px #557e; width: 150ep; height: 22ep; background: #fffe; margin: 1px'>c</div>"
	    "<div style='border: 5ep #456e; width: 40ep; height: 40ep; background: #567e; margin: 1px'>d</div>" // ensure border color goes through sRGB conversion
	    "<div style='border: #007e; border: 10px 30px 10px 30px; border-radius: 20px 20px 20px 20px; width: 100ep; height: 100ep; background: #fffe; margin: 10px'></div>");
}

void DoBaselineAlignment(xo::Doc* doc) {
	// Text DOM elements cannot define styles on themselves - they MUST inherit all of their styling from
	// their parents. Therefore, if you want different font styles on the same line, then you must wrap
	// those different styles in DOM Nodes. This example demonstrates that the "baseline" position is
	// preserved when entering those DOM Nodes. If this were not true, then the baselines of the different
	// sized text elements would not line up.

	// Another thing to do with this example is to narrow the window until text starts wrapping.
	// Every new line should define its own baseline.

	auto root = &doc->Root;
	root->StyleParse("padding: 10px;");
	root->StyleParse("background: #ddd");
	doc->ClassParse("baseline", "baseline:baseline");

	if (1) {
		root->ParseAppend("<div                  style='font-size: 38px; font-family: Microsoft Sans Serif; background: #ffe0e0'>H</div>");
		root->ParseAppend("<div class='baseline' style='font-size: 13px; font-family: Microsoft Sans Serif; background: #e0ffe0'>ello.</div>");
		//root->ParseAppend("<div class='baseline' style='font-size: 18px; font-family: Times New Roman; background: #e0e0ff'> More times at a smaller size.</div>");
		root->ParseAppend("<span class='baseline' style='font-size: 18px; font-family: Times New Roman; background: #e0e0ff'> More times at a smaller size.</span>");
	}

	// ramp of 'e' characters from 8 to 30 pixels
	if (1) {
		root->StyleParse("font-family: Segoe UI; background: #fff");

		for (int size = 8; size < 30; size++) {
			auto txt = root->AddNode(xo::TagDiv);
			txt->AddClass("baseline");
			txt->StyleParse(tsf::fmt("font-size: %dpx; background: #e0e0e0", size).c_str());
			txt->SetText("e");
		}
	}
}

void DoBaselineAlignment_rev2(xo::Doc* doc) {
	auto       root = &doc->Root;
	xo::String e;
	int        v = 3;
	if (v == 1) {
		// only 1 deep
		e = root->Parse(
		    "<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
		    "	<lab style='hcenter: hcenter; vcenter: vcenter; background: #faa; width: 80ep; height: 50ep'/>"
		    "</div>");
	} else if (v == 2) {
		// This creates 2 blocks in a row.
		// The first block has 20ep text, and the second block has 10ep text.
		// The first block has text centered inside it. The second block's text is aligned to the baseline of the first.
		// Unfortunately this concept is impossible to imitate in Layout3, since we're no longer propagating baseline
		// down the tree, only horizontally across the tree. You'll just have to use injected-flow elements, such as "spans",
		// if you want baseline to cross over multiple objects at different heights.
		e = root->Parse(
		    "<div style='width: 150ep; height: 80ep; box-sizing: margin; margin: 0 4ep 0 4ep; background: #ddd'>"
		    "	<lab style='vcenter: vcenter; font-size: 40ep; background: #dbb'>Hello-p</lab>"
		    "</div>"
		    "<div style='width: 100ep; height: 80ep; box-sizing: margin; margin: 0 8ep 0 8ep; background: #bbb'>"
		    "	<lab style='baseline: baseline; font-size: 16ep; background: #bdb'>world</lab>"
		    "</div>");
		// TODO:
		/*
		<edit baseline:baseline, height:7, background:#fff, border: solid 1 #00c, width: 12em>
		  edit me
		<edit>
		*/
	} else if (v == 3) {
		// this is like 5, but changed to work with layout3
		e = root->Parse(
		    "<div style='width: 150ep; height: 80ep; box-sizing: margin; margin: 0 4ep 0 4ep; background: #ddd'>"
		    "	<lab style='vcenter: vcenter; font-size: 40ep; background: #dbb'>Hello-p</lab>"
		    "</div>"
		    "<div style='width: 100ep; height: 80ep; box-sizing: margin; margin: 0 8ep 0 8ep; background: #bbb; baseline: baseline;'>"
		    "	<lab style='vcenter: vcenter; font-size: 16ep; background: #bdb'>world</lab>"
		    "</div>");
	}

	XO_ASSERT(e == "");
}

void DoBaselineAlignment_DownPropagate(xo::Doc* doc) {
	// These examples were used when trying out downward baseline propagation with layout3.
	// AHEM. I have abandoned downward baseline propagation, hopefully for the last time.
	doc->Root.Parse(R"(
		<div style='font-family: Times New Roman; background: #fcc; font-size: 35ep'>I just called</div>
		<div style='font-family: Times New Roman; background: #fcf; font-size: 14ep'>to say hello</div>
	)");
}

void DoBaselineAlignment_Multiline(xo::Doc* doc) {
	auto root = &doc->Root;
	doc->ClassParse("baseline", "baseline: baseline");
	doc->ClassParse("big", "font-size: 20ep");
	doc->ClassParse("small", "font-size: 11ep");
	doc->ClassParse("breakbefore", "break: before");
	doc->ClassParse("breakafter", "break: after");
	root->Parse(
	    "<div class='big'                     >Line 1, text item A</div><div class='small baseline'>Line 1, text item B (break before)</div>"
	    "<div class='big baseline breakbefore'>Line 2, text item A</div><div class='small baseline'>Line 2, text item B (break before)</div>"
	    "<div class='big baseline breakbefore'>Line 3, text item A</div><div class='small baseline'>Line 3, text item B (break before)</div>"

	    "<div class='big breakbefore'         >Line 1, text item A</div><div class='small baseline breakafter'>Line 1, text item B (break after)</div>"
	    "<div class='big baseline'            >Line 2, text item A</div><div class='small baseline breakafter'>Line 2, text item B (break after)</div>"
	    "<div class='big baseline'            >Line 3, text item A</div><div class='small baseline breakafter'>Line 3, text item B (break after)</div>");
}

void DoCanvas(xo::Doc* doc) {
	auto           root     = &doc->Root;
	xo::DomCanvas* canvasEl = doc->Root.AddCanvas();
	int            w        = 200;
	int            h        = 200;
	canvasEl->SetSize(w, h);
	canvasEl->StyleParse("border-radius: 35ep");
	canvasEl->StyleParse("border: 13ep #a00e");
	canvasEl->StyleParse("background: #0f0");
	canvasEl->StyleParse("sizing: content");
	xo::Canvas2D* c2d = canvasEl->GetCanvas2D();
	for (int y = 0; y < h; y++) {
		int yb = (y & 4) == 4;
		for (int x = 0; x < w; x++) {
			int xb = (x & 4) == 4;
			int cb = xb ^ yb ? 255 : 0;
			c2d->SetPixel(x, y, xo::RGBA::Make(255 - y / 2, cb, cb, 255 - x / 2));
		}
	}
	c2d->SetPixel(0, 0, xo::RGBA::Make(255, 0, 0, 255));
	c2d->SetPixel(1, 0, xo::RGBA::Make(255, 0, 0, 255));
	c2d->SetPixel(0, 1, xo::RGBA::Make(255, 0, 0, 255));
	c2d->SetPixel(2, 2, xo::RGBA::Make(255, 0, 0, 255));
	c2d->SetPixel(3, 3, xo::RGBA::Make(255, 0, 0, 255));

	c2d->SetPixel(4, 4, xo::RGBA::Make(0, 255, 0, 255));
	c2d->SetPixel(5, 4, xo::RGBA::Make(0, 255, 0, 255));
	c2d->SetPixel(4, 5, xo::RGBA::Make(0, 255, 0, 255));
	c2d->SetPixel(6, 6, xo::RGBA::Make(0, 255, 0, 255));
	c2d->SetPixel(7, 7, xo::RGBA::Make(0, 255, 0, 255));
	c2d->Invalidate();
	delete c2d;
}

void DoCenter(xo::Doc* doc) {
	auto       root = &doc->Root;
	xo::String e;
	int        v = 3;
	if (v == 1) {
		// inner element's size comes from text impostor
		e = root->Parse(
		    "<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
		    "	<lab style='hcenter: hcenter; vcenter: vcenter; background: #faa;'>"
		    "		<div style='width: 80ep; height: 50ep; background: #eee'/>"
		    "	</lab>"
		    "</div>");
	} else if (v == 2) {
		// inner element's size comes from text
		e = root->Parse(
		    "<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
		    "	<lab style='hcenter: hcenter; vcenter: vcenter; background: #faa;'>Hello\nWorld!</lab>"
		    "</div>");
	} else if (v == 3) {
		// two text elements beneath each other, centered horizontally
		// txtContainer needs two passes over its children, so that it can center them horizontally, after it knows its own width.
		e = root->Parse(
		    "<div style='width: 140ep; height: 70ep; box-sizing: margin; background: #ddd; margin: 0 5ep 0 5ep'>"
		    "	<div style='hcenter: hcenter; vcenter: vcenter; background: #faa;'>"
		    "		<lab style='hcenter: hcenter; background: #cfc; break: after'>Hello</lab>"
		    "		<lab style='hcenter: hcenter; background: #cfc;'>&lt;World&gt;</lab>"
		    "	</div>"
		    "</div>");
	}
}

void DoHCenter(xo::Doc* doc) {
	doc->ClassParse("h-outer", "width: 140ep; height: 70ep; background: #fef; border: 1px #0008; margin: 1ep");
	doc->ClassParse("v-outer", "width: 140ep; height: 70ep; background: #ffe; border: 1px #0008; margin: 1ep");
	doc->ClassParse("h-inner", "vcenter: vcenter; background: #faa");
	doc->ClassParse("v-inner", "hcenter: hcenter; baseline: none; background: #faa;"); // note that we need to remove 'baseline' from <div>
	doc->Root.ParseAppend(
	    "<div class='h-outer'><lab class='h-inner' style='hcenter: hcenter; left: left'>stretch left</lab></div>"
	    "<div class='h-outer'><lab class='h-inner' style='hcenter: hcenter; right: right'>stretch right</lab></div>"
	    "<div class='h-outer'><lab class='h-inner' style='hcenter: hcenter; left: left; right: right'>stretch both 1</lab></div>"
	    "<div class='h-outer'><lab class='h-inner' style='left: left; right: right'>stretch both 2</lab></div>"
	    "<div class='h-outer'><lab class='h-inner' style='left: left'>move left (identity)</lab></div>"
	    "<div class='h-outer'><lab class='h-inner' style='right: right'>move right</lab></div>");
	doc->Root.ParseAppend(
	    "<div class='v-outer'><lab class='v-inner' style='vcenter: vcenter; top: top'>stretch top</lab></div>"
	    "<div class='v-outer'><lab class='v-inner' style='vcenter: vcenter; bottom: bottom'>stretch bottom</lab></div>"
	    "<div class='v-outer'><lab class='v-inner' style='vcenter: vcenter; top: top; bottom: bottom'>stretch both</lab></div>"
	    "<div class='v-outer'><lab class='v-inner' style='top: top'>move top (identity)</lab></div>"
	    "<div class='v-outer'><lab class='v-inner' style='bottom: bottom'>move bottom</lab></div>"
	    "<div class='v-outer'><lab class='v-inner' style='top: top; bottom: bottom'>stretch</lab></div>");
}

void DoTwoTextRects(xo::Doc* doc) {
	if (1) {
		xo::DomNode* div = doc->Root.AddNode(xo::TagDiv);
		div->StyleParse("width: 90px; height: 90px; background: #faa; margin: 4px");
		div->StyleParse("font-size: 13px");
		div->SetText("Ave quick brown fox jumps over the lazy dog.\nText wrap and kerning. >>");
	}

	if (1) {
		// This block has width=height=unspecified, so it gets its size from its children
		// We expect to see the green background behind this text
		xo::DomNode* div = doc->Root.AddNode(xo::TagDiv);
		div->StyleParse("background: #afa8; margin: 4px; padding: 5px");
		div->StyleParse("font-size: 13px");
		div->SetText("Parent has no size, but this text gives it size. Expect green background behind this text.\nPpPp\npPpP\n\naaa\naaapq");
	}
}

void DoBlockMargins(xo::Doc* doc) {
	doc->Root.StyleParse("padding: 4px");

	// This should produce a matrix of blocks that are perfectly separated by 8 pixels.
	// It would be 4 pixels between blocks if we collapsed margins, but we don't do that.
	if (1) {
		for (int i = 0; i < 20; i++) {
			xo::DomNode* div = doc->Root.AddNode(xo::TagDiv);
			div->StyleParse("width: 150px; height: 80px; background: #faa8; margin: 4px; border-radius: 5px;");
			div->StyleParse("font-size: 13px");
			div->SetText(tsf::fmt("  block %v", i).c_str());
		}
	}

	// First block has margins, second and third blocks have no margins
	// We expect to see a 4 pixel margin around the first block, regardless of the second block's settings
	if (0) {
		doc->Root.AddNode(xo::TagDiv)->StyleParse("width: 90px; height: 90px; background: #faa8; margin: 4px; border-radius: 5px;");
		doc->Root.AddNode(xo::TagDiv)->StyleParse("width: 90px; height: 90px; background: #faa8; margin: 0px; border-radius: 5px;");
		doc->Root.AddNode(xo::TagDiv)->StyleParse("width: 90px; height: 90px; background: #faa8; margin: 0px; border-radius: 5px;");
	}
}

void DoLongText(xo::Doc* doc) {
	auto div = doc->Root.AddNode(xo::TagDiv);
	div->StyleParse("padding: 10px; width: 500px; font-family: Times New Roman; font-size: 19px; color: #333; background: #eee;");
	div->SetText(
	    "It is an ancient Mariner,\n"
	    "And he stoppeth one of three.\n"
	    "'By thy long grey beard and glittering eye,\n"
	    "Now wherefore stopp'st thou me?\n"
	    "\n"
	    "The Bridegroom's doors are opened wide,\n"
	    "And I am next of kin;\n"
	    "The guests are met, the feast is set:\n"
	    "May'st hear the merry din.'\n"
	    "\n"
	    "He holds him with his skinny hand,\n"
	    "'There was a ship,' quoth he.\n"
	    "'Hold off! unhand me, grey-beard loon!'\n"
	    "Eftsoons his hand dropt he.\n");
}

// This was used when developing Layout3
void DoInlineFlow(xo::Doc* doc) {
	auto create = [doc](xo::Event& ev) {
		static bool doSpan = false;
		doSpan             = !doSpan;
		doc->Root.DeleteAllChildren();
		doc->Root.StyleParse("font-size: 26px");
		doc->Root.StyleParse("margin: 5px");
		doc->ClassParse("red", "margin: 2px; padding: 2px; border-radius: 3px; border: 1px #d00b; background: #fddb");
		doc->ClassParse("blue", "margin: 2px; padding: 2px; border-radius: 3px; border: 1px #00d; background: #ddf");
		//doc->Root.ParseAppend(R"(<div style='cursor: hand'>The dogge</div>)");
		//doc->Root.ParseAppend(R"(The quick <span style='color: #a00; background: #aaa; cursor: hand'>brown fox jumps</span> over)");
		//doc->Root.ParseAppend(R"(The slow quick fast one two three four five six seven eight nine <span class='red'>brown fox jumps</span> over)");
		//doc->Root.ParseAppend(R"(The brown fox)");
		//doc->Root.ParseAppend(R"(The <span class='red'>brown</span> fox)");
		//doc->Root.ParseAppend(R"(<span class='red'>The brown</span> fox)");
		//if (doSpan)
		//	doc->Root.ParseAppend(R"(The <span class='red'>brown</span> fox)");
		//else
		//	doc->Root.ParseAppend(R"(The brown fox)");
		//if (doSpan)
		//	doc->Root.ParseAppend(R"(The slow quick fast <span class='red'>brown Fox jumps</span> over)");
		//else
		//	doc->Root.ParseAppend(R"(The slow quick fast brown Fox jumps over)");
		//doc->Root.ParseAppend(R"(<span class='red'>brown</span> over)");
		//doc->Root.ParseAppend(R"(<span class='red'>brown fox jumps</span> over)");

		doc->Root.ParseAppend(R"(Once upon a time, The quick <span class='red'><span class='blue'>brown fox jumps</span></span> over)");
		//doc->Root.ParseAppend(R"(Once upon a time, The quick <span class='red'>brown fox jumps</span> over)");
		//doc->Root.ParseAppend(R"(Once upon a time, The quick <span class='red'>brown</span>)");
		//doc->Root.ParseAppend(R"(Once upon a time, The quick)");

		//doc->Root.ParseAppend(R"(one one <span class='red'>two two</span> three three)");
		//doc->Root.ParseAppend(R"(The <span class='red'>brown</span>)");
		//doc->Root.ParseAppend(R"(The <span class='red'><span class='blue'>brown</span></span>)");
		//doc->Root.ParseAppend(R"(<div style='cursor: hand'>blah!</div>)");
		//doc->Root.ParseAppend(R"(The <span style='color: #a00; background: #fff'>brown</span>)");
		//doc->Root.ParseAppend( R"(The quick)");
	};
	doc->Root.OnClick(create);
	create(xo::Event());
}

void DoBackupSettings(xo::Doc* doc) {
	// The goal here is to replicate part of bvckup2's UI
	xo::DomNode* root = &doc->Root;
	root->StyleParse("font-family: Segoe UI; font-size: 12px;");
	//root->StyleParse( "font-family: Audiowide; font-size: 12px;" );
	doc->ClassParse("pad-light", "box-sizing: border; background: #f8f8f8; width: 140ep; height: 10ep;");
	doc->ClassParse("pad-dark", "box-sizing: border; background: #efefef; width: 470ep; height: 10ep;");
	doc->ClassParse("bg-light", "box-sizing: border; color: #000; background: #f8f8f8; width: 140ep; height: 50ep; padding: 8ep;");
	doc->ClassParse("bg-dark", "box-sizing: border; color: #000; background: #efefef; width: 470ep; top: top; bottom: bottom");
	doc->ClassParse("textbox", "color: #000; background: #fff; padding: 5ep 3ep 5ep 3ep; margin: 6ep 3ep 6ep 3ep; border: 1px #bdbdbd; canfocus: true; cursor: text");
	doc->ClassParse("textbox:focus", "border: 1px #8888ee");
	doc->ClassParse("button", "color: #000; background: #ececec; margin: 6ep 0ep 6ep 0ep; padding: 14ep 3ep 14ep 3ep; border: 1px #bdbdbd; border-radius: 2.5ep; canfocus: true");
	doc->ClassParse("button:focus", "border: 1px #8888ee");
	doc->ClassParse("button:hover", "background: #ddd");
	doc->ClassParse("baseline", "baseline:baseline");

	auto horzPadder =
	    "<div style='break:after'>"
	    "	<div class='pad-light'></div>"
	    "	<div class='pad-dark'></div>"
	    "</div>";

	root->ParseAppend(horzPadder);

	auto addLine = [&](xo::String title) {
		root->ParseAppend(
		    "<div style='break:after'>"
		    "	<div class='bg-light'><lab style='vcenter:vcenter'>" +
		    title +
		    "</lab></div>"
		    "	<div class='bg-dark'>"
		    "		<lab class='textbox' style='width: 320ep'>this is a text box</lab>"
		    "		<lab class='button'>Browse...</lab>"
		    "	</div>"
		    "</div>");
	};

	addLine("Backup from");
	addLine("Backup to");
	addLine("Description");

	root->ParseAppend(horzPadder);
}

void DoPadding(xo::Doc* doc) {
	xo::DomNode* root = &doc->Root;
	root->ParseAppend("<div style='padding: 8ep; background: #ddd'><lab>8ep padding</lab></div>");
}

void DoTextQuality(xo::Doc* doc) {
	doc->Root.StyleParse("background: #f0f0f0");
	doc->Root.StyleParse("padding: 20px");

	doc->Root.ParseAppend("<div style='break:after; font-family: Microsoft Sans Serif'>The quick brown fox jumps over the lazy dog<div>");
	doc->Root.ParseAppend("<div style='break:after; padding: 20px; font-family: Microsoft Sans Serif'>h<div>");
	doc->Root.ParseAppend("<div style='break:after; font-family: Microsoft Sans Serif'>Backup from<div>");
	doc->Root.ParseAppend("<div style='break:after; font-family: Segoe UI; font-size: 12px; color: #f008'>Backup from<div>");
	doc->Root.ParseAppend("<div style='break:after; font-family: Consolas; font-size: 12px; color: #383'>DoBaselineAlignment_Multiline(doc)<div>");
}

void DoQuadraticSplines(xo::Doc* doc) {
	// In order for this example to be meaningful, you must uncomment the magic color detection at the top line of xoRenderer::RenderNode(),
	// and it was only implemented on OpenGL.
	// Also, you must disable backface culling, or the bottom (blue) curve won't render.

	doc->Root.ParseAppend("<div style='width: 10ep; height: 10ep; break: after'>top<div>"); // top pad
	doc->Root.ParseAppend("<div style='width: 20ep; height: 50ep'>left<div>");              // left pad
	doc->Root.ParseAppend("<div style='background: #fff0f0ff; width: 100ep; height: 100ep'><div>");
	auto top_pad  = doc->Root.ChildByIndex(0)->GetInternalID();
	auto left_pad = doc->Root.ChildByIndex(1)->GetInternalID();
	doc->Root.OnMouseMove([top_pad, left_pad](xo::Event& ev) {
		ev.Doc->GetNodeByInternalIDMutable(top_pad)->StyleParsef("height: %dep", (int) ev.PointsAbs[0].y);
		ev.Doc->GetNodeByInternalIDMutable(left_pad)->StyleParsef("width: %dep", (int) ev.PointsAbs[0].x);
	});
}

void DoTimer(xo::Doc* doc) {
	auto canvas = doc->Root.AddCanvas();
	canvas->SetSize(512, 512);

	int* size  = new int(0);
	int* dsize = new int(1);

	auto ontimer = [size, dsize, canvas](xo::Event& ev) {
		xo::Canvas2D* cx = canvas->GetCanvas2D();
		*size += *dsize;
		if (*size >= 100)
			*dsize = -1;
		else if (*size <= 0)
			*dsize = 1;
		cx->Fill(xo::Color::RGBA(255, 255, 255, 255));
		cx->FillRect(xo::Box(0, 0, *size, *size), xo::Color::RGBA(150, 0, 0, 255));
		canvas->ReleaseCanvas(cx);
	};
	canvas->OnTimer(ontimer, 10);
}

void DoEditBox(xo::Doc* doc) {
	//doc->Root.ParseAppend("<div style='break:after'>something</div>");
	auto edit = xo::controls::EditBox::AppendTo(&doc->Root);
	edit->StyleParse("width: 15em");
	auto btn = xo::controls::Button::AppendTo(&doc->Root, "Click me");
	btn->OnClick([doc](xo::Event& ev) {
		xo::controls::MsgBox::Show(doc, "How now brown cow\nA new line");
	});
}

void InitDOM(xo::Doc* doc) {
	xo::DomNode* body = &doc->Root;
	body->StyleParse("font-family: Segoe UI, Roboto");

	//DoBorder(doc);
	//DoBaselineAlignment(doc);
	//DoBaselineAlignment_rev2(doc);
	//DoBaselineAlignment_Multiline(doc);
	//DoBaselineAlignment_DownPropagate(doc);
	DoCanvas(doc);
	//DoCenter(doc);
	//DoHCenter(doc);
	//DoVCenter(doc);
	//DoTwoTextRects(doc);
	//DoBlockMargins(doc);
	//DoLongText(doc);
	//DoInlineFlow(doc);
	//DoBackupSettings(doc);
	//DoPadding(doc);
	//DoTextQuality(doc);
	//DoQuadraticSplines(doc);
	//DoTimer(doc);
	//DoEditBox(doc);

	body->OnClick([](xo::Event& ev) {
		//xo::Trace("%f %f\n", ev.PointsAbs[0].x, ev.PointsAbs[0].y);
		//xo::Trace("button: %d\n", (int) ev.Button);
		//xoGlobal()->EnableKerning = !xoGlobal()->EnableKerning;
		//Trace("InternalID: %d\n", ev.Target->GetInternalID());
		if (ev.Button == xo::Button::MouseLeft) {
			// Force a re-layout. Useful to click on the document and be able to debug the layout that occurs.
			ev.Doc->IncVersion();
		} else if (ev.Button == xo::Button::MouseRight) {
			// To make the best use of this, comment out the line inside Doc.cpp that says: TagStyles[TagBody].Parse("width: 100%; height: 100%", this)
			int width = (int) xo::PosToReal(ev.LayoutResult->Body()->Pos.Width());
			int height = (int) xo::PosToReal(ev.LayoutResult->Body()->Pos.Height());
			xo::Image img;
			if (ev.Doc->GetDocGroup()->Wnd->CopySurfaceToImage(xo::Box(0, 0, width, height), img)) {
				img.SaveToPng("c:\\temp\\xo-screenshot.png");
			}
		}
	});
}
