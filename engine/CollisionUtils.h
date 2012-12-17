#ifndef _ENGINE_COLLISION_UTILS_H_
#define _ENGINE_COLLISION_UTILS_H_
#pragma once

#include "../core/Vector.h"

namespace CollisionUtils
{
	//\brief Intersection check between a line and a sphere
	//\param a_lineStart is the start of the line segment
	//\param a_lineEnd is the end of the line segment
	//\param a_shperePos is the sphere's centroid in world space
	//\param a_sphereRadius is the size of the sphere
	//\return True if and part of the line is on or inside the sphere
	extern bool IntersectLineSphere(Vector a_lineStart, Vector a_lineEnd, Vector a_spherePos, float a_sphereRadius);
	
	//\brief Intersection check between a line and a box with equal sides
	//\param a_lineStart is the start of the line segment
	//\param a_lineEnd is the end of the line segment
	//\param a_cubePos is the cube's centroid in world space
	//\param a_cubeSize is the length of one edge of the cube
	extern bool IntersectLineCube(Vector a_lineStart, Vector a_lineEnd, Vector a_cubePos, float a_cubeSize);

	//\brief Stubbed out for future implementation
	extern bool IntersectLineAABB(Vector a_lineStart, Vector a_lineEnd);
}

#endif /* _ENGINE_COLLISION_UTILS_H_ */
