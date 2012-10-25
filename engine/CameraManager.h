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
        inline Vector GetCameraPos() { return m_pos; }

private:

		//\brief Update the camera matrix from the inputs
        void CalculateCameraMatrix();

		static const float sc_debugCameraSpeed;	///< Debug camera translation constant
		static const float sc_debugCameraRot;	///< Debug camera rotation max

        Matrix m_mat;                   ///< The matrix defining the new coordinate system as calculated by the camera
        Vector m_pos;                   ///< Position of the camera in the world
        Vector2 m_orientation;			///< Angle of the camera around it's view axis
		Vector2 m_orientationInput;		///< Input to orientation given mouse corrds
};

#endif //_ENGINE_CAMERA_MANAGER