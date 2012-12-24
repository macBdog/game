#include "CollisionUtils.h"

extern bool CollisionUtils::IntersectLineAABB(Vector a_lineStart, Vector a_lineEnd, Vector a_boxPos, Vector a_boxDimensions)
{
	// TODO actually implement this
	return IntersectPointAABB(a_lineEnd, a_boxPos, a_boxDimensions);
}

extern bool CollisionUtils::IntersectLineSphere(Vector a_lineStart, Vector a_lineEnd, Vector a_spherePos, float a_sphereRadius)
{
	// TODO actually implement this
	return false;
}

extern bool CollisionUtils::IntersectPointAABB(Vector a_point, Vector a_boxPos, Vector a_boxDimensions)
{
	Vector halfDim = a_boxDimensions * 0.5f;
	if (a_point.GetX() >= a_boxPos.GetX() - halfDim.GetX() && 
		a_point.GetX() <= a_boxPos.GetX() + halfDim.GetX() &&
		a_point.GetY() >= a_boxPos.GetY() - halfDim.GetY() && 
		a_point.GetY() <= a_boxPos.GetY() + halfDim.GetY() &&
		a_point.GetZ() >= a_boxPos.GetZ() - halfDim.GetZ() && 
		a_point.GetZ() <= a_boxPos.GetZ() + halfDim.GetZ())
	{
		return true;
	}

	return false;
}

extern bool CollisionUtils::IntersectPointSphere(Vector a_point, Vector a_spherePos, float a_sphereRadius)
{
	return (a_point - a_spherePos).LengthSquared() <= (a_sphereRadius * a_sphereRadius);
}