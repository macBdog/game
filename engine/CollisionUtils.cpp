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

	// Check left plane
	planeCentre = Vector(a_boxPos.GetX() - halfDim.GetX(), a_boxPos.GetY(), a_boxPos.GetZ());
	if (IntersectLinePlane(a_lineStart, a_lineEnd, planeCentre, Vector(-1.0f, 0.0f, 0.0f), a_intersection_OUT))
	{
		if (fabsf(a_intersection_OUT.GetX() - planeCentre.GetX()) <= halfDim.GetX() &&
			fabsf(a_intersection_OUT.GetY() - planeCentre.GetY()) <= halfDim.GetY() &&
			fabsf(a_intersection_OUT.GetZ() - planeCentre.GetZ()) <= halfDim.GetZ())
		{
			return true;
		}
	}

	// Check right plane
	planeCentre = Vector(a_boxPos.GetX() + halfDim.GetX(), a_boxPos.GetY(), a_boxPos.GetZ());
	if (IntersectLinePlane(a_lineStart, a_lineEnd, planeCentre, Vector(1.0f, 0.0f, 0.0f), a_intersection_OUT))
	{
		if (fabsf(a_intersection_OUT.GetX() - planeCentre.GetX()) <= halfDim.GetX() &&
			fabsf(a_intersection_OUT.GetY() - planeCentre.GetY()) <= halfDim.GetY() &&
			fabsf(a_intersection_OUT.GetZ() - planeCentre.GetZ()) <= halfDim.GetZ())
		{
			return true;
		}
	}

	// Check top plane
	planeCentre = Vector(a_boxPos.GetX(), a_boxPos.GetY(), a_boxPos.GetZ() + halfDim.GetZ());
	if (IntersectLinePlane(a_lineStart, a_lineEnd, planeCentre, Vector(0.0f, 0.0f, 1.0f), a_intersection_OUT))
	{
		if (fabsf(a_intersection_OUT.GetX() - planeCentre.GetX()) <= halfDim.GetX() &&
			fabsf(a_intersection_OUT.GetY() - planeCentre.GetY()) <= halfDim.GetY() &&
			fabsf(a_intersection_OUT.GetZ() - planeCentre.GetZ()) <= halfDim.GetZ())
		{
			return true;
		}
	}

	// Check bottom plane
	planeCentre = Vector(a_boxPos.GetX(), a_boxPos.GetY(), a_boxPos.GetZ() - halfDim.GetZ());
	if (IntersectLinePlane(a_lineStart, a_lineEnd, planeCentre, Vector(0.0f, 0.0f, -1.0f), a_intersection_OUT))
	{
		if (fabsf(a_intersection_OUT.GetX() - planeCentre.GetX()) <= halfDim.GetX() &&
			fabsf(a_intersection_OUT.GetY() - planeCentre.GetY()) <= halfDim.GetY() &&
			fabsf(a_intersection_OUT.GetZ() - planeCentre.GetZ()) <= halfDim.GetZ())
		{
			return true;
		}
	}

	return false;
}

extern bool CollisionUtils::IntersectLineSphere(Vector a_lineStart, Vector a_lineEnd, Vector a_spherePos, float a_sphereRadius)
{
	// Early out if the start or end or the line segment is inside the radius
	const float sphRadSq = a_sphereRadius * a_sphereRadius;
	const Vector lineSeg = a_lineEnd - a_lineStart;
	if ((a_spherePos - a_lineStart).LengthSquared() <= sphRadSq)
	{
		return true;
	}
	if ((a_spherePos - a_lineEnd).LengthSquared() <= sphRadSq)
	{
		return true;	
	}

	float proj = (a_spherePos - a_lineStart).Dot(lineSeg) / lineSeg.Dot(lineSeg);
	float distanceToSphereSq = 0.0f;
	if(proj <= 0.0f)    // closest point on the line segment is the tailPoint
	{
	   distanceToSphereSq = (a_spherePos - a_lineEnd).LengthSquared();
	}
	else if(proj >= 1.0f)    // closest point on the line segment is the headPoint
	{
		distanceToSphereSq = (a_lineStart - a_spherePos).LengthSquared();
	}
	else    // closest point lies on the line segment itself
	{
		Vector closestPoint = a_lineStart + (lineSeg*proj);
		distanceToSphereSq = (closestPoint - a_spherePos).LengthSquared(); 
	}
	return distanceToSphereSq <= a_sphereRadius;
}

extern bool CollisionUtils::IntersectPointAxisBox(const Vector & a_point, const Vector & a_boxPos, const Vector & a_boxDimensions)
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