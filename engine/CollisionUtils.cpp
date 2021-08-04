#include "../core/MathUtils.h"

#include "CollisionUtils.h"

bool CollisionUtils::IntersectLinePlane(Vector a_lineStart, Vector a_lineEnd, Vector a_planeCentre, Vector a_planeNormal, Vector & a_intesection_OUT)
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

bool CollisionUtils::IntersectLineAxisBox(Vector a_lineStart, Vector a_lineEnd, Vector a_boxPos, Vector a_boxDimensions, Vector & a_intersection_OUT)
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

bool CollisionUtils::IntersectLineSphere(Vector a_lineStart, Vector a_lineEnd, Vector a_spherePos, float a_sphereRadius)
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
	return distanceToSphereSq <= sphRadSq;
}

bool CollisionUtils::IntersectPointAxisBox(const Vector & a_point, const Vector & a_boxPos, const Vector & a_boxDimensions)
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

bool CollisionUtils::IntersectAxisBoxes(const Vector & a_box1Pos, const Vector & a_box1Dim, const Vector & a_box2Pos, const Vector & a_box2Dim)
{
	// TODO: Fix this, for ALL intersection cases
	// Exhaustively test each corner of the game object passed in against our volume
	Vector halfSize = a_box1Dim * 0.5f;
	Vector corners[8] = {	a_box1Pos + Vector(halfSize.GetX(), halfSize.GetY(), halfSize.GetZ()), 
							a_box1Pos + Vector(halfSize.GetX(), -halfSize.GetY(), halfSize.GetZ()), 
							a_box1Pos + Vector(-halfSize.GetX(), halfSize.GetY(), halfSize.GetZ()), 
							a_box1Pos + Vector(-halfSize.GetX(), -halfSize.GetY(), halfSize.GetZ()), 
							a_box1Pos + Vector(halfSize.GetX(), halfSize.GetY(), -halfSize.GetZ()), 
							a_box1Pos + Vector(halfSize.GetX(), -halfSize.GetY(), -halfSize.GetZ()), 
							a_box1Pos + Vector(-halfSize.GetX(), halfSize.GetY(), -halfSize.GetZ()), 
							a_box1Pos + Vector(-halfSize.GetX(), -halfSize.GetY(), -halfSize.GetZ())};
	for (int i = 0; i < 8; ++i)
	{
		if (CollisionUtils::IntersectPointAxisBox(corners[i], a_box2Pos, a_box2Dim))
		{
			return true;
		}
	}
	return false;
}

bool CollisionUtils::IntersectPointSphere(Vector a_point, Vector a_spherePos, float a_sphereRadius)
{
	return (a_point - a_spherePos).LengthSquared() <= (a_sphereRadius * a_sphereRadius);
}

bool CollisionUtils::IntersectSpheres(const Vector & a_bodyAPos, float a_bodyARadius, const Vector & a_bodyBPos, float a_bodyBRadius, Vector & a_collisionPos_OUT, Vector & a_collisionNormal_OUT)
{
	Vector betweenSpheres = a_bodyBPos - a_bodyAPos;
	float distBetween = betweenSpheres.Length();
	if (distBetween <= a_bodyARadius + a_bodyBRadius && distBetween >= fabs(a_bodyARadius - a_bodyBRadius))
	{
		betweenSpheres.Normalise();
		a_collisionNormal_OUT = betweenSpheres;

		// Get the circle that defines the intersection between the spheres, could stop here
		float centreOfIntersection = 1.0f / 2.0f + (a_bodyARadius * a_bodyARadius - a_bodyBRadius * a_bodyBRadius) / (2 * distBetween * distBetween);
		float radiusOfIntersection = sqrt(a_bodyARadius*a_bodyARadius - centreOfIntersection * centreOfIntersection * distBetween * distBetween);

		// Or go on to find an arbitrary but coplanar point on this cicle. 
		a_collisionPos_OUT = a_bodyAPos + (a_collisionNormal_OUT * centreOfIntersection);
		return true;
	}
	return false;
}

bool CollisionUtils::IntersectBoxes(const Vector & a_boxAPos, const Vector & a_boxASize, const Quaternion & a_boxARot, const Vector & a_boxBPos, const Vector & a_boxBSize, const Quaternion & a_boxBRot, Vector & a_collisionPos_OUT, Vector & a_collisionNormal_OUT)
{
	// TODO
	return false;
}

bool CollisionUtils::IntersectBoxSphere(const Vector & a_spherePos, float a_sphereRadius, const Vector & a_boxPos, const Vector & a_boxSize, const Quaternion & a_boxRot, Vector & a_collisionPos_OUT, Vector & a_collisionNormal_OUT)
{
	// TODO
	return false;
}

bool CollisionUtils::IntersectAxisBoxSphere(const Vector& a_spherePos, float a_sphereRadius, const Vector& a_boxPos, const Vector& a_boxSize, Vector& a_collisionPos_OUT, Vector& a_collisionNormal_OUT)
{
	// Get box closest point to sphere center by clamping
	const float boxMinX = a_boxPos.GetX() - a_boxSize.GetX();
	const float boxMaxX = a_boxPos.GetX() + a_boxSize.GetX();
	const float boxMinY = a_boxPos.GetY() - a_boxSize.GetY();
	const float boxMaxY = a_boxPos.GetY() + a_boxSize.GetY();
	const float boxMinZ = a_boxPos.GetZ() - a_boxSize.GetZ();
	const float boxMaxZ = a_boxPos.GetZ() + a_boxSize.GetZ();
	const float x = MathUtils::Clamp(boxMinX, a_spherePos.GetX(), boxMaxX);
	const float y = MathUtils::Clamp(boxMinY, a_spherePos.GetY(), boxMaxY);
	const float z = MathUtils::Clamp(boxMinZ, a_spherePos.GetZ(), boxMaxZ);

	// Check point inside sphere
	const float distance = sqrtf((x - a_spherePos.GetX()) * (x - a_spherePos.GetX()) +
		(y - a_spherePos.GetY()) * (y - a_spherePos.GetY()) +
		(z - a_spherePos.GetZ()) * (z - a_spherePos.GetZ()));

	if (distance <= a_sphereRadius)
	{
		a_collisionPos_OUT = Vector(x, y, z);
		a_collisionNormal_OUT = a_spherePos - a_collisionPos_OUT;
		a_collisionNormal_OUT.Normalise();
		return true;
	}
	return false;
}
