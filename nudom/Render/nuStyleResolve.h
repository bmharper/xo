#pragma once

#include "../nuDoc.h"
#include "nuRenderDomEl.h"

class nuRenderStack;

/* This is responsible for producing an exact list of style attributes for every DOM element.

[This whole rambling on parallelizing this was from before the time that I
created the nuRenderStack. The parallelization discussion now belongs at a higher
level than just here]

I'm not sure how best to parallelize this. I suspect there is some well known algorithm
for parallelizing a tree/DAG computation such as this.

A very naive approach would be to simply process a node until you've finished X number
of its children, where X is some arbitrary constant such as 1000. Initially your queue
consists only of the root node. You would in this case limit the depth of each evaluation
to some other arbitrary constant such as 5. Whenever you hit a node that was at a depth
greater than your limit (our hypothetical 5), then you don't process that node, but 
instead add it to the queue.

However, it would probably be fast enough to do a first initial pass over the entire tree,
storing the accumulated number of children beneath each node. That would help you decide
which nodes to process in your queue. I'm pretty sure that the ideal queue would consist
initially of the root node, and then a further N nodes only, where N is your number of
threads.

NOTE: All of the protected functions assume that the style busy being resolved is on the top
of the stack. Our only public function "Resolve" adds this blank style to the top of the stack
before passing control to the other functions.
*/
class NUAPI nuStyleResolver
{
public:
	// Resolves the given node, and places its style on the top of the stack
	static void		ResolveAndPush( nuRenderStack& stack, const nuDomNode* node );

protected:
	static void		Set( nuRenderStack& stack, const nuDomEl* node, intp n, const nuStyleAttrib* vals );
	static void		Set( nuRenderStack& stack, const nuDomEl* node, const nuStyle& style );
	static void		SetInherited( nuRenderStack& stack, const nuDomEl* node, nuStyleCategories cat );
};