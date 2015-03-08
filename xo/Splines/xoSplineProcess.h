#pragma once

class xoGeomUtils
{
public:
	// Return true if the triangles overlap. Return false if the triangles merely touch.
	// THIS IS COMPLETELY UNTESTED
	bool TriangleOverlapTest(const xoVec2f* t1, const xoVec2f* t2);

private:
	// Returns true if we can find a splitting axis on t1
	bool FindSplittingAxisOnTriangle(const xoVec2f* t1, const xoVec2f* t2);
};
