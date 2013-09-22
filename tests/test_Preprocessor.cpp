#include "pch.h"

static const char* IfDef1 =
"The first line\n"
"#ifdef BLAH\n"
"I am blah\n"
"#else\n"
"I am not blah\n"
"#endif";

static const char* IfDefined1 =
"The first line\n"
"#if defined(BLAH)\n"
"I am blah\n"
"#else\n"
"I am not blah\n"
"#endif";

static const char* IfDefined2 =
"The first line\n"
"#if defined(FOO) || defined(BAR)\n"
"I am blah\n"
"#else\n"
"I am not blah\n"
"#endif";

static const char* IfDefined3 =
"The first line\n"
"#if defined(FOO) && defined(BAR)\n"
"I am blah\n"
"#else\n"
"I am not blah\n"
"#endif";

#define IDENTITY(input) TTASSERT( p.Run(input) == input )
#define IO(input, output) TTASSERT( p.Run(input) == output )

TESTFUNC(Preprocessor)
{
	nuPreprocessor p;
	IDENTITY("");
	IDENTITY(" ");
	IDENTITY("\n");
	IDENTITY("Hello World\n");
	IDENTITY("Hello World\n ");
	IDENTITY(" Hello World\n ");
	IDENTITY("\n\n");
	IDENTITY("a\nb\nc");

	TTASSERT( p.Run(IfDef1) == "The first line\nI am not blah\n" );
	
	p.SetMacro( "BLAH", "" );
	TTASSERT( p.Run(IfDef1) == "The first line\nI am blah\n" );

	p.ClearMacros();
	TTASSERT( p.Run(IfDefined1) == "The first line\nI am not blah\n" );

	p.SetMacro( "BLAH", "" );
	TTASSERT( p.Run(IfDefined1) == "The first line\nI am blah\n" );

	p.ClearMacros();
	TTASSERT( p.Run(IfDefined2) == "The first line\nI am not blah\n" );
	TTASSERT( p.Run(IfDefined3) == "The first line\nI am not blah\n" );

	p.SetMacro( "FOO", "" );
	TTASSERT( p.Run(IfDefined2) == "The first line\nI am blah\n" );
	TTASSERT( p.Run(IfDefined3) == "The first line\nI am not blah\n" );

	p.ClearMacros();
	p.SetMacro( "BAR", "" );
	TTASSERT( p.Run(IfDefined2) == "The first line\nI am blah\n" );
	TTASSERT( p.Run(IfDefined3) == "The first line\nI am not blah\n" );

	p.ClearMacros();
	p.SetMacro( "FOO", "" );
	p.SetMacro( "BAR", "" );
	TTASSERT( p.Run(IfDefined2) == "The first line\nI am blah\n" );
	TTASSERT( p.Run(IfDefined3) == "The first line\nI am blah\n" );

	p.ClearMacros();
	p.SetMacro( "MACRO", "macro" );
	TTASSERT( p.Run("Hello MACRO") == "Hello macro" );
	TTASSERT( p.Run("Hello MACROMACRO") == "Hello MACROMACRO" );

	p.ClearMacros();
	p.SetMacro( "MACRO", "macro" );
	p.SetMacro( "MACROS", "macros" );
	p.SetMacro( "MACROSES", "macroses" );
	p.SetMacro( "MACROSESEZ", "macrosesez" );
	TTASSERT( p.Run("Hello MACRO MACROS MACROSES MACROSESEZ") == "Hello macro macros macroses macrosesez" );
	TTASSERT( p.Run("_MACRO") == "_MACRO" );
	TTASSERT( p.Run("MACRO_") == "MACRO_" );
	TTASSERT( p.Run("0MACRO") == "0MACRO" );
	TTASSERT( p.Run("MACRO0") == "MACRO0" );
}


