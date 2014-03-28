#include <SDL.h>

#include "..\core\MathUtils.h"

#include "CameraManager.h"
#include "OculusCamera.h"
#include "InputManager.h"

const float Camera::sc_defaultCameraSpeed = 10.0f;
const float Camera::sc_defaultCameraRotSpeed = 0.01f;

template<> CameraManager * Singleton<CameraManager>::s_instance = NULL;

void CameraManager::SetPosition(const Vector & a_newPos)
{
	m_currentCamera->SetPos(a_newPos);
}

void CameraManager::SetRotation(const Vector & a_newRot)
{
	//m_currentCamera->SetRot(a_newRot);
}

void CameraManager::SetFOV(const float & a_newFov)
{
	//m_currentCamera->SetFOV(a_newFov)
}

void CameraManager::Startup(bool a_useVrCamera)
{
	if (a_useVrCamera)
	{
		m_oculusCamera = new OculusCamera();
		if (m_oculusCamera != NULL && m_oculusCamera->IsInitialised())
		{
			m_currentCamera = m_oculusCamera;
		}
	}
}

void CameraManager::Shutdown()
{
	if (m_oculusCamera != NULL)
	{
		m_oculusCamera->Shutdown();
		delete m_oculusCamera;
	}
}

void CameraManager::Update(float a_dt)
{
	m_currentCamera = &m_gameCamera;
	
	// Use VR camera if enabled and initialised correctly
	if (m_oculusCamera != NULL && m_oculusCamera->IsInitialised())
	{
		m_currentCamera = m_oculusCamera;
	}

	// Process camera controls
	InputManager & inMan = InputManager::Get();
	if (DebugMenu::Get().IsDebugMenuEnabled())
	{
		m_currentCamera = &m_debugCamera;
	
		// Create a view direction matrix
		Matrix viewMat = m_currentCamera->GetViewMatrix();
		Vector camPos = m_currentCamera->GetPosition();
		const float speed = m_currentCamera->GetTranslationSpeed();
		const float rotSpeed = m_currentCamera->GetRotationSpeed();
		
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
				const float cameraMoveEpsilonSq = 0.005f;
				if ((orientInput - curInput).LengthSquared() > cameraMoveEpsilonSq)
				{
					orientInput = curInput;
				}
				orient += (curInput - orientInput) * maxRot;
				m_currentCamera->SetOrientation(orient);
				m_currentCamera->SetOrientationInput(curInput);
			}
		}
	}

	m_currentCamera->Update();
}

 float CameraManager::GetVRProjectionCentreOffset()
{
	return m_oculusCamera != NULL && m_oculusCamera->IsInitialised() ? m_oculusCamera->GetProjectionCentreOffset() : 0.0f;
}

float CameraManager::GetVRAspect() {

	return m_oculusCamera != NULL && m_oculusCamera->IsInitialised() ? m_oculusCamera->GetAspect() : 0.0f; 
}

float CameraManager::GetVRFOV() 
{ 
	return m_oculusCamera != NULL && m_oculusCamera->IsInitialised() ? m_oculusCamera->GetFOV() : 0.0f; 
}

float CameraManager::GetVRIPD() 
{ 
	return m_oculusCamera != NULL && m_oculusCamera->IsInitialised() ? m_oculusCamera->GetIPD() : 0.0f; 
}

void Camera::Update()
{
	// Refresh the view mat
	m_viewMat = Matrix::Identity();
	m_viewMat = m_viewMat.Multiply(Matrix::GetRotateX(m_orientation.GetY() * m_rotationSpeed));
	m_viewMat = m_viewMat.Multiply(Matrix::GetRotateZ(m_orientation.GetX() * m_rotationSpeed));
	m_viewMat.SetPos(m_pos);

    // Create the matrix
	m_mat = Matrix::Identity();
	m_mat.SetPos(m_pos);

    // Rotate the matrix about two axis defined by the mouse coords
    float angleX = (PI * 0.5f) + m_orientation.GetY() * m_rotationSpeed;
    float angleY = m_orientation.GetX() * m_rotationSpeed;

    m_mat = m_mat.Multiply(Matrix::GetRotateZ(-angleY));
    m_mat = m_mat.Multiply(Matrix::GetRotateX(-angleX));
}
