#pragma once

namespace Panacea
{
	namespace IO
	{
		/** Auto-sensing memory/disk file.
		AutoMemFile begins by using a memory file to write to, but if
		we run out of memory, then we swap to a temporary disk file.
		**/ 
		class PAPI AutoMemFile : public AbCore::IFile
		{
		protected:
			bool SwitchFailed;
			bool DiskMode;
			AbCore::IFile* BaseFile;

			bool SwitchToDisk( INT64 preserveBytes );

		public:

			/** Specify the maximum amount of memory to allocate.
			If this value is zero, then the file only switches to disk when
			a realloc fails. The default is zero.
			**/
			size_t MaximumMemory;

			AutoMemFile( size_t initial = 0 );
			virtual ~AutoMemFile();
			
			void Reset( size_t desiredMemory = 0 );

			/** Reserve a specific number of bytes.
			If successful, Length() will be at least requiredBytes.
			\return True if the buffer could allocate enough space.
			**/
			bool Reserve( size_t requiredBytes );

			bool UsingMemFile();

			/** Explicitly retrieve the pointer to the start of the MemFile data.
			This will return null if the file is not operating off a memfile, and will
			debug assert.
			**/
			void* GetMemFileData();

			virtual bool Error();
			virtual XString ErrorMessage();
			virtual void ClearError();

			virtual bool Flush( int flags );

			virtual size_t Write( const void* data, size_t bytes );

			virtual size_t Read( void* data, size_t bytes )
			{
				return BaseFile->Read( data, bytes );
			}

			// Have to re-overload these because of a stupid c++ rule.
			size_t Write( size_t bytes, IFile* src )
			{
				return AbCore::IFile::Write( bytes, src );
			}
			
			size_t Read( size_t bytes, IFile* dst )
			{
				return AbCore::IFile::Read( bytes, dst );
			}

			virtual UINT64 Position()
			{
				return BaseFile->Position();
			}

			virtual UINT64 Length()
			{
				return BaseFile->Length();
			}

			virtual bool SetLength( UINT64 len )
			{
				ASSERT(false);
				return false;
			}

			virtual bool Seek(UINT64 pos)
			{
				return BaseFile->Seek( pos );
			}
		};
	}
}
