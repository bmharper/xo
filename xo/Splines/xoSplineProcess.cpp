#include "pch.h"
#include "../xoDefs.h"
#include "xoSplineProcess.h"

// Test 'test', to see on which side of the line org->dir it lies.
// Returns +1,0,-1, depending on the side of the line that 'test' lies on.
static int SideOf(xoVec2f dir, xoVec2f org, xoVec2f test)
{
	return xoSign(dir.x * (test.x - org.x) + dir.y * (test.y - org.y));
}

bool xoGeomUtils::TriangleOverlapTest(const xoVec2f* t1, const xoVec2f* t2)
{
	if (FindSplittingAxisOnTriangle(t1, t2))
		return false;
	if (FindSplittingAxisOnTriangle(t2, t1))
		return false;
	return true;
}

bool xoGeomUtils::FindSplittingAxisOnTriangle(const xoVec2f* t1, const xoVec2f* t2)
{
	// I believe this is cheaper than mod3, but I haven't tested
	const int plus1[3] = { 1, 2, 0 };

	// Iterate over the edges of t1.
	int j = 3;
	for (int i = 0; i < 3; i++)
	{
		// Project the three points of t2 onto an edge that is perpendicular to this edge of t1.
		// We also project the 'other' point of t1. If t1 is on one side of the perpendicular edge,
		// and all the points of t2 are on the other side of the edge, then we have a separating side.

		xoVec2f side_dir = t1[i] - t1[j];
		side_dir = xoVec2f(-side_dir.y, side_dir.x);
		if (side_dir.x == 0 && side_dir.y == 0)
			continue;

		// Compute the side of the 'other' point of t1
		int side_k = SideOf(side_dir, t1[j], t1[plus1[i]]);

		int nleft = 0, nright = 0;
		for (int q = 0; q < 3; q++)
		{
			int side = SideOf(side_dir, t1[j], t2[q]);
			if (side <= 0) nleft++;
			if (side >= 0) nright++;
		}

		if (nleft == 3 && side_k >= 0) return true;
		if (nright == 3 && side_k <= 0) return true;

		j = i;
	}

	return false;
}
