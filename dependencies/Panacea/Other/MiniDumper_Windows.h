#pragma once

#include "../IO/IFile.h"

/** MiniDump custom user data callback, which allows us to produce a log file to accompany the minidump file.
Note that this has nothing to do with the Windows Error Dump API callback function.
**/
typedef void (*MiniDump_WriteUserData) ( AbCore::IFile* log_file, wchar_t* video );

/** Release build end-user debugging aid.
**/
class PAPI MiniDumper
{
public:

	static void Install( MiniDump_WriteUserData custom_callback );
	static void SetRemoteParameters( XString dumpUrl, XString appName );
	static void SetNetworkDirectory( XString network_dir );
	static void SetLocalDirectory( XString local_dir );
	static bool DumpOutOfMemory( bool sendToRemote );
	static bool DumpGenericAndSend();
	static bool Dump( LPCTSTR filename );

	static void RunProtected( void* context, void (*callback) (void* cx) );

protected:
	static bool DumpInternal( LPCTSTR filename );

};
