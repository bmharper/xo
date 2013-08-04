#pragma once
#include "../System/Date.h"

/*
Synchronization helper for a single plain file on disk, that is shared by a number of processes.

Usage:

	* Call Init()

	* Before every read, <if IsOutOfDate()>
		* Read latest
		* Call SetSynchronized()

	* After every write
		* Call SetSynchronized()

This is not intended to be used with a file on a network. The intention is that the file is on the machine's hard drive.
Principally, the check for whether the file has been updated needs to be fast.

This was originally created in order to help adb keep a database that is uses to populate auto-complete dropdown combo boxes
on database attributes.

A big assumption here is that you always read or write the entire file, never partially.

NOTE: We make no attempt to handle multiple simultaneous writes. The assumption is that the user is only using one
application at a time, so it's a user-time thing.

FINAL NOTE: There is very little going on here!

*/
class PAPI AbcSyncedFlatFile
{
public:
	
	XString		Path;
	AbcDate		LastWrite;
	u64			LastSize;

	AbcSyncedFlatFile();
	~AbcSyncedFlatFile();

	void	Init( XString path );	// Call this once
	void	Invalidate();			// Basically, this is "Set Not Synchronized"
	bool	IsOutOfDate();			// The file has been modified since you last read it or wrote it.
	void	SetSynchronized();		// Call this after reading or writing the file.
};

