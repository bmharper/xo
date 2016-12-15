#pragma once
namespace xo {

class GeomUtils {
public:
	// Return true if the triangles overlap. Return false if the triangles merely touch.
	// THIS IS COMPLETELY UNTESTED
	bool TriangleOverlapTest(const Vec2f* t1, const Vec2f* t2);

private:
	// Returns true if we can find a splitting axis on t1
	bool FindSplittingAxisOnTriangle(const Vec2f* t1, const Vec2f* t2);
};
}
