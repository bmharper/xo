
=begin

Shader Generator for xo
=======================

Input is pairs of .glsl files inside xo/Shaders, such as "RectFrag.glsl", "RectVert.glsl".
Output is 2 files per shader, such as "RectShader.cpp", "RectShader.h".

Eventually I'd like to incorporate this into the tundra build.

=end

CombinedBaseH_Start = <<-END
#pragma once
#if IF_BUILD

#include "../../Render/xoRenderGLDX_Defs.h"

class xoGLDXProg_NAME : public xoGLDXProg
{
public:
	xoGLDXProg_NAME();
	virtual void			Reset();
	virtual const char*		VertSrc();
	virtual const char*		FragSrc();
	virtual const char*		Name();
	virtual bool			LoadVariablePositions();	// Performs glGet[Uniform|Attrib]Location for all variables. Returns true if all variables are found.
	virtual uint32			PlatformMask();				// Combination of xoPlatform bits.
	virtual xoVertexType	VertexType();				// Only meaningful on DirectX

END

CombinedBaseH_End = <<-END
};

#endif // IF_BUILD
END

CombinedBaseCpp = <<-END
#include "pch.h"
#if IF_BUILD
#include "NAMEShader.h"

xoGLDXProg_NAME::xoGLDXProg_NAME()
{
	Reset();
}

void xoGLDXProg_NAME::Reset()
{
	ResetBase();
RESET
}

const char* xoGLDXProg_NAME::VertSrc()
{
	return
VERT_SRC;
}

const char* xoGLDXProg_NAME::FragSrc()
{
	return
FRAG_SRC;
}

const char* xoGLDXProg_NAME::Name()
{
	return "NAME";
}


bool xoGLDXProg_NAME::LoadVariablePositions()
{
	int nfail = 0;

LOAD_FUNC_BODY
	return nfail == 0;
}

uint32 xoGLDXProg_NAME::PlatformMask()
{
	return PLATFORM_MASK;
}

xoVertexType xoGLDXProg_NAME::VertexType()
{
	return VERTEX_TYPE;
}

#endif // IF_BUILD
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

def die(msg)
	put(msg + "\n")
	exit(1)
end

def name_from_shader_source(source, include_vert_or_frag = true)
	if source =~ /\/(\w+)_Vert\..lsl/
		return $1 + (include_vert_or_frag ? "_" + "Vert" : "")
	end
	if source =~ /\/(\w+)_Frag\..lsl/
		return $1 + (include_vert_or_frag ? "_" + "Frag" : "")
	end
	raise "Do not understand how to name the shader #{source}"
end

def ext2name(ext)
	return ext == "hlsl" ? "DX" : "GL"
end

def ext2namelong(ext)
	return ext == "hlsl" ? "DIRECTX" : "OPENGL"
end

def escape_txt(txt)
	cpp = ""
	txt.each_line { |line|
		cpp << "\t\t\"" + line.rstrip.gsub("\"", "\\\"") + "\\n\"\n"
	}
	return cpp
end

def extract_vertex_type(vert_src, name)
	# example:
	# VSOutput main(VertexType_PTCV4 vertex)
	vert_src.each_line{ |line|
		if line =~ /main\(VertexType_(\w+)/
			return "xoVertexType_" + $1
		end
	}
	die("Couldn't find vertex type for shader #{name}")
end

def gen_combined(common, ext, vert, frag, name, filename_base)
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

			if line =~ /#XO_PLATFORM_(\w+)/
				use_line = false
				platform = $1
				case platform
				when "WIN_DESKTOP" then platforms[:xoPlatform_WinDesktop] = 1
				when "LINUX_DESKTOP" then platforms[:xoPlatform_LinuxDesktop] = 1
				when "ANDROID" then platforms[:xoPlatform_Android] = 1
				else raise "Unrecognized platform #{platform}"
				end
			end

			cleaned_src << line if use_line
		}
		cleaned_src
	}
	vert_src, frag_src = replaced[0], replaced[1]
	vert_src = common + vert_src
	frag_src = common + frag_src

	vertex_type = "xoVertexType_NULL"
	vertex_type = extract_vertex_type(vert_src, name) if ext2name(ext) == "DX"

	platforms[:xoPlatform_All] = 1 if platforms.length == 0

	replace = lambda { |txt|
		rep = txt
		rep = rep.gsub("NAMEUC", name.upcase)
		rep = rep.gsub("NAME", name)
		rep = rep.gsub("GLDX", ext2name(ext))
		rep = rep.gsub("VERT_SRC", escape_txt(vert_src))
		rep = rep.gsub("FRAG_SRC", escape_txt(frag_src))
		rep = rep.gsub("IF_BUILD", "XO_BUILD_" + ext2namelong(ext))
		rep = rep.gsub("PLATFORM_MASK", platforms.keys.join(" | "))
		rep = rep.gsub("VERTEX_TYPE", vertex_type)
		return rep
	}

	File.open(filename_base + ".h", "w") { |file|
		txt = CombinedBaseH_Start + ""
		variables.each { |var|
			txt << "\tGLint v_#{var.name}; #{' ' * (30 - var.name.length)} // #{var.nature} #{var.type}\n"
		}
		txt << CombinedBaseH_End
		txt = replace.call(txt)
		file << txt
		file << "\n"
	}

	File.open(filename_base + ".cpp", "w") { |file|
		txt = CombinedBaseCpp + ""
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
		load_func_body << "\tif (nfail != 0)\n"
		load_func_body << "\t\tXOTRACE(\"Failed to bind %d variables of shader #{name}\\n\", nfail);\n"
		reset.rstrip!
		txt = replace.call(txt)
		txt.gsub!("RESET", reset)
		txt.gsub!("LOAD_FUNC_BODY", load_func_body)
		file << txt
		file << "\n"
	}
end

# vert and frag are paths to .hlsl/.glsl files
def gen_pair(common, base_dir, ext, vert, frag)
	name = name_from_shader_source(vert, false)
	gen_combined(common, ext, vert, frag, name, base_dir + "/Processed_#{ext}/#{name}Shader")
end

def run_dir(base_dir, ext)
	shaders = Dir.glob("#{base_dir}/*.#{ext}")
	#print(shaders)

	common = ""
	fcommon = "#{base_dir}/_Common.#{ext}"
	if File.exist?(fcommon)
		common = File.open(fcommon) { |file| file.read }
	end

	# At present we expect pairs of "xyzFrag.glsl" and "xyzVert.glsl" (or .hlsl)
	# Maybe someday we could pair them up differently.. but not THIS DAY
	shaders.each { |candidate|
		if candidate =~ /_Vert\.#{ext}/
			vert = candidate
			frag = candidate.sub("Vert.#{ext}", "Frag.#{ext}")
			#print(vert + " " + frag + "\n")
			gen_pair(common, base_dir, ext, vert, frag)
		end
	}
end

run_dir("xo/Shaders", "glsl")
run_dir("xo/Shaders", "hlsl")