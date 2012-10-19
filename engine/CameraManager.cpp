#include <SDL.h>

#include "..\core\MathUtils.h"

#include "CameraManager.h"
#include "InputManager.h"

const float CameraManager::sc_debugCameraSpeed = 1.0f;
const float CameraManager::sc_debugCameraRot = 360.0f;

template<> CameraManager * Singleton<CameraManager>::s_instance = NULL;

void CameraManager::Update(float a_dt)
{
		// Process debug camera controls
		InputManager & inMan = InputManager::Get();
		if (inMan.IsKeyDepressed(SDLK_w))
		{
			m_pos += Vector(0.0f, sc_debugCameraSpeed * a_dt, 0.0f);
		}	
		if (inMan.IsKeyDepressed(SDLK_s))
		{
			m_pos += Vector(0.0f, -sc_debugCameraSpeed * a_dt, 0.0f);
		}	
		if (inMan.IsKeyDepressed(SDLK_a))
		{
			m_pos += Vector(sc_debugCameraSpeed * a_dt, 0.0f, 0.0f);
		}	
		if (inMan.IsKeyDepressed(SDLK_d))
		{
			m_pos += Vector(-sc_debugCameraSpeed * a_dt, 0.0f, 0.0f);
		}	

        // Set rotation speed
        m_orientation.SetX(m_orientation.GetX() + (sc_debugCameraRot * a_dt * inMan.GetMousePosRelative().GetX()));
        m_orientation.SetY(m_orientation.GetY() + (sc_debugCameraRot * a_dt * inMan.GetMousePosRelative().GetY()));

        CalculateCameraMatrix();
}

void CameraManager::CalculateCameraMatrix()
{
        // Create a right vector orthoganol to the look
        Vector right = Vector(1.0f, 0.0f, 0.0f);
        right.Normalize();

        // Calculate look direction
        Vector forward = Vector(0.0f, 1.0f, 0.0f);
        forward.Normalize();

        // Make sure the up vector is colinear
        Vector up = Vector(0.0f, 0.0f, 1.0f);
        up.Normalize();

        // Create the matrix
        m_mat.SetRight(right);
        m_mat.SetLook(forward);
        m_mat.SetUp(up);
        m_mat.SetPos(m_pos);

        // Rotate the matrix about two axis defined by the mouse coords
        float angleX = (PI * 0.5f) + m_orientation.GetY() * 0.01f;
        float angleY = m_orientation.GetX() * 0.01f;

        m_mat = m_mat.Multiply(Matrix::GetRotateZ(-angleY));
        m_mat = m_mat.Multiply(Matrix::GetRotateX(-angleX));
}
