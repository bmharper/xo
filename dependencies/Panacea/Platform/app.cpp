#include "pch.h"

static u32 ProcessCloseRequested = 0;

PAPI	bool	AbcProcessCloseRequested()
{
	return !!ProcessCloseRequested;
}

PAPI	void	AbcProcessCloseRequested_Set()
{
	ProcessCloseRequested = 1;
}
