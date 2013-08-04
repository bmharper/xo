#include "pch.h"
#include "FileFormatServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Panacea
{
namespace IO
{

FileFormatServer::FileFormatServer()
{
}

FileFormatServer::~FileFormatServer()
{
	Clear();
}

void FileFormatServer::GetFileSaveDialogFilter( dvect<TCHAR>& filter )
{
	GetFileDialogFilter( false, true, false, filter );
}

void FileFormatServer::GetFileOpenDialogFilter( dvect<TCHAR>& filter, bool includeAll )
{
	GetFileDialogFilter( true, false, includeAll, filter );
}

void Append( dvect<TCHAR>& dst, const XString& str, bool withZero )
{
	for ( int i = 0; i < str.Length(); i++ )
		dst += str[i];
	if ( withZero ) dst += 0;
}

void FileFormatServer::GetFileDialogFilter( bool forRead, bool forWrite, bool includeAll, dvect<TCHAR>& filter )
{
	filter.clear();

	if ( includeAll && Formats.size() > 0 )
	{
		Append( filter, "All Supported Files", true );

		for ( int i = 0; i < Formats.size(); i++ )
		{
			if ( !Formats[i]->CanRead ) continue;
			for ( int j = 0; j < Formats[i]->Extensions.size(); j++ )
			{
				filter += '*';
				filter += '.';
				Append( filter, Formats[i]->Extensions[j], false );
				filter += ';';
			}
		}

		// remove the last ';'
		filter.pop_back();

		filter += 0;
	}

	for ( int i = 0; i < Formats.size(); i++ )
	{
		FileFormat *format = Formats[i];
		
		if ( forRead && !format->CanRead ) continue;
		if ( forWrite && !format->CanWrite ) continue;

		Append( filter, format->Name, true );

		for ( int ext = 0; ext < format->Extensions.size(); ext++ )
		{
			filter += '*';
			filter += '.';

			Append( filter, format->Extensions[ext], false );

			if ( ext < format->Extensions.size() - 1 )
				filter += ';';
		}
		filter += 0;
	}
	filter += 0;
}

void FileFormatServer::Clear()
{
	delete_all( Formats );
}

void FileFormatServer::Register( FileFormatProvider* provider )
{
	pvect< FileFormat* > formats;
	provider->GetFormats( formats );
	for (int i = 0; i < formats.size(); i++)
		Register( provider, formats[i] );
}

void FileFormatServer::Register( FileFormatProvider* provider, FileFormat *format )
{
	format->Provider = provider;
	Formats.push_back( format );
}

FileFormat* FileFormatServer::GetFormat( XString filename, bool save )
{
	XString ext;
	Path::Split( filename, NULL, NULL, NULL, &ext );
	ext.TrimLeft( '.' );

	for ( int i = 0; i < Formats.size(); i++ )
	{
		FileFormat *format = Formats[i];
		if ( save && !format->CanWrite ) continue;
		if ( !save && !format->CanRead ) continue;
		for ( int j = 0; j < format->Extensions.size(); j++ )
		{
			XString test = format->Extensions[j];
			test.TrimLeft( '.' );
			if ( test.CompareNoCase( ext ) == 0 )
			{
				return format;
			}
		}
	}
	return NULL;
}

FileFormat* FileFormatServer::GetLoadFormat( XString filename )
{
	return GetFormat( filename, false );
}

FileFormat* FileFormatServer::GetSaveFormat( XString filename )
{
	return GetFormat( filename, true );
}

}
}
