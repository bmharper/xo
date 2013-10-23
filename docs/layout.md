Layout Concepts
===============

Horizontal vs Vertical
----------------------
When performing layout, a box can specify whether it uses
vertical or horizontal layout. Vertical is the regular thing
you expect, where new lines are underneath each other, and text
flows horizontally. In this case we say that the major axis
is vertical and the minor axis is horizontal.

A box can also specify that an axis direction is reversed.
The normal direction for vertical is top to bottom.
The normal direction for horizontal is left to right.
nuFlowDirection is the enum used to control this.

To set horizontal to reversed, use the following style:

	flow-direction-horizontal: reverse

To set vertical to normal, use the following style:

	flow-direction-vertical: normal

"Display" types
---------------
* __Block__ Block implies a line break before and after the element.
* __Inline__ Inline has no implicit line break, before or after.