#pragma once

// Returns true if the process is trying to close. This was created so that background tasks
// could cancel themselves when this is detected. Specifically, we want AbcBackgroundFileLoader to
// cancel itself if the process is trying to exit.
PAPI	bool	AbcProcessCloseRequested();
PAPI	void	AbcProcessCloseRequested_Set();

