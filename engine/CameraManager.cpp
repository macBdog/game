#include <SDL.h>

#include "..\core\MathUtils.h"

#include "CameraManager.h"
#include "OculusCamera.h"
#include "InputManager.h"

const float Camera::sc_defaultCameraSpeed = 8.0f;
const float Camera::sc_defaultCameraRotSpeed = 48.0f;

template<> CameraManager * Singleton<CameraManager>::s_instance = NULL;

void CameraManager::SetPosition(const Vector & a_newPos)
{
	m_currentCamera->SetPosition(a_newPos);
}

void CameraManager::SetRotation(const Vector & a_newRot)
{
	//m_currentCamera->SetRot(a_newRot);
}

void CameraManager::SetFOV(const float & a_newFov)
{
	//m_currentCamera->SetFOV(a_newFov)
}

void CameraManager::SetTarget(const Vector & a_newTarget)
{
	m_currentCamera->SetTarget(a_newTarget);
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
		Matrix cameraMat = m_currentCamera->GetCameraMatrix();
		Vector camPos = m_currentCamera->GetPosition();
		Vector camTarget = m_currentCamera->GetTarget();
		const float speed = m_currentCamera->GetTranslationSpeed();
		const float rotSpeed = m_currentCamera->GetRotationSpeed();
		
		// Don't change camera while there is a dialog up
		if (!DebugMenu::Get().IsDebugMenuActive())
		{
			// WSAD for FPS style movement
			Vector moveOffset(0.0f);
			if (inMan.IsKeyDepressed(SDLK_w)) {	moveOffset -= cameraMat.GetUp() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_s)) {	moveOffset += cameraMat.GetUp() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_d)) { moveOffset += cameraMat.GetRight() * a_dt * speed; }	
			if (inMan.IsKeyDepressed(SDLK_a)) {	moveOffset -= cameraMat.GetRight() * a_dt * speed; }
			if (inMan.IsKeyDepressed(SDLK_q)) {	moveOffset += Vector(0.0f, 0.0f, speed * a_dt); }
			if (inMan.IsKeyDepressed(SDLK_e)) { moveOffset -= Vector(0.0f, 0.0f, speed * a_dt); }	
		
			// Move both the camera and the target together
			m_currentCamera->SetPosition(camPos + moveOffset);
			m_currentCamera->SetTarget(camTarget + moveOffset);
			
			// Set rotation based on mouse delta while hotkey pressed
			if (inMan.IsKeyDepressed(SDLK_LSHIFT))
			{
				// Get current camera inputs
				const float maxRot = 360.0f;
				Vector2 curInput = inMan.GetMousePosRelative();
				Vector2 lastInput = m_currentCamera->GetOrientationInput();

				// Cancel out mouse movement above an epsilon to prevent the camera jumping around
				const float cameraMoveEpsilonSq = 0.05f;
				Vector2 vecInput = curInput - lastInput;
				if (vecInput.LengthSquared() > cameraMoveEpsilonSq)
				{
					vecInput = Vector2::Vector2Zero();
				}

				Quaternion rotation(Vector(vecInput.GetY() * a_dt * rotSpeed, 0.0f, -vecInput.GetX() * a_dt * rotSpeed));
				Matrix rotMat = Matrix::Identity();
				rotation.ApplyToMatrix(rotMat);
				Vector newTarget = rotMat.Transform(camTarget);
				m_currentCamera->SetTarget(newTarget);

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
	// Refresh the camera matrix by reconstruction
	Vector forward = (m_target - m_pos);
	forward.Normalise();
	forward = -forward;

	Vector right = forward.Cross(Vector::Up());
	right.Normalise();
	right = right;

	Vector up = right.Cross(forward);
 
	right = -right;
	m_mat.SetRight(right);
	m_mat.SetLook(up);
	m_mat.SetUp(forward);
	m_mat.SetPos(m_pos);
}
