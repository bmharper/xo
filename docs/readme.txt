Run gen.rb to generate the docs inside 'html', from the content inside 'src'.
You will need to 'gem install kramdown' in order to run gen.rb.

I'm still not 100% sold on this technique. What would be cleaner is a 100%
browser solution, where you fetch the markdown in the browser, parse it,
and output HTML. Very similar to flatdoc, but not just a single page.