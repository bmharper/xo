module(..., package.seeall)

function apply(env, options)
  tundra.unitgen.load_toolset("gcc", env)

  env:set_many {
    ["CC"] = "/home/ben/dev/afl-2.51b/afl-clang",
    ["CXX"] = "/home/ben/dev/afl-2.51b/afl-clang++",
    ["LD"] = "/home/ben/dev/afl-2.51b/afl-clang",
  }
end
