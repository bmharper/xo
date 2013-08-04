#include "pch.h"
#include <zlib/zlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <IO.h>
#include "CompoundFile.h"
#include "../Other/lmPlatform.h"
#include "../Strings/strings_os.h"
#include "VirtualFile.h"

using namespace AbCore;

#include "../Strings/stringutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CFID 0xabba0070
#define CFVER 1

CompoundFile::CompoundFile()
{
	cf = NULL;
	bWrite = false;
	bModify = false;
	OwnSourceFile = false; 
	OwnMemFile = false;
	SourceFile = NULL;
	mMemFile = NULL;
	memset( Head.Reserved, 0, CFTopHeader::HEAD_RESERVED );
	Head.Flags = 0;
	CaseSense = false;
	Age = 0;
	// use no more than 64k for name cache
	// This number ought really to be log2(n), for n subfiles, since that is
	// the expected number of searches required in a binary search. However,
	// since the last few searches all come from the same block, we can say
	// that this number ought to be perhaps log2(n) - 3.
	// we assume that the last 3 search items will fit into one block, 
	// since our block size is 20 and 2^3 = 8, which is less than half of 20.
	// sizeof(NameBlock) right now is 3376, yielding 19 blocks that will 
	// fit into 64k of storage.
	// 2 ^ (19 + 3) = 4 million, which is a reasonably large number of subfiles.
	// A compound file of 4 million subfiles would require 700mb simply to store
	// the subfile names (it could be much more efficient if we used a tree of course).
	// The point of this argument is that the algorithm shouldn't thrash. That is,
	// by the time the search reaches its destination, the two starting blocks should
	// still be in the cache. If not, the cache is very ineffective.
	MaxNameBlocks = (64 * 1024) / sizeof(NameBlock);
}

CompoundFile::~CompoundFile()
{
	Close();
	ClearSubStore();
}

void CompoundFile::FlushCache()
{
	delete_all( NameBlocks );
	LuckyMap.clear();
}

bool CompoundFile::SetCaseSensitive( bool on )
{
	if ( on == CaseSense ) return true;
	if ( GetSubCount() > 0 ) { ASSERT(false); return false; }
	CaseSense = on;
	return true;
}

void CompoundFile::ClearSubStore()
{
	delete_all( NameBlocks );
	delete_all( SubStore );
	Age = 0;
	SubMap.clear();
	SubSeekPos.clear();
	LuckyMap.clear();
	CachedLowName = "";
	CachedHighName = "";
}

// Opens a compound file from disk, or create a new one
bool CompoundFile::Open( LPCWSTR fname, UINT flags )
{
	Close();

	AbCore::DiskFile *diskfile;
	OwnSourceFile = true;
	SourceFile = diskfile = new AbCore::DiskFile;
	bool ok = false;
	
	bool create = (flags & OpenFlagCreate) != 0;
	bool modify = (flags & OpenFlagModify) != 0;

	if ( create )		ok = diskfile->Open( fname, L"wb+" );
	else if ( modify )	ok = diskfile->Open( fname, L"rb+" );
	else				ok = diskfile->Open( fname, L"rb" );

	if ( !ok )
	{
		OwnSourceFile = false;
		delete SourceFile; SourceFile = NULL;
		return false;
	}
	else
	{
		DiskFilename = fname;
		return Open( SourceFile, flags );
	}
}

// Opens a compound file
bool CompoundFile::Open( AbCore::IFile *file, UINT flags )
{
	bool create = (flags & OpenFlagCreate) != 0;
	bool modify = (flags & OpenFlagModify) != 0;

	UINT headFlags = 0;
	if ( flags & OpenFlagCompressAll ) headFlags |= HeadFlagCompressAll;

	bool ok = false;

	if ( create )
	{
		if ( flags & OpenFlagCompressAll )
		{
			if ( dynamic_cast<AbCore::MemFile*>(file) == NULL )
			{
				SourceFile = file;
				OwnMemFile = true;
				mMemFile = new AbCore::MemFile;
				cf = mMemFile;
			}
		}
		else
		{
			SourceFile = file;
			cf = file;
		}
		bModify = false;
		bWrite = true;
		// prep a new header
		Head.id = CFID;
		Head.SubCount = 0;
		Head.SubStart = sizeof(Head);
		Head.Version = CFVER;
		Head.Flags = headFlags;
		ClearSubStore();
		ok = true;
	}
	else 
	{
		cf = file;
		SourceFile = file;
		bWrite = false;
		bModify = modify;
		if ( ReadHeaders() )
		{
			ok = true;
		}
		else
		{
			Close();
			ok = false;
		}
	}

	if ( ok )
	{
		// skip past the top header. ready for writing the next sub now.
		cf->Seek( Head.SubStart );
	}

	return ok;
}

// open a compound file from a memory block
bool CompoundFile::Open( void* mem, size_t bytes, UINT flags )
{
	Close();
	OwnMemFile = true;
	mMemFile = new AbCore::MemFile( mem, bytes );
	return Open( mMemFile, flags );
}

// Closes the compound file
void CompoundFile::Close()
{
	if ( OwnSourceFile ) delete SourceFile; SourceFile = NULL;
	if ( OwnMemFile ) delete mMemFile; mMemFile = NULL;
	OwnSourceFile = false;
	OwnMemFile = false;
	cf = NULL;
	bWrite = false;
	bModify = false;
	ClearSubStore();
}

bool CompoundFile::IsOpen()
{
	return cf != NULL;
}

bool CompoundFile::IsOpenForCreate()
{
	return (cf != NULL) && bWrite;
}

bool CompoundFile::IsOpenForModification()
{
	return (cf != NULL) && bModify;
}

size_t CompoundFile::MemoryUsed()
{
	size_t sum = 0;
	if ( OwnMemFile && mMemFile != NULL )
		sum += mMemFile->Allocated;

	sum += sizeof( SubFile ) * SubStore.size();

	size_t mapSum = 0;
	for ( WStrIntMap::iterator it = SubMap.begin(); it != SubMap.end(); it++ )
	{
		mapSum += sizeof(*it) + it->first.Length() * 2;
	}

	sum += mapSum;

	return sum;
}

// Writes the compound file's headers
bool CompoundFile::Save()
{
	if ( !WriteHeaders() ) return false;

	if ( Head.Flags & HeadFlagCompressAll )
	{
		CompressGlobal();
		bool res = true;
		if ( SourceFile )
		{
			SourceFile->Write( mMemFile->Data, mMemFile->Length() );
			res = !SourceFile->Error();
		}
		UncompressGlobal();
		return res;
	}

	return true;
}

// Reads the header of the compound file
bool CompoundFile::ReadHeaders()
{
	cf->Seek( 0 );
	if ( cf->Read( &Head, sizeof(Head) ) != sizeof(Head) ) return false;
	if ( Head.id != CFID ) return false;
	if ( Head.Version != CFVER ) return false;
	
	CaseSense = (Head.Flags & HeadFlagCaseSensitive) != 0;

	if ( Head.Flags & HeadFlagCompressAll )
	{
		UncompressGlobal();
	}
	
	// Read the subitem headers
	if ( UsingNameBlocks() )
	{
		//TRACE( "Subfiles are sorted\n" );
		ClearSubStore();
		SubSeekPos.resize( Head.SubCount );
		SubSeekPos.fill( 0 );
		SubFile sf;
		// read first
		cf->Seek( Head.SubStart );
		cf->Read( &sf, sizeof(sf) );	CachedLowName = sf.FileName;
		// read last
		cf->Seek( Head.SubStart + sizeof(sf) * (Head.SubCount - 1) );
		cf->Read( &sf, sizeof(sf) );	CachedHighName = sf.FileName;
		if ( !CaseSense ) 
		{
			CachedLowName = CachedLowName.UpCase();
			CachedHighName = CachedHighName.UpCase();
		}
		if ( cf->Error() ) return false;
	}
	else
	{
		//TRACE( "Subfiles are NOT SORTED!\n" ); // Not necessarily true, since if we're open for modification or creation, then we don't use this mechanism.
		if ( !ReadAllSubs() ) return false;
	}

	return true;
}

bool CompoundFile::ReadAllSubs()
{
	ClearSubStore();
	SubStore.reserve( Head.SubCount );
	SubMap.resize( Head.SubCount * 2 );

	cf->Seek( Head.SubStart );

	//TRACE( "Reading %d subfiles\n", Head.SubCount );
	for ( int i = 0; i < (int) Head.SubCount; i++ )
	{
		SubStore += new SubFile;
		SubSeekPos += 0;
		SubFile* s = SubStore.back();
		if ( cf->Read( s, sizeof(SubFile) ) != sizeof(SubFile) ) return false;
		XStringW sname = s->FileName;
		if ( !CaseSense )	SubMap.insert( sname.UpCase(), i );
		else				SubMap.insert( sname, i );
	}
	//TRACE( "Subfile headers done\n" );
	return true;
}

int CompoundFile::GetSubIndex( LPCWSTR fname )
{
	if ( cf == NULL ) return -1;

	if ( UsingNameBlocks() )
	{
		return FindSubIndexFromNameBlock( fname );
	}
	else
	{
		XStringW upname = fname;
		if ( !CaseSense ) upname = upname.UpCase();

		if ( !SubMap.contains( upname ) ) return -1;
		return SubMap.get( upname );
	}
}

CompoundFile::SubFile* CompoundFile::GetSub( int index )
{
	if ( index < 0 ) return NULL;
	if ( UsingNameBlocks() )
	{
		return GetSubFromNameBlock( index );
	}
	else return SubStore[index];
}

CompoundFile::SubFile* CompoundFile::GetSub( LPCWSTR fname )
{
	int index = GetSubIndex( fname );
	if ( index >= 0 )
	{
		if ( UsingNameBlocks() )
		{
			return FindSubFromNameBlock( fname );
		}
		else
		{
			return SubStore[index];
		}
	}
	else return NULL;
}

int CompoundFile::GetSubCount()
{
	return std::max( (int) SubStore.size(), (int) Head.SubCount );
}

XStringW CompoundFile::GetSubName( int sub )
{
	if ( UsingNameBlocks() )
	{
		return GetSubFromNameBlock( sub )->FileName;
	}
	else
	{
		return SubStore[sub]->FileName;
	}
}

// Writes the header of the compound file
bool CompoundFile::WriteHeaders()
{
	// sort the subfiles lexicographically
	if ( CaseSense )	sort( SubStore, SubFile::Compare );
	else				sort( SubStore, SubFile::CompareUpCase );
	
	// always sort upcase, giving the user the option of saving a file in CaseSense mode, and opening
	// it in non-casesense mode.
	//sort( SubStore, SubFile::CompareUpCase );

	/*
	for ( int i = 0; i < SubStore.size(); i++ )
	{
		wprintf( L"%02d: %s\n", i, SubStore[i]->FileName );
	}
	*/

	ReindexNames();

	Head.Flags |= HeadFlagSorted;
	Head.Flags |= CaseSense ? HeadFlagCaseSensitive : 0;

	// write the head
	cf->Seek( 0 );
	if ( cf->Write( &Head, sizeof(Head) ) != sizeof(Head) ) return false;

	// verify that none of the sub files overwrite the sub header
	ASSERT( VerifyStructure() );

	// Write the sub file headers
	cf->Seek( Head.SubStart );
	for ( int i = 0; i < SubStore.size(); i++ )
	{
		if ( cf->Write( SubStore[i], sizeof(SubFile) ) != sizeof(SubFile) ) return false;
	}

	return true;
}

bool CompoundFile::VerifyStructure()
{
	for ( int i = 0; i < SubStore.size(); i++ )
	{
		if ( SubStore[i]->location < sizeof(Head) )
		{
			// file lies before the head
			ASSERT( false );
			return false;
		}
		if ( SubStore[i]->location + SubStore[i]->size > Head.SubStart )
		{
			// file lies within the sub list
			ASSERT( false );
			return false;
		}
	}
	return true;
}

// returns the size of the specified sub file, or zero if it does not exist
INT64 CompoundFile::SubSize( LPCWSTR fname )
{
	SubFile *s = GetSub( fname );
	if ( s == NULL ) return 0;

	return s->size;
}

// seeks to a position in the subfile. Always seeks from file origin.
bool CompoundFile::SeekSub( LPCWSTR fname, INT64 pos )
{
	int index = GetSubIndex( fname );
	if ( index < 0 ) return false;

	SubFile* s = GetSub( index );

	if ( bWrite )
		SubSeekPos[ index ] = pos;
	else
		SubSeekPos[ index ] = __min( s->size, pos );

	return true;
}

bool CompoundFile::ReadSub( LPCWSTR fname, dvect<BYTE>& buffer, INT64 bytes )
{
	int index = GetSubIndex( fname );
	SubFile *s = GetSub( index );
	if ( s == NULL ) return false;

	if ( bytes < 0 )
		bytes = s->size - SubSeekPos[ index ];

	if ( buffer.size() < bytes ) buffer.resize( bytes );
	return ReadSub( fname, buffer.data, bytes );
}

bool CompoundFile::ReadSub( LPCWSTR fname, void* buffer, INT64 bytes )
{
	int index = GetSubIndex( fname );
	SubFile *s = GetSub( index );
	if ( s == NULL ) return false;

	if ( bytes < 0 )
		bytes = s->size - SubSeekPos[ index ];
	
	cf->Seek( s->location + SubSeekPos[ index ] );
	if ( cf->Read( buffer, (size_t) bytes ) != bytes ) return false;
	SubSeekPos[ index ] += bytes;
	return true;
}

bool CompoundFile::WriteSub( LPCWSTR fname, const void* buffer, INT64 bytes )
{
	if ( cf == NULL ) return false;
	int index = GetSubIndex( fname );
	SubFile *s = GetSub( index );

	if ( s == NULL ) 
	{
		if ( bWrite || bModify )
		{
			SubStore += new SubFile;
			SubSeekPos += 0;
			index = SubStore.size() - 1;
			s = SubStore.back();
			NewSubFiles.insert( s );
			XStringW nameUp = fname;
			nameUp = nameUp.UpCase();

			if ( !CaseSense )	SubMap.insert( nameUp, SubStore.size() - 1 );
			else				SubMap.insert( fname, SubStore.size() - 1 );

			memset( s, 0, sizeof(*s) );
#if _MSC_VER >= 1400
			wcscpy_s( (wchar_t*) s->FileName, MaxFilenameLength, fname );
#else
			wcscpy( s->FileName, fname );
#endif
			s->location = Head.SubStart;
			s->size = 0;
			Head.SubCount++;
		}
		else
		{
			return false;
		}
	}

	if ( bModify && !NewSubFiles.contains( s ) && (SubSeekPos[ index ] + bytes > s->size) )
	{
		// modification may not enlarge sub files.
		return false;
	}

	cf->Seek( s->location + SubSeekPos[ index ] );
	size_t ww = cf->Write( buffer, (size_t) bytes );
	if ( ww != bytes ) 
	{
		//Head.SubCount--;
		return false;
	}
	s->size = max( s->size, SubSeekPos[ index ] + bytes );
	SubSeekPos[ index ] += bytes;
	Head.SubStart = max( Head.SubStart, s->location + s->size );
	return true;
}

bool CompoundFile::TruncateSub( LPCWSTR fname, INT64 size )
{
	SubFile* s = GetSub( fname );
	if ( s == NULL ) return false;

	if ( size > s->size ) return false;

	s->size = min( s->size, size );

	return true;
}

void CompoundFile::ReindexNames()
{
	SubMap.clear();
	for ( int i = 0; i < SubStore.size(); i++ )
	{
		XStringW name = SubStore[i]->FileName;
		if ( !CaseSense ) name = name.UpCase();
		ASSERT( !SubMap.contains( name ) );
		SubMap.insert( name, i );
	}
}

// Compresses everything except the head
void CompoundFile::CompressGlobal()
{
	if (mMemFile == NULL) { ASSERT(false); return; }
	size_t srclen = mMemFile->Length() - sizeof(Head);
	size_t tbuff_size = (size_t) (srclen * 1.1 + 20);
	BYTE* buff = (BYTE*) mMemFile->Data;
	void* srcbuff = buff + sizeof(Head);
	void* tbuff = malloc( tbuff_size );
	if (buff == NULL || tbuff == NULL) { ASSERT(false); return; }
	uLongf compsize = (uLongf) tbuff_size;
	if ( compress2( (Bytef*) tbuff, &compsize, (const Bytef*) srcbuff, (uLong) srclen, 6 ) == Z_OK )
	{
		// recreate the whole file (Head + compressed section)
		Head.UncompressedSize = srclen;
		Head.CompressedSize = compsize;
		if ( OwnMemFile )
		{
			delete mMemFile;
			mMemFile = NULL;
		}
		OwnMemFile = true;
		mMemFile = new AbCore::MemFile;
		mMemFile->Write( &Head, sizeof(Head) );
		mMemFile->Write( tbuff, compsize );
		cf = SourceFile;
	}
	free(tbuff);
}

// uncompresses everything but the head
// Head must already be read.
bool CompoundFile::UncompressGlobal()
{
	if (cf == NULL) { ASSERT(false); return false; }
	ASSERT( cf == SourceFile );
	// seek to just past the head. everything else is compressed
	cf->Seek( sizeof(Head) );
	BYTE* srcbuff = (BYTE*) malloc(Head.CompressedSize);
	BYTE* debuff = (BYTE*) malloc(Head.UncompressedSize);
	cf->Read( srcbuff, Head.CompressedSize );
	uLongf dstlen = Head.UncompressedSize;
	if ( uncompress( debuff, &dstlen, srcbuff, Head.CompressedSize ) != Z_OK )
	{
		ASSERT(false);
		return false;
	}
	ASSERT(dstlen == Head.UncompressedSize);
	// duplicate the whole file in memory, and discard the disk file
	delete mMemFile; mMemFile = NULL;
	OwnMemFile = true;
	mMemFile = new AbCore::MemFile;
	mMemFile->Write( &Head, sizeof(Head) );
	mMemFile->Write( debuff, dstlen );
	if ( OwnSourceFile )
	{
		delete SourceFile;
		SourceFile = NULL;
	}
	free(debuff);
	free(srcbuff);
	cf = mMemFile;
	return true;
}

bool CompoundFile::NamesEqual( LPCWSTR a, LPCWSTR b )
{
	if ( a == NULL || b == NULL )
		return false;

	if ( CaseSense )
		return wcsncmp( a, b, MaxFilenameLength ) == 0;
	else
		return _wcsicmp( a, b ) == 0;
}

bool CompoundFile::NamesEqual( const UINT16* a, LPCWSTR b )
{
	if ( a == NULL || b == NULL )
		return false;

	if ( CaseSense )
		return strcmp_u16_wchar( a, b, MaxFilenameLength ) == 0;
	else
		return stricmp_u16_wchar( a, b, MaxFilenameLength ) == 0;
}

bool CompoundFile::UsingNameBlocks()
{
	return (Head.Flags & HeadFlagSorted) && !IsOpenForCreate() && !IsOpenForModification();
}

CompoundFile::NameBlock* CompoundFile::GetNameBlock( int subIndex )
{
	for ( int i = 0; i < NameBlocks.size(); i++ )
	{
		if ( NameBlocks[i]->First <= subIndex && NameBlocks[i]->Last >= subIndex )
		{
			NameBlocks[i]->Age = Age++;
			return NameBlocks[i];
		}
	}

	if ( NameBlocks.size() >= MaxNameBlocks )
	{
		int oldest = least( NameBlocks );
		erase_delete( NameBlocks, oldest );
	}

	NameBlock* bl = new NameBlock;
	bl->Age = Age++;
	bl->First = (subIndex / NameBlock::BlockSize) * NameBlock::BlockSize;
	bl->Last = bl->First + NameBlock::BlockSize - 1;
	bl->Last = min( bl->Last, (int) Head.SubCount - 1 );
	if ( ReadNameBlock( bl ) )
	{
		NameBlocks += bl;
	}
	else
	{
		ASSERT(false);
		delete bl;
		bl = NULL;
	}
	
	return bl;
}

bool CompoundFile::ReadNameBlock( NameBlock* bl )
{
	int scount = 1 + bl->Last - bl->First;
	if ( !cf->Seek( Head.SubStart + bl->First * sizeof(SubFile) ) ) return false;

	size_t bytes = scount * sizeof(SubFile);
	return cf->Read( bl->Files, bytes ) == bytes;
}

CompoundFile::SubFile* CompoundFile::GetSubFromNameBlock( int subIndex )
{
	NameBlock* bl = GetNameBlock( subIndex );
	if ( bl == NULL ) { ASSERT(false); return NULL; }
	int rel = subIndex - bl->First;
	return &bl->Files[rel];
}

CompoundFile::SubFile* CompoundFile::FindSubFromNameBlock( LPCWSTR fname )
{
	int index = FindSubIndexFromNameBlock( fname );
	if ( index < 0 ) return NULL;
	return GetSubFromNameBlock( index );
}

int CompoundFile::FindSubIndexFromNameBlock( LPCWSTR fname )
{
	XStringW name = fname;
	XStringW nameU = fname;
	XStringW nameA = fname;
	nameU = nameU.UpCase();
	if ( !CaseSense ) nameA = nameU;
	if ( nameA < CachedLowName || CachedHighName < nameA ) return -1;
	
	int hash = nameA.GetHashCode();

	if ( LuckyMap.contains( hash ) )
	{
		// try the chance map first.
		SubFile* f = GetSubFromNameBlock( LuckyMap[hash] );
		if ( NamesEqual( f->FileName, fname ) ) return LuckyMap[hash];
	}

	if ( nameA == CachedLowName ) return 0;
	else if ( nameA == CachedHighName ) return GetSubCount() - 1;

	int result = -1;
	int first = 0;
	int last = GetSubCount() - 1;
	while ( last - first > 1 )
	{
		int mid = (first + last) / 2;
		SubFile* fmid = GetSubFromNameBlock( mid );
		XStringW iname = fmid->FileName;
		if ( !CaseSense ) iname = iname.UpCase();
		if ( NamesEqual( iname, name ) ) 
		{
			result = mid;
			break;
		}
		if ( nameA < iname )	last = mid;
		else					first = mid;
	}

	if ( result >= 0 )
	{
		LuckyMap[ nameA.GetHashCode() ] = result;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CompoundFile::SubFile::Compare( const SubFile& a, const SubFile& b )
{
	return strcmp_u16_u16( a.FileName, b.FileName, MaxFilenameLength );
}

// Way back when, I stupidly neglected to use wcsicmp. Instead, I converted to upper case, and then
// compared. So now we have to live with that mistake.
// Note that performing a case insensitive compare is NOT the same as converting to upper case in all cases,
// specifically where non-letter characters are involved.
// Equality is the same, but greater-than or less-than is NOT the same.
int CompoundFile::SubFile::CompareUpCase( const SubFile& a, const SubFile& b )
{
	//return stricmp_u16_u16( a.FileName, b.FileName, MaxFilenameLength );
	XStringW ta, tb;
	ta.SetFromU16LE( a.FileName, MaxFilenameLength );
	tb.SetFromU16LE( b.FileName, MaxFilenameLength );
	ta.MakeUpCase();
	tb.MakeUpCase();
	return ta.Compare( tb );
	/*
	XStringW ta, tb;
	wchar_t bufa[MaxFilenameLength+1], bufb[MaxFilenameLength+1];
	wcscpy_s( bufa, a.FileName );
	wcscpy_s( bufb, b.FileName );
	ta.MakeTemp( bufa );
	tb.MakeTemp( bufb );
	ta.MakeUpCase();
	tb.MakeUpCase();
	int r = ta.Compare( tb );
	ta.DestroyTemp();
	tb.DestroyTemp();
	return r;
	*/
}

