#ifndef _ENGINE_COLLISION_UTILS_H_
#define _ENGINE_COLLISION_UTILS_H_
#pragma once

#include "../core/Vector.h"
#include "../core/Quaternion.h"

namespace CollisionUtils
{
	//\brief Intersection check between a line segment and a a plane of unlimited dimension
	//\param a_lineStart is the start of the line segment
	//\param a_lineEnd is the end of the line segment
	//\param a_planeCentre is a point on the plane
	//\param a_planeNormal is the normal of the plane
	//\param a_intersection_OUT is the point at which the plane intersects the line, if any
	//\return bool true if there is an intersection, false otherwise
	extern bool IntersectLinePlane(Vector a_lineStart, Vector a_lineEnd, Vector a_planeCentre, Vector a_planeNormal, Vector & a_intesection_OUT);
	
	//\brief Intersection check between a line segment and an axis aligned box
	//\param a_boxPos the middle of the box
	//\param a_boxDimensions the size of the box in x,y,z order
	//\param a_intersection_OUT is the point at which the line intersects the box, if any
	//\return bool true if the line touches the box
	extern bool IntersectLineAxisBox(Vector a_lineStart, Vector a_lineEnd, Vector a_boxPos, Vector a_boxDimensions, Vector & a_intersection_OUT);

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
	extern bool IntersectPointAxisBox(const Vector & a_point, const Vector & a_boxPos, const Vector & a_boxDimensions);

	//\brief Intersection between two axis aligned boxes
	//\param a_boxPos the middle of the first box
	//\param a_boxDimensions the size of the first box in x,y,z order
	//\param a_boxPos the middle of the secondbox
	//\param a_boxDimensions the size of the second box in x,y,z order
	//\return true if the boxes intersect at all
	extern bool IntersectAxisBoxes(const Vector & a_box1Pos, const Vector & a_box1Dim, const Vector & a_box2Pos, const Vector & a_box2Dim);

	//\brief Intersection between a point and a sphere
	//\param a_point worldpos vector of the point
	//\param a_spherePos the middle of the sphere
	//\param a_sphereRadius is the size of the sphere
	//\return true if the point is on or inside the box
	extern bool IntersectPointSphere(Vector a_point, Vector a_spherePos, float a_sphereRadius);

	//\brief Intersection between two spheres
	//\param a_bodyPos The position of a sphere in worldspace
	//\param a_bodyRadius The size of the sphere from centre to extent
	//\a_collisionPosition_OUT Output parameter of the worldspace position of the collision if there is one	
	//\a_collisionNormal_OUT Output parameter of the normalized direction of the collision if there is one
	//\return true if the two supplied spheres are touching and the a_normal_OUT was modified
	extern bool IntersectSpheres(const Vector & a_bodyAPos, float a_bodyARadius, const Vector & a_bodyBPos, float bodyBRadius, Vector & a_collisionPos_OUT, Vector & a_collisionNormal_OUT);

	//\brief Intersection between two boxes non axis aligned
	//\param a_boxPos The position of a box in worldspace
	//\param a_boxSize The size of the box in all three axis
	//\param a_boxRot THe orientation of the box
	//\a_collisionPosition_OUT Output parameter of the worldspace position of the collision if there is one	
	//\a_collisionNormal_OUT Output parameter of the normalized direction of the collision if there is one
	//\return true if the two supplied spheres are touching and the a_normal_OUT was modified
	extern bool IntersectBoxes(const Vector & a_boxAPos, const Vector & a_boxASize, const Quaternion & a_boxARot, const Vector & a_boxBPos, const Vector & a_boxBSize, const Quaternion & a_boxBRot, Vector & a_collisionPos_OUT, Vector & a_collisionNormal_OUT);

	//\brief Intersection between a box and a sphere
	//\param a_spherePos The position of a sphere in worldspace
	//\param a_sphereRadius The size of the sphere from centre to extent
	//\param a_boxPos The position of a box in worldspace
	//\param a_boxSize The size of the box in all three axis
	//\param a_boxRot THe orientation of the box
	//\a_collisionPosition_OUT Output parameter of the worldspace position of the collision if there is one	
	//\a_collisionNormal_OUT Output parameter of the normalized direction of the collision if there is one
	//\return true if the two supplied spheres are touching and the a_normal_OUT was modified
	extern bool IntersectBoxSphere(const Vector & a_spherePos, float a_sphereRadius, const Vector & a_boxPos, const Vector & a_boxSize, const Quaternion & a_boxRot, Vector & a_collisionPos_OUT, Vector & a_collisionNormal_OUT);
}

#endif /* _ENGINE_COLLISION_UTILS_H_ */
