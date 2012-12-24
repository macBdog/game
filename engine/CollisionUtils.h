#ifndef _ENGINE_COLLISION_UTILS_H_
#define _ENGINE_COLLISION_UTILS_H_
#pragma once

#include "../core/Vector.h"

namespace CollisionUtils
{
	//\brief Intersection check between a line segment and an axis aligned box
	//\param a_boxPos the middle of the box
	//\param a_boxDimensions the size of the box in x,y,z order
	//\return true if the line touches the box
	extern bool IntersectLineAABB(Vector a_lineStart, Vector a_lineEnd, Vector a_boxPos, Vector a_boxDimensions);

	//\brief Intersection check between a line and a sphere
	//\param a_lineStart is the start of the line segment
	//\param a_lineEnd is the end of the line segment
	//\param a_shperePos is the sphere's centroid in world space
	//\param a_sphereRadius is the size of the sphere
	//\return True if and part of the line is on or inside the sphere
	extern bool IntersectLineSphere(Vector a_lineStart, Vector a_lineEnd, Vector a_spherePos, float a_sphereRadius);

	//\brief Intersection between a point and an axis aligned box
	//\param a_point worldpos vector of the point
	//\param a_boxPos the middle of the box
	//\param a_boxDimensions the size of the box in x,y,z order
	//\return true if the point is on or inside the box
	extern bool IntersectPointAABB(Vector a_point, Vector a_boxPos, Vector a_boxDimensions);

	//\brief Intersection between a point and a sphere
	//\param a_point worldpos vector of the point
	//\param a_spherePos the middle of the sphere
	//\param a_sphereRadius is the size of the sphere
	//\return true if the point is on or inside the box
	extern bool IntersectPointSphere(Vector a_point, Vector a_spherePos, float a_sphereRadius);
}

#endif /* _ENGINE_COLLISION_UTILS_H_ */
