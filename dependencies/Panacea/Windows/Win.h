#ifdef _WIN32
#pragma once

namespace Panacea
{
	namespace Win
	{
		void PAPI WindowsMessageDispatch( HWND wnd );
	}
}

// NOTE: A negative height yields a top-down bitmap
HBITMAP AbcCreateDIBSection( int width, int height, void** bits );

// Delay-loaded kernel functions
// You probably want to use the Panacea-global NTDLL() function.
// WARNING: This class has no constructor. It is designed to be used as static data,
// which implies that it will get zero-initialized by the linker.
// That is the reason why it has no constructor.
struct NTDLLFunctions
{
	void EnsureInit();				// Does the GetProcAddress calls
	
	bool IsInitialized;
	bool AtLeastWinVista;
	bool AtLeastWin7;

	HANDLE
	(APIENTRY
	*CreateTransaction) (
			IN LPSECURITY_ATTRIBUTES lpTransactionAttributes OPTIONAL,
			IN LPGUID UOW OPTIONAL,
			IN DWORD CreateOptions OPTIONAL,
			IN DWORD IsolationLevel OPTIONAL,
			IN DWORD IsolationFlags OPTIONAL,
			IN DWORD Timeout OPTIONAL,
			__in_opt LPWSTR Description
			);

	BOOL
	(APIENTRY
	*CommitTransaction) (
			IN HANDLE TransactionHandle
			);

	BOOL
	(APIENTRY
	*RollbackTransaction) (
			IN HANDLE TransactionHandle
			);



	BOOL
	(WINAPI
	*CopyFileTransactedW) (
			__in     LPCWSTR lpExistingFileName,
			__in     LPCWSTR lpNewFileName,
			__in_opt LPPROGRESS_ROUTINE lpProgressRoutine,
			__in_opt LPVOID lpData,
			__in_opt LPBOOL pbCancel,
			__in     DWORD dwCopyFlags,
			__in     HANDLE hTransaction
			);

	BOOL
	(WINAPI
	*GetFileAttributesTransactedW) (
			__in  LPCWSTR lpFileName,
			__in  GET_FILEEX_INFO_LEVELS fInfoLevelId,
			__out LPVOID lpFileInformation,
			__in     HANDLE hTransaction
			);

	BOOL
	(WINAPI
	*SetFileAttributesTransactedW) (
			__in     LPCWSTR lpFileName,
			__in     DWORD dwFileAttributes,
			__in     HANDLE hTransaction
			);

	BOOL
	(WINAPI
	*DeleteFileTransactedW) (
			__in     LPCWSTR lpFileName,
			__in     HANDLE hTransaction
			);

	BOOL
	(WINAPI
	*CreateDirectoryTransactedW) (
			__in_opt LPCWSTR lpTemplateDirectory,
			__in     LPCWSTR lpNewDirectory,
			__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			__in     HANDLE hTransaction
			);

	BOOL
	(WINAPI
	*RemoveDirectoryTransactedW) (
			__in LPCWSTR lpPathName,
			__in     HANDLE hTransaction
			);

	__out
	HANDLE
	(WINAPI
	*FindFirstFileTransactedW) (
			__in       LPCWSTR lpFileName,
			__in       FINDEX_INFO_LEVELS fInfoLevelId,
			__out      LPVOID lpFindFileData,
			__in       FINDEX_SEARCH_OPS fSearchOp,
			__reserved LPVOID lpSearchFilter,
			__in       DWORD dwAdditionalFlags,
			__in       HANDLE hTransaction
			);

	__out
	HANDLE
	(WINAPI
	*CreateFileTransactedW) (
			__in       LPCWSTR lpFileName,
			__in       DWORD dwDesiredAccess,
			__in       DWORD dwShareMode,
			__in_opt   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			__in       DWORD dwCreationDisposition,
			__in       DWORD dwFlagsAndAttributes,
			__in_opt   HANDLE hTemplateFile,
			__in       HANDLE hTransaction,
			__in_opt   PUSHORT pusMiniVersion,
			__reserved PVOID  lpExtendedParameter
			);

};

// This is defined and initialized inside panaceaApp.cpp
PAPI NTDLLFunctions*	NTDLL();
#endif