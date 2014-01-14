#include <SDL.h>

#include "..\core\MathUtils.h"

#include "CameraManager.h"
#include "InputManager.h"

const float Camera::sc_defaultCameraSpeed = 24.0f;
const float Camera::sc_defaultCameraRotSpeed = 0.01f;

template<> CameraManager * Singleton<CameraManager>::s_instance = NULL;

void CameraManager::Update(float a_dt)
{
	m_currentCamera = &m_gameCamera;

	// Process camera controls
	InputManager & inMan = InputManager::Get();
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		m_currentCamera = &m_debugCamera;
	
		// Create a view direction matrix
		Matrix viewMat = Matrix::Identity();
		Vector camPos = m_currentCamera->GetPosition();
		const float speed = m_currentCamera->GetTranslationSpeed();
		const float rotSpeed = m_currentCamera->GetRotationSpeed();
		viewMat = viewMat.Multiply(Matrix::GetRotateX(m_currentCamera->GetOrientation().GetY() * m_currentCamera->GetRotationSpeed()));
		viewMat = viewMat.Multiply(Matrix::GetRotateZ(m_currentCamera->GetOrientation().GetX() * m_currentCamera->GetRotationSpeed()));

		// Don't change camera while there is a dialog up
		if (!DebugMenu::Get().IsDebugMenuActive())
		{
			// WSAD for FPS style movement
			if (inMan.IsKeyDepressed(SDLK_w)) {	camPos -= viewMat.GetLook() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_s)) {	camPos += viewMat.GetLook() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_d)) { camPos -= viewMat.GetRight() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_a)) {	camPos += viewMat.GetRight() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_q)) {	camPos -= Vector(0.0f, 0.0f, speed * a_dt); }
			if (inMan.IsKeyDepressed(SDLK_e)) { camPos += Vector(0.0f, 0.0f, speed * a_dt); }	
		

			m_currentCamera->SetPos(camPos);

			// Set rotation based on mouse delta while hotkey pressed
			if (inMan.IsKeyDepressed(SDLK_LSHIFT))
			{
				// Get current camera inputs
				const float maxRot = 360.0f;
				Vector2 curInput = inMan.GetMousePosRelative();
				Vector2 orient = m_currentCamera->GetOrientation();
				Vector2 orientInput = m_currentCamera->GetOrientationInput();

				// Cancel out mouse movement above an epsilon to prevent the camera jumping around
				const float cameraMoveEpsilonSq = 0.05f;
				if ((orientInput - curInput).LengthSquared() > cameraMoveEpsilonSq)
				{
					m_currentCamera->SetOrientationInput(curInput);
				}
				orient += (curInput - orientInput) * maxRot;
				m_currentCamera->SetOrientation(orient);
				m_currentCamera->SetOrientationInput(curInput);
			}
		}
	}

	m_currentCamera->Update();
}

void Camera::Update()
{
    // Create the matrix
	m_mat = Matrix::Identity();
	m_mat.SetPos(m_pos);

    // Rotate the matrix about two axis defined by the mouse coords
    float angleX = (PI * 0.5f) + m_orientation.GetY() * m_rotationSpeed;
    float angleY = m_orientation.GetX() * m_rotationSpeed;

    m_mat = m_mat.Multiply(Matrix::GetRotateZ(-angleY));
    m_mat = m_mat.Multiply(Matrix::GetRotateX(-angleX));
}
