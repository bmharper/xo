require 'kramdown'

Dir.mkdir("html") if !Dir.exists?("html")

template = File.read("src/template.html")

pages = []

# Generate content pages
Dir.foreach("src") { |item|
	next if item == "." || item == ".."
	if item =~ /([^.]+)\.md/
		docname = $1
		infile = "src/" + item
		outfile = "html/" + docname + ".html"
		File.open(outfile, "wb") { |fout|
			inner_html = Kramdown::Document.new(File.read(infile), :input => 'markdown').to_html
			html = template.clone
			html.gsub!("$CONTENT", inner_html)
			fout << html
		}
		pages << docname
	end
}

# Generate index
html = template.clone
inner = "<h1>These docs were ad-hoc notes. Many are out of date or not implemented</h1>"
pages.each { |page|
	inner << "<p><a href='#{page}.html'>#{page}</a></p>\n"
}
html.gsub!("$CONTENT", inner)
File.open("html/index.html", "wb") { |fout|
	fout << html
}
