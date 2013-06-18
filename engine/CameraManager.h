#ifndef _ENGINE_CAMERA_MANAGER
#define _ENGINE_CAMERA_MANAGER
#pragma once

#include "..\core\Matrix.h"
#include "..\core\Vector.h"

#include "Singleton.h"

class CameraManager : public Singleton<CameraManager>
{

public:

        //\brief Contruct a valid camera at the origin look down the Y axis
        CameraManager() 
                : m_pos(Vector::Zero())
                , m_orientation(0.0f)
        {
                CalculateCameraMatrix();
        }

        //\brief Reset the camera based on a new look at target
        void LookAt(Vector a_worldPos);

		//\brief Stubbed out for loading cameras for each scene
		void Startup() {};
		void Shutdown() {};
        void Update(float a_dt);

        //\brief Accessor for rendering 
        inline Matrix GetCameraMatrix() { return m_mat; }
		inline Matrix GetViewMatrix() { return m_viewMat; }
		inline Vector GetWorldPos() { return Vector(-m_pos.GetX(), -m_pos.GetY(), -m_pos.GetZ()); }

		void GetInverseMat(Matrix & a_mat_OUT);

private:

		//\brief Update the camera matrix from the inputs
        void CalculateCameraMatrix();

		static const float sc_debugCameraSpeed;		///< Debug camera translation constant
		static const float sc_debugCameraRot;		///< Debug camera rotation max degrees
		static const float sc_debugCameraRotSpeed;	///< Debug camera rotation speed degrees per second

        Matrix m_mat;                   ///< The matrix defining the new coordinate system as calculated by the camera
		Matrix m_viewMat;				///< The matrix defining the direction the camera is looking
        Vector m_pos;                   ///< Position of the camera in the world
        Vector2 m_orientation;			///< Angle of the camera around it's view axis
		Vector2 m_orientationInput;		///< Input to orientation given mouse corrds
};

#endif //_ENGINE_CAMERA_MANAGER