#include "../core/MathUtils.h"

#include "CollisionUtils.h"

extern bool CollisionUtils::IntersectLinePlane(Vector a_lineStart, Vector a_lineEnd, Vector a_planeCentre, Vector a_planeNormal, Vector & a_intesection_OUT)
{
	// Check the line and plane aren't parallel
	Vector line = a_lineEnd - a_lineStart;
	if (a_planeNormal.LengthSquared() == 1.0f && 
		!MathUtils::IsZeroEpsilon(line.Dot(a_planeNormal)))
	{	
		float intersectAmount = (a_planeNormal.Dot(a_planeCentre - a_lineStart)) / (a_planeNormal.Dot(a_lineEnd - a_lineStart));
		a_intesection_OUT = a_lineStart + (line * intersectAmount);
		return true;
	}
	else
	{
		return false;
	}
}

extern bool CollisionUtils::IntersectLineAxisBox(Vector a_lineStart, Vector a_lineEnd, Vector a_boxPos, Vector a_boxDimensions, Vector & a_intersection_OUT)
{
	// Check the dimensions of the line against the box
	const Vector halfDim = a_boxDimensions * 0.5f;

	// Check the front plane of the box first
	Vector planeCentre = Vector(a_boxPos.GetX(), a_boxPos.GetY() - halfDim.GetY(), a_boxPos.GetZ());
	if (IntersectLinePlane(a_lineStart, a_lineEnd, planeCentre, Vector(0.0f, -1.0f, 0.0f), a_intersection_OUT))
	{
		if (fabsf(a_intersection_OUT.GetX() - planeCentre.GetX()) <= halfDim.GetX() &&
			fabsf(a_intersection_OUT.GetY() - planeCentre.GetY()) <= halfDim.GetY() &&
			fabsf(a_intersection_OUT.GetZ() - planeCentre.GetZ()) <= halfDim.GetZ())
		{
			return true;
		}
	}

	// Check back plane
	planeCentre = Vector(a_boxPos.GetX(), a_boxPos.GetY() + halfDim.GetY(), a_boxPos.GetZ());
	if (IntersectLinePlane(a_lineStart, a_lineEnd, planeCentre, Vector(0.0f, 1.0f, 0.0f), a_intersection_OUT))
	{
		if (fabsf(a_intersection_OUT.GetX() - planeCentre.GetX()) <= halfDim.GetX() &&
			fabsf(a_intersection_OUT.GetY() - planeCentre.GetY()) <= halfDim.GetY() &&
			fabsf(a_intersection_OUT.GetZ() - planeCentre.GetZ()) <= halfDim.GetZ())
		{
			return true;
		}
	}

	// TODO Check all other planes

	return false;
}

extern bool CollisionUtils::IntersectLineSphere(Vector a_lineStart, Vector a_lineEnd, Vector a_spherePos, float a_sphereRadius)
{
	// TODO actually implement this
	return false;
}

extern bool CollisionUtils::IntersectPointAxisBox(Vector a_point, Vector a_boxPos, Vector a_boxDimensions)
{
	const Vector halfDim = a_boxDimensions * 0.5f;
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