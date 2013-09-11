
=begin

Shader Generator for nuDom
==========================

Input is pairs of .glsl files inside nudom/Shaders, such as "RectFrag.glsl", "RectVert.glsl".
Output is 2 files per shader, such as "RectShader.cpp", "RectShader.h".

Eventually I'd like to incorporate this into the tundra build.

=end

def name_from_glsl(glsl, include_vert_or_frag = true)
	if glsl =~ /\/(\w+)Vert\.glsl/
		return $1 + (include_vert_or_frag ? "_" + "Vert" : "")
	end
	if glsl =~ /\/(\w+)Frag\.glsl/
		return $1 + (include_vert_or_frag ? "_" + "Frag" : "")
	end
	raise "Do not understand how to name the shader #{glsl}"
end

# This is no longer used
def gen_cpp(glsl)
	name = name_from_glsl(glsl)
	File.open(glsl.sub(".glsl", ".cpp"), "w") { |cpp|
		cpp << "\#include \"pch.h\"\n"
		cpp << "const char* NU_SHADER_#{name.upcase} = \n"
		File.open(glsl) { |src|
			src.each_line { |line|
				cpp << "\"" + line.rstrip + "\\n\"\n"
			}
		}
		cpp << ";"
		cpp << "\n"
	}
end

# This is no longer used
def gen_h(glsl)
	name = name_from_glsl(glsl)
	File.open(glsl.sub(".glsl", ".h"), "w") { |doth|
		doth << "\#pragma once\n"
		doth << "static const char* NU_SHADER_#{name.upcase};\n"
		doth << "\n"
	}
end

CombinedBaseH = <<-END
#pragma once

#include "../../Render/nuRenderGL_Defs.h"

class nuGLProg_NAME : public nuGLProg
{
public:
	nuGLProg_NAME();
	virtual void		Reset();
	virtual const char*	VertSrc();
	virtual const char*	FragSrc();
	virtual const char*	Name();
	virtual bool		LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32		PlatformMask();				// Combination of nuPlatform bits.

END

CombinedBaseCpp = <<-END
#include "pch.h"
#include "NAMEShader.h"

nuGLProg_NAME::nuGLProg_NAME()
{
	Reset();
}

void nuGLProg_NAME::Reset()
{
	ResetBase();
RESET
}

const char* nuGLProg_NAME::VertSrc()
{
	return
VERT_SRC;
}

const char* nuGLProg_NAME::FragSrc()
{
	return
FRAG_SRC;
}

const char* nuGLProg_NAME::Name()
{
	return "NAME";
}


bool nuGLProg_NAME::LoadVariablePositions()
{
	int nfail = 0;

LOAD_FUNC_BODY
	return nfail == 0;
}

uint32 nuGLProg_NAME::PlatformMask()
{
	return PLATFORM_MASK;
}

END

# nature:	uniform, attribute
# type:		vec2, vec3, vec4, mat2, mat3, mat4
class Variable
	attr_accessor :nature, :type, :name
	def initialize(_nature, _type, _name)
		@nature = _nature
		@type = _type
		@name = _name
	end
end

def escape_txt(txt)
	cpp = ""
	txt.each_line { |line|
		# The initial tab tends to preserve column formatting better
		cpp << "\"\t" + line.rstrip.gsub("\"", "\\\"") + "\\n\"\n"
	}
	return cpp
end

def escape_file(file)
	return escape_txt( File.open(file) { |f| f.read } )
end

def gen_combined(vert, frag, name, filename_base)
	variables = []
	platforms = {}
	vert_src = File.open(vert) { |f| f.read }
	frag_src = File.open(frag) { |f| f.read }

	replaced = [vert_src, frag_src].collect { |src|
		cleaned_src = ""
		src.each_line { |line|
			use_line = true
			# uniform mat4 mvproj;
			if line =~ /uniform\s+(\w+)\s+(\w+);/
				variables << Variable.new("uniform", $1, $2)
			elsif line =~ /attribute\s+(\w+)\s+(\w+);/
				variables << Variable.new("attribute", $1, $2)
			end

			if line =~ /#NU_PLATFORM_(\w+)/
				use_line = false
				platform = $1
				case platform
				when "WIN_DESKTOP" then platforms[:nuPlatform_WinDesktop] = 1
				when "ANDROID" then platforms[:nuPlatform_Android] = 1
				else raise "Unrecognized platform #{platform}"
				end
			end

			cleaned_src << line if use_line
		}
		cleaned_src
	}
	vert_src, frag_src = replaced[0], replaced[1]

	platforms[:nuPlatform_All] = 1 if platforms.length == 0

	File.open(filename_base + ".h", "w") { |file|
		txt = CombinedBaseH + ""
		txt.gsub!("NAMEUC", name.upcase)
		txt.gsub!("NAME", name)
		variables.each { |var|
			txt << "\tGLint v_#{var.name}; #{' ' * (15 - var.name.length)} // #{var.nature} #{var.type}\n"
		}
		txt << "};\n"
		file << txt
		file << "\n"
	}

	File.open(filename_base + ".cpp", "w") { |file|
		txt = CombinedBaseCpp + ""
		txt.gsub!("NAMEUC", name.upcase)
		txt.gsub!("NAME", name)
		txt.gsub!("VERT_SRC", escape_txt(vert_src))
		txt.gsub!("FRAG_SRC", escape_txt(frag_src))
		load_func_body = ""
		reset = ""
		variables.each { |var|
			reset << "\tv_#{var.name} = -1;\n"
			if var.nature == "uniform"
				load_func_body << "\tnfail += (v_#{var.name} = glGetUniformLocation( Prog, \"#{var.name}\" )) == -1;\n"
			elsif var.nature == "attribute"
				load_func_body << "\tnfail += (v_#{var.name} = glGetAttribLocation( Prog, \"#{var.name}\" )) == -1;\n"
			else
				raise "Unrecognized variable type #{var.nature}"
			end
		}
		load_func_body << "\tif ( nfail != 0 )\n"
		load_func_body << "\t\tNUTRACE( \"Failed to bind %d variables of shader #{name}\\n\", nfail );\n"
		reset.rstrip!
		txt.gsub!("RESET", reset)
		txt.gsub!("LOAD_FUNC_BODY", load_func_body)
		txt.gsub!("PLATFORM_MASK", platforms.keys.join(" | "))
		file << txt
		file << "\n"
	}
end

# vert and frag are paths to .glsl files
def gen_pair(base_dir, vert, frag)
	name = name_from_glsl(vert, false)
	#gen_cpp(vert)
	#gen_cpp(frag)
	#gen_h(vert)
	#gen_h(frag)
	gen_combined(vert, frag, name, base_dir + "/Processed/#{name}Shader")
end

base_dir = "nudom/Shaders"
shaders = Dir.glob("#{base_dir}/*.glsl")
#print(shaders)

# At present we expect pairs of "xyzFrag.glsl" and "xyzVert.glsl"
# Maybe someday we could pair them up differently.. but this THIS DAY
shaders.each { |candidate|
	if candidate =~ /Vert\.glsl/
		vert = candidate
		frag = candidate.sub("Vert.glsl", "Frag.glsl")
		#print(vert + " " + frag + "\n")
		gen_pair(base_dir, vert, frag)
	end
}
