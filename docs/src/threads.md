# Threads

## Setting the Windows Cursor
erg.. It feels like it's going to take longer to describe the problems
with the system than to just fix it. But let it be said that there IS
a problem keeping the Windows cursor up to date. Right now it's always
behind by at least one mouse-move message. We need a more sophisticated
synchronization system than "add message to queue and forget".

The following diagrams are true for Windows. I intend to have the
other systems work in a similar fashion.

Event flow for a left mouse click:

	                Main Thread     UI Thread                         
	                +---------+     +-------+                         
	                          |     |                                 
	                          |     |                                 
	Left click is read from   | +-> |                                 
	the system message queue  |     |  Perform some action in response
	and added to our internal |     |  to the left click, and update  
	message queue             |     |  the Canonical DOM              
	                          |     |                                 
	Copy Canonical DOM from   | <-+ |                                 
	UI Thread to Main Thread  |     |                                 
	                          |     |                                 
	          Perform Layout  |     |                                 
	                          |     |                                 
	           Render to GPU  |     |                                 
	                          |     |                                 
	       Publish Layout as  |     |                                 
	       new Latest Layout  |     |                                 
	                          | +-> |  Future DOM event processing    
	                          |     |  uses the new Latest Layout     
	                          +     +                                 
