#pragma once

#include "../nuDoc.h"
#include "nuRenderDomEl.h"

/* This is responsible for producing an exact list of style attributes for every DOM element.

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

*/
class NUAPI nuStyleResolver
{
public:
	static const int				NumInherited = 1;
	static const nuStyleCategories	Inherited[NumInherited];	// Styles that are inherited by default

	void		Resolve( nuDoc* doc, nuRenderDomEl* root, nuPool* pool );

protected:
	nuDoc*		Doc;
	nuPool*		Pool;

	void		ResolveNode( nuRenderDomEl* parentRNode, nuRenderDomEl* rnode );
	void		Set( nuRenderDomEl* parentRNode, nuRenderDomEl* rnode, int n, const nuStyleAttrib* vals );
	void		Set( nuRenderDomEl* parentRNode, nuRenderDomEl* rnode, const nuStyle& style );

};