#include "pch.h"
#include "PathSync.h"
#include "../Platform/debuglib.h"
#include "../System/Date.h"
#include "../Other/profile.h"
#include "../Windows/Win.h"
#include "../Strings/wildcard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEBUG_PAUSE_MS 1500

using namespace Panacea::Sys;

namespace Panacea
{
namespace IO
{

PathSync::SStatistics::SStatistics()
{
	ResetPOD();
}

PathSync::SStatistics::~SStatistics()
{
}

void PathSync::SStatistics::ResetPOD()
{
	FilesIdentical = 0;
	FilesRORemoved = 0;
	FilesOverwritten = 0;
	FilesDeleted = 0;
	FilesCreated = 0;
	FoldersCreated = 0;
	FoldersDeleted = 0;
}

void PathSync::SStatistics::Reset()
{
	*this = PathSync::SStatistics();
	ResetPOD();
}

int PathSync::SStatistics::NumChanges() const
{
	int tot = 
		FilesOverwritten +
		FilesDeleted +
		FilesCreated +
		FoldersCreated +
		FoldersDeleted;
	return tot;
}

int PathSync::SStatistics::NumFailures() const
{
	int tot = 
		FoldersFailedCreate.size() +
		FoldersFailedDelete.size() +
		FilesFailedRORemove.size() + 
		FilesFailedTimeSet.size() +
		FilesFailedInspect.size() +
		FilesFailedCopy.size() +
		FilesFailedOverwrite.size() +
		FilesFailedDelete.size();
	if ( !FirstError.IsEmpty() && tot == 0 ) { ASSERT(false); tot++; }
	return tot;
}



PathSync::PathSync()
{
	Options.Silent = false;
	Options.Backup = false;
	Options.BackupOverwrite = true;
	Options.CaseSensitive = false;
	Options.Delete = true;
	Options.OnlyFresh = false;
	Options.RemoveRO = false;
	Options.DryRun = false;
	Options.PauseAfterEveryFile = false;
	Options.AbortOnError = false;
	Options.ProgEachItem = false;
	Options.Transaction = NULL;
	
	Random rnd( RDTSC() );
	char cookie_chars[] = "abcdefghijk";
	for ( int i = 0; i < 10; i++ )
		ClassCookie += cookie_chars[rnd.Int(0, arraysize(cookie_chars) - 1)];

	DirSep = L"\\";
	Reset();
}

PathSync::~PathSync()
{
}

void PathSync::Reset()
{
	Stats.Reset();
	LastProgTime = AbcRTC();
	Prog = NULL;
}

bool PathSync::Process( XStringW srcDir, XStringW dstDir, AbCore::IProgMon* prog )
{
	Reset();
	
	if ( srcDir.EndsWith('\\') ) srcDir.Chop();
	if ( dstDir.EndsWith('\\') ) dstDir.Chop();

	if ( Path::FolderExists( srcDir ) )
	{
		if ( !Path::FolderExists( dstDir ) )
		{
			if ( !Path::CreateFolder( dstDir, Options.Transaction ) )
			{
				TakeErrorAndPrint( XStringPFW(L"Failed to create destination folder %s", (LPCWSTR) dstDir) );
				return false;
			}
			else 
			{
				Print( L"Creating folder " + dstDir );
				Stats.FoldersCreated++;
			}
		}

		if ( Options.Backup )
		{
			if ( !FindBackupPath( dstDir ) )
			{
				TakeErrorAndPrint( "Failed to create backup path" );
				return false;
			}
		}

		Prog = prog;
		SrcRoot = srcDir;
		DstRoot = dstDir;
		Stats.StartTime = Date::Now();
		ProcessInternal( srcDir, dstDir );
		DeleteFile( TempFile() );
		Stats.EndTime = Date::Now();
		Prog = NULL;
		return true;
	}
	else
	{
		TakeErrorAndPrint( XStringPFW(L"Source %s does not exist", (LPCWSTR) srcDir) );
		return false;
	}
}

int64 PathSync::TotalBytes( XStringW srcDir, AbCore::IProgMon* prog )
{
	int64 total = 0;
	pvect<Panacea::IO::FileFindItem*> items;
	Path::FindFiles( srcDir + L"\\*.*", items );

	for ( int i = 0; i < items.size(); i++ )
	{
		if ( items[i]->Directory )
			total += TotalBytes( srcDir + L"\\" + items[i]->Name, prog );
		else
			total += items[i]->Size;
	}
	return total;
}

bool PathSync::FindBackupPath( XStringW dstDir )
{
	int pos = dstDir.ReverseFind( DirSep );
	if ( pos <= 0 ) return false;
	XStringW left = dstDir.Left( pos );
	XStringW right = dstDir.Mid( pos + 1 );
	XStringW backDir = dstDir + L"_Backup";
	if ( Path::FolderExists( backDir ) )
	{
		if ( Options.BackupOverwrite )
		{
			if ( Path::DeleteFolder( backDir ) )
			{
				if ( Path::CreateFolder( backDir ) )
				{
					BackupRoot = backDir;
					return true;
				}
			}
		}
	}
	else
	{
		if ( Path::CreateFolder( backDir ) )
		{
			BackupRoot = backDir;
			return true;
		}
	}
	return false;
}

void PathSync::SetProg( const XStringW& msg )
{
	if ( !Prog ) return;
	double t = AbcRTC();
	if ( Options.ProgEachItem || t - LastProgTime > 0.3 )
	{
		double pc = Options.NumFilesToProgPercent ? Stats.FilesCreated : 0;
		Prog->ProgSet( msg, pc );
		LastProgTime = t;
	}
}

bool PathSync::IsExcluded( const XStringW& name )
{
	for ( int i = 0; i < Options.Exclude.size(); i++ )
	{
		if ( MatchWildcard( name, Options.Exclude[i], Options.CaseSensitive ) )
			return true;
	}
	return false;
}

void PathSync::BuildMap( pvect< FileFindItem* >& items, WStrIntMap& map )
{
	map.clear();
	for ( int i = 0; i < items.size(); i++ )
	{
		if ( IsExcluded(items[i]->Name) ) continue;
		if ( Options.CaseSensitive ) map[ items[i]->Name ] = i;
		else map[ items[i]->Name.UpCase() ] = i;
	}
}

void PathSync::ProcessInternal( XStringW src, XStringW dst )
{
	// progress
	PrintRaw( L"." );
	SetProg( L"--< " + src + L" >--" );

	if ( !Path::FolderExists( dst ) )
	{
		if ( Options.DryRun )
		{
			Stats.FoldersCreated++;
			return;
		}
		else
		{
			if ( !Path::CreateFolder( dst, Options.Transaction ) )
			{
				TakeError( L"Creating directory " + dst );
				Stats.FoldersFailedCreate.push_back( dst );
				return;
			}
			else
			{
				Print( L"Creating folder " + dst );
				Stats.FoldersCreated++;
			}
		}
	}

	// setup state
	SrcDir = src;
	DstDir = dst;

	dvect<XStringW> srcDirs, dstDirs;

	// enumerate files
	SrcItems.clear();
	DstItems.clear();
	Path::FindFiles( SrcDir + L"\\*.*", SrcItems, 0, NULL );
	Path::FindFiles( DstDir + L"\\*.*", DstItems, 0, Options.Transaction );

	// build lookups for the names
	BuildMap( SrcItems, SrcItemMap );
	BuildMap( DstItems, DstItemMap );

	// match src -> dst
	for ( int i = 0; i < SrcItems.size() && !IsCancelledOrAborted(); i++ )
	{
		const XStringW& name = SrcItems[i]->Name;
		//if ( name == L"." ) continue;  - removed by FindFiles
		//if ( name == L".." ) continue;
		if ( IsExcluded(name) ) continue;

		if ( SrcItems[i]->Directory )
		{
			srcDirs.push_back( name );
			continue;
		}

		int posDst = FindName( name, DstItemMap );
		if ( posDst < 0 )
		{
			// source file does not exist in dest.
			SetProg( L"New File: " + name );
			CopyNewFile( name );
		}
		else if ( !Options.OnlyFresh )
		{
			// destination file exists.. check whether the two are identical
			bool dstRO;
			if ( !FilesIdentical( name, dstRO ) )
			{
				SetProg( L"Overwrite: " + name );
				CopyExistingFile( name );
			}
			else
			{
				SetProg( L"--< " + name + L" >--" );
				if ( dstRO && Options.RemoveRO )
				{
					if ( RemoveRO(DstDir + DirSep + name) )
						Stats.FilesRORemoved++;
					else
					{
						TakeError( L"Removing read-only rights on " + name );
						Stats.FilesFailedRORemove += name;
					}
				}
				Stats.FilesIdentical++;
			}
		}
	}

	if ( Options.Delete )
	{
		// match dst <- dst (delete files that do not exist in src)
		for ( int i = 0; i < DstItems.size() && !IsCancelledOrAborted(); i++ )
		{
			const XStringW& name = DstItems[i]->Name;
			if ( name == L"." ) continue;
			if ( name == L".." ) continue;
			if ( IsExcluded(name) ) continue;

			if ( DstItems[i]->Directory )
			{
				dstDirs.push_back( name );
				continue;
			}

			int posSrc = FindName( name, SrcItemMap );
			if ( posSrc < 0 )
			{
				// file does not exist in src so delete it from dst
				SetProg( L"Delete: " + name );
				DeleteDstFile( name );
			}
		}

		// operate on sub directories.

		// delete directories that do not exist in src
		for ( int i = 0; i < dstDirs.size(); i++ )
		{
			int srcPos = FindName( dstDirs[i], SrcItemMap );
			if ( srcPos < 0 )
			{
				DeleteDstFolder( dstDirs[i] );
			}
		}
	}

	delete_all( SrcItems );
	delete_all( DstItems );

	// recurse into directories existent in src
	// WARNING! This recursion means that we are losing all our state data.. so we copy only that which we need.
	XStringW srcDirCopy = SrcDir;
	XStringW dstDirCopy = DstDir;
	for ( int i = 0; i < srcDirs.size() && !IsCancelledOrAborted(); i++ )
	{
		ProcessInternal( srcDirCopy + DirSep + srcDirs[i], dstDirCopy + DirSep + srcDirs[i] );
	}
}

bool PathSync::IsCancelledOrAborted()
{
	if ( Options.AbortOnError && Stats.NumFailures() != 0 ) return true;
	return Prog && Prog->ProgIsCancelled();
}

void PathSync::DeleteDstFolder( const XStringW& name )
{
	if ( Options.DryRun )
	{
		Stats.FoldersDeleted++;
		return;
	}
	ASSERT( Options.Delete );
	XStringW dstName = DstDir + DirSep + name;
	if ( Path::DeleteFolder( dstName, Options.Transaction ) )
	{
		Print( L"Deleting folder: " + dstName );
		Stats.FoldersDeleted++;
	}
	else
	{
		TakeError( L"Deleting directory " + dstName );
		Stats.FoldersFailedDelete.push_back( dstName );
	}
}

DWORD PathSync::GetAttribs( const XStringW& name, Side side )
{
	if ( Options.Transaction && side == DESTINATION )
	{
		WIN32_FILE_ATTRIBUTE_DATA ad;
		if ( !NTDLL()->GetFileAttributesTransactedW( name, GetFileExInfoStandard, &ad, Options.Transaction ) )
			return INVALID_FILE_ATTRIBUTES;
		return ad.dwFileAttributes;
	}
	else
		return GetFileAttributes( name );
}

bool PathSync::SetAttribs( const XStringW& name, DWORD attribs )
{
	if ( Options.Transaction )	return !!NTDLL()->SetFileAttributesTransactedW( name, attribs, Options.Transaction );
	else						return !!SetFileAttributes( name, attribs );
}

bool PathSync::FilesIdentical( const XStringW& name, bool& dstRO )
{
	//if ( name == "Ref_Township_App.PX" )
	//	int aaa = 1;
	dstRO = false;

	XStringW srcName = SrcDir + DirSep + name;
	XStringW dstName = DstDir + DirSep + name;

	DWORD src = GetAttribs( srcName, SOURCE );
	DWORD dst = GetAttribs( dstName, DESTINATION );
	if (	src == INVALID_FILE_ATTRIBUTES ||
				dst == INVALID_FILE_ATTRIBUTES ) 
	{
		if ( src == INVALID_FILE_ATTRIBUTES ) { TakeError( L"Retrieving file attributes " + srcName ); Stats.FilesFailedInspect.push_back( srcName ); }
		if ( dst == INVALID_FILE_ATTRIBUTES ) { TakeError( L"Retrieving file attributes " + dstName ); Stats.FilesFailedInspect.push_back( dstName ); }
		ASSERT( false );
		return false;
	}
	else
	{
		// mask out attribute that we don't care about
		DWORD ignore = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL;
		ignore |= FILE_ATTRIBUTE_READONLY;
		dstRO = !!(dst & FILE_ATTRIBUTE_READONLY);
		src &= ~ignore;
		dst &= ~ignore;

		// check attributes
		if ( src != dst )
		{
			if ( Options.Verbose >= 1 ) PrintRaw( XStringPFW(L"Attribs: %d vs %d (%s)\n", src, dst, (LPCWSTR) srcName ) );
			return false;
		}

		// check times
		Date srcCreate, srcAccess, srcWrite;
		Date dstCreate, dstAccess, dstWrite;

		u64 srcSize;
		u64 dstSize;
		Path::GetFileTimes( srcName, srcCreate, srcAccess, srcWrite, &srcSize, NULL );
		Path::GetFileTimes( dstName, dstCreate, dstAccess, dstWrite, &dstSize, Options.Transaction );

		// pay attention only to modification times
		double writeApart = Date::SecondsApart( srcWrite, dstWrite );
		if ( fabs(writeApart) > 1 )
		{
			if ( Options.Verbose >= 1 ) PrintRaw( XStringPFW(L"Write Time: %.01f vs %.01f (%s)\n", srcWrite.SecondsUTC_1970(), dstWrite.SecondsUTC_1970(), (LPCWSTR) srcName ) );
			return false;
		}

		// check sizes
		if ( srcSize != dstSize )
		{
			if ( Options.Verbose >= 1 ) PrintRaw( XStringPFW(L"Size: %I64d vs %I64d (%s)\n", srcSize, dstSize, (LPCWSTR) srcName ) );
			return false;
		}

		return true;
	}
}

bool PathSync::RemoveRO( const XStringW& name )
{
	if ( Options.DryRun )
	{
		return true;
	}
	DWORD attribs = GetAttribs( name, DESTINATION );
	attribs &= ~FILE_ATTRIBUTE_READONLY;
	return SetAttribs( name, attribs );
}

bool PathSync::DeleteFileInt( const XStringW& name )
{
	bool ok;
	if ( Options.Transaction )	ok = !!NTDLL()->DeleteFileTransactedW( name, Options.Transaction );
	else						ok = !!DeleteFileW( name );
	return ok;
}

bool PathSync::CopyFileInt( const XStringW& src, const XStringW& dst, bool failIfExist )
{
	bool ok;

	DWORD flags = failIfExist ? COPY_FILE_FAIL_IF_EXISTS : 0;
	if ( Options.Transaction )	ok = !!NTDLL()->CopyFileTransactedW( src, dst, NULL, NULL, NULL, flags, Options.Transaction );
	else						ok = !!CopyFileW( src, dst, failIfExist );

	if ( !ok && Options.Transaction && GetLastError() == ERROR_TRANSACTIONS_UNSUPPORTED_REMOTE )
	{
		// Copy to intermediate first
		if ( CopyFile( src, TempFile(), false ) )
			ok = !!NTDLL()->CopyFileTransactedW( TempFile(), dst, NULL, NULL, NULL, flags, Options.Transaction );
		else
			ok = false;
	}

	return ok;
}

void PathSync::DeleteDstFile( const XStringW& name )
{
	if ( Options.DryRun )
	{
		Stats.FilesDeleted++;
		return;
	}
	ASSERT( Options.Delete );
	// we must first remove the readonly attribute because DeleteFile does not like it.
	XStringW dstName = DstDir + DirSep + name;
	if ( !RemoveRO( dstName ) )
	{
		// maybe DeleteFile still deletes it...
	}

	if ( DeleteFileInt( dstName ) != 0 )
	{
		Stats.FilesDeleted++;
		Print( L"Deleting file: " + dstName );
	}
	else
	{
		TakeError( L"Deleting file " + dstName );
		Stats.FilesFailedDelete.push_back( dstName );
	}
}

XString	PathSync::TempFile()
{
	XString tmpDir = Options.IntermediateRoot;
	if ( tmpDir.IsEmpty() )
		tmpDir = Path::GetTempFolder();
	return tmpDir + DirSep + ClassCookie;
}

void PathSync::CopyNewFile( const XStringW& name )
{
	if ( Options.DryRun )
	{
		Stats.FilesCreated++;
		return;
	}
	XStringW srcName = SrcDir + DirSep + name;
	XStringW dstName = DstDir + DirSep + name;
	bool ok = CopyFileInt( srcName, dstName, true ) != 0;
	if ( ok )
	{
		if ( Options.RemoveRO ) RemoveRO( dstName ); // ignore failure
		CopyFileTimes( name );
		Stats.FilesCreated++;
		Print( L"Creating file: " + dstName );
	}
	else
	{
		TakeError( L"Creating file " + dstName );
		Stats.FilesFailedCopy.push_back( SrcDir + DirSep + name );
	}
	if ( Options.PauseAfterEveryFile ) Sleep( DEBUG_PAUSE_MS );
}

void PathSync::CopyExistingFile( const XStringW& name )
{
	if ( Options.DryRun )
	{
		Stats.FilesOverwritten++;
		return;
	}

	XStringW srcName = SrcDir + DirSep + name;
	XStringW dstName = DstDir + DirSep + name;
	
	// we must first remove the readonly attribute because DeleteFile does not like it.
	if ( !RemoveRO( dstName ) )
	{
		TakeError( L"Removing read-only attribute from " + dstName );
		Stats.FilesFailedOverwrite.push_back( dstName );
		return;
	}

	// [30-August-2011] Why delete the file first? You're throwing away metadata here.
	// I wonder if I did this back when I was using this to sync up my work beween home and the office.
	//if ( DeleteFileW( dstName ) == 0 ) 
	//{
	//	Stats.FilesFailedOverwrite.push_back( dstName );
	//	return;
	//}

	if ( !CopyFileInt( srcName, dstName, false ) )
	{
		TakeError( L"Copying to file " + dstName );
		Stats.FilesFailedOverwrite.push_back( dstName );
		return;
	}

	if ( Options.RemoveRO ) RemoveRO( dstName ); // ignore failure

	CopyFileTimes( name );

	Print( L"Overwriting file: " + dstName );

	Stats.FilesOverwritten++;
	if ( Options.PauseAfterEveryFile ) Sleep( DEBUG_PAUSE_MS );
}

void PathSync::CopyFileTimes( const XStringW& name )
{
	Date timeCreate, timeAccess, timeWrite;
	XString srcFull = SrcDir + DirSep + name;
	XString dstFull = DstDir + DirSep + name;
	if ( Path::GetFileTimes( srcFull, timeCreate, timeAccess, timeWrite, NULL, NULL ) )
	{
		if ( !Path::SetFileTimes( dstFull, timeCreate, timeAccess, timeWrite, Options.Transaction ) )
		{
			TakeError( L"Setting file time on " + dstFull );
			Stats.FilesFailedTimeSet.push_back( dstFull );
		}
	}
	else
	{
		TakeError( L"Retrieving file time on " + srcFull );
		Stats.FilesFailedInspect.push_back( srcFull );
	}
}

int PathSync::FindName( const XStringW& name, WStrIntMap& map )
{
	if ( Options.CaseSensitive ) 
	{
		if ( !map.contains( name ) ) return -1;
		else return map[ name ];
	}
	else
	{
		if ( !map.contains( name.UpCase() ) ) return -1;
		return map[ name.UpCase() ];
	}
}

void PathSync::Print( const XStringW& msg )
{
	if ( Options.Silent ) return;
	wprintf( L"%s\n", (LPCWSTR) msg );
}

void PathSync::PrintRaw( const XStringW& msg )
{
	if ( Options.Silent ) return;
	wprintf( L"%s", (LPCWSTR) msg );
}

XStringW PathSync::PrintStats()
{
	int seconds = (int) (Stats.EndTime.MicroSecondsUTC_1970() - Stats.StartTime.MicroSecondsUTC_1970()) / 1000000;
	int minutes = seconds / 60;
	int minsec = seconds - (minutes * 60);

	XStringW str = L"Statistics:\n";
	str += XStringW::FromFormat( L"Time:               %d:%02d\n", minutes, minsec );
	str += XStringW::FromFormat( L"Files Identical:    %d\n", Stats.FilesIdentical );
	str += XStringW::FromFormat( L"Files Overwritten:  %d\n", Stats.FilesOverwritten );
	str += XStringW::FromFormat( L"Files Deleted:      %d\n", Stats.FilesDeleted );
	str += XStringW::FromFormat( L"Files Created:      %d\n", Stats.FilesCreated );
	str += XStringW::FromFormat( L"Files Read-Only:    %d\n", Stats.FilesRORemoved );
	str += XStringW::FromFormat( L"Folders Created:    %d\n", Stats.FoldersCreated );
	str += XStringW::FromFormat( L"Folders Deleted:    %d\n", Stats.FoldersDeleted );

	PrintList( Stats.FoldersFailedCreate, str,	L"Folders that could not be created" );
	PrintList( Stats.FoldersFailedDelete, str,	L"Folders that failed deletion" );
	PrintList( Stats.FilesFailedInspect, str,	L"Files that failed inspection" );
	PrintList( Stats.FilesFailedTimeSet, str,	L"Files that would not have their time set" );
	PrintList( Stats.FilesFailedCopy, str,		L"Files that failed to be copied" );
	PrintList( Stats.FilesFailedOverwrite, str, L"Files that failed to be overwritten" );
	PrintList( Stats.FilesFailedDelete, str,	L"Files that failed to be deleted" );
	PrintList( Stats.FilesFailedRORemove, str,	L"Files that failed to have their read-only attributes removed" );

	return str;
}

void PathSync::PrintList( const dvect<XStringW>& list, XStringW& toStr, LPCWSTR msg )
{
	if ( list.size() == 0 ) return;
	toStr += msg;
	toStr += L"\n";
	for ( int i = 0; i < list.size(); i++ )
	{
		toStr += list[i] + L"\n";
	}
}

void PathSync::TakeErrorAndPrint( XString msg )
{
	Print( msg );
	TakeError( msg );
}

void PathSync::TakeError( XString msg )
{
	if ( Stats.FirstError != "" ) return;
	if ( msg != "" ) Stats.FirstError = msg + L"\n";
	Stats.FirstError += L" " + AbcSysLastErrorMsg();
}

}
}
