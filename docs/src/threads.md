# Threads

## Setting the Windows Cursor
erg.. It feels like it's going to take longer to describe the problems
with the system than to just fix it. But let it be said that there IS
a problem keeping the Windows cursor up to date. Right now it's always
behind by at least one mouse-move message. We need a more sophisticated
synchronization system than "add message to queue and forget".

Also, I REALLY need to draw a thread diagram out and have it unified
across the different platforms. Or at least as little difference as
possible.