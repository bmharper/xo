# Threads

The following diagrams are true for Windows. I intend to have the
other systems work in a similar fashion.

### Event flow for a left mouse click:

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


### Setting the cursor is somewhat special:

	                Main Thread     UI Thread                           
	                +---------+     +-------+                           
	                          |     |                                   
	Mouse move message is     |     |                                   
	read from the system      |     |                                   
	message queue. For a      |     |                                   
	WM_SETCURSOR, we use the  |     |                                   
	most recent cursor        |     |                                   
	that was computed by the  |     |                                   
	UI thread.                | +-> | Cursor position is hit-tested     
	                          |     | against most recent DOM rendering,
	                          |     | and the 'cursor' style is read    
	                          |     | from the DOM element that contains
	                          |     | the cursor.                       
	                          |     | If the cursor has changed, then a 
	                          |     | custom message is posted to the   
	                          |     | system window's queue, telling it 
	The custom message is     | <-+ | to refresh its cursor.            
	picked up, and the system |     |                                   
	cursor is updated.        |     |                                   
	                          +     +                                   
