#pragma once

// warnings.h must be outside of our push/pop, because it's design is to permanently
// turn off certain static analysis warnings
#include "warnings.h"

#pragma warning( push )
#pragma warning( disable: 4251 ) // template class needs dll-interface
#pragma warning( disable: 4244 ) // loss of precision

#include "lmTypes.h"

#include "aligned_malloc.h"
#include "../Strings/stringutil.h"

#include "lmPlatform.h"
#include "Random.h"
#include "randomlib.h"
#include "../Strings/ConvertUTF.h"
#include "Guid.h"

#include "ScopeOverrider.h"

#include "ProgMon.h"

#include "../HashCrypt/CRC32.h"

// vector math
#include "../Vec/VecPrim.h"
#include "../Vec/vec2.h"
#include "../Vec/vec3.h"
#include "../Vec/vec4.h"
#include "../Vec/mat3.h"
#include "../Vec/mat5.h"

#include "../Vec/Ray3.h"
#include "../Vec/Ray4.h"

typedef dvect< Vec2f >	Vec2fVect;
typedef dvect< Vec2 >	Vec2Vect;
typedef dvect< Vec3f >	Vec3fVect;
typedef dvect< Vec3 >	Vec3Vect;

#include "../Vec/Mat4.h"
#include "../Vec/Quat.h"

#include "../Vec/vectGen.h"
#include "../HashTab/nicemaps.h"

#include "../IO/VirtualFile.h"

#include "../Vec/Rect2I.h"
#include "../Vec/Bounds2.h"
#include "../Vec/Bounds3.h"
#include "../Trees/Bsp2.h"

#include "../Geom/geom2d.h"
#include "../Geom/geom3d.h"
#include "../Geom/geomNd.h"

#include "../Bits/BitMap.h"

// array accessible vectors
#include "../Containers/dvec.h"
// typed pointer vector wrapper, with 0% code bloat
#include "../Containers/pvect.h"

#include "../Containers/podvec.h"

#include "../HashCrypt/md5.h"
#include "../HashCrypt/sha1.h"
#include "../HashCrypt/sha2.h"

#include "../Containers/pvec2D.h"

#include "../Containers/SortedVector.h"

#include "../IO/PackFile.h"

#include "SimpleLog.h"

#include "Mesh.h"

// cheap n^3 polygon triangulation
#include "../Geom/triangulate.h"
#include "stats.h"
#include "../Strings/strings.h"
#include "misc.h"
#include "color.h"
#include "GlobalCache.h"
#include "../modp/src/modp_ascii.h"
#include "../modp/src/modp_numtoa.h"
#include "../modp/src/modp_burl.h"
#include "StackAllocators.h"
#include "Log.h"
#include "profile.h"
#include "../Trees/StaticTree.h"

#include "JobQueue.h"
#include "../platform/net.h"
#include "../platform/syncprims.h"
#include "../platform/timeprims.h"
#include "../IO/BackgroundFileLoader.h"

#pragma warning( pop )
