require 'json'

out = File.open( ".tundra2.dag.cleaned.json", "wb" )

File.open( ".tundra2.dag.json", "rb" ) { |f|
	all = JSON.parse(f.read)
	out.write(JSON.generate(all, { :indent => " ", :space => " ", :object_nl => "\n"}) )
}

out.close