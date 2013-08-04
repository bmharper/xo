#pragma once

namespace Panacea
{
	namespace IO
	{

#ifdef _INC_COMMDLG

		/** Aids in extraction of files from an OPENFILENAME structure. 
		**/
		class PAPI OpenFileHelper
		{
		public:
			/** Extracts files from an OPENFILENAME structure.
			This supports all forms of data that GetOpenFileName or GetSaveFileName return.
			**/
			static dvect<XString> Extract( OPENFILENAME* ofn )
			{
				dvect<XString> files;
				if ( (ofn->Flags & OFN_ALLOWMULTISELECT) == 0 )
				{
					// single select
					files += ofn->lpstrFile;
				}
				else
				{
					// multi select
					int start = 0;
					TCHAR* fptr = ofn->lpstrFile;

					for ( int i = 0; fptr[i] != 0; i++ )
					{
						if ( fptr[i + 1] == 0 && fptr[i + 2] == 0 )
						{
							// only one separator --> single file.
							files += fptr;
							return files;
						}
						start = i + 2;
					}

					XString dir = fptr;

					int pos = start;
					while ( true )
					{
						if ( fptr[pos] == 0 )
						{
							files += dir + DIR_SEP_WS + XString(&fptr[start]);
							start = pos + 1;
							if ( fptr[start] == 0 ) break;
						}
						pos++;
					}
				}
				return files;
			}
		};
#endif

		class FileFormatProvider;

		/** A file format usable by a FileFormatProvider object.
		**/
		class PAPI FileFormat
		{
		public:
			FileFormat()
			{
				CanRead = true;
				CanWrite = true;
				Provider = NULL;
			}
			virtual ~FileFormat() {}

			/** The provider that this format belongs to.
			This will be implicitly filled by the server if you use Register().
			**/
			FileFormatProvider* Provider;

			/** Available extensions (without the dot).
			When saving, the first extension is used.
			**/
			dvect<XString> Extensions;

			/** Name that appears to the user along with the extensions.
			**/
			XString Name;

			/** User provided id of the format.
			These id's must be shared by the writer and reader. This is because 
			an object will use the reader-provided format to read a file.
			Then, when the user clicks save, the file must be saved to the same
			format.
			**/
			int UserId;

			/** Use for whatever you want.
			**/
			int UserSubId;

			/// True if the provider supports reading of this file type. Default = true.
			bool CanRead;

			/// True if the provider supports writing of this file type. Default = true.
			bool CanWrite;
		};


		/** An object that provides read/write capability
		**/
		class PAPI FileFormatProvider
		{
		public:
			virtual ~FileFormatProvider() {}
			virtual void GetFormats( pvect< FileFormat* >& formats ) = 0;
		};

		
		/** Manager of available file formats.
		**/
		class PAPI FileFormatServer
		{
		public:
			FileFormatServer();
			~FileFormatServer();

			/** Retrieve the first file format that provides reading capability for the given filename.
			**/
			FileFormat* GetLoadFormat( XString filename );

			/** Retrieve the first file format that provides writing capability for the given filename.
			**/
			FileFormat* GetSaveFormat( XString filename );

			/** Retrieve a filter for use in the windows file open dialog box.
			@param includeAll If true, then the first item in the filter list is 'All Supported Files'.
			**/
			void GetFileOpenDialogFilter( dvect<TCHAR>& filter, bool includeAll = true );

			/** Retrieve a filter for use in the windows file save dialog box.
			**/
			void GetFileSaveDialogFilter( dvect<TCHAR>& filter );

			/** Register a new shape format.
			The server will delete the format object, but not the provider.
			**/
			void Register( FileFormatProvider* provider, FileFormat *format );

			/** Register all a provider's formats.
			The function calls FileFormatProvider::GetFormats().
			**/
			void Register( FileFormatProvider* provider );

		protected:

			void Clear();
			FileFormat* GetFormat( XString filename, bool save );

			void GetFileDialogFilter( bool forRead, bool forWrite, bool includeAll, dvect<TCHAR>& filter );

			pvect< FileFormat* > Formats;
		};
	}
}
