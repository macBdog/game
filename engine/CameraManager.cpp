#include <SDL.h>

#include "..\core\MathUtils.h"

#include "CameraManager.h"
#include "InputManager.h"
#include "FontManager.h"

const float Camera::sc_defaultCameraSpeed = 32.0f;
const float Camera::sc_defaultCameraRotSpeed = 256.0f;
const float Camera::sc_defaultCameraTargetDistance = 10.0f;

template<> CameraManager * Singleton<CameraManager>::s_instance = nullptr;

void CameraManager::Update(float a_dt)
{
	m_currentCamera = &m_gameCamera;
	
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
				Vector2 curInput = inMan.GetMousePosRelative();
				Vector2 lastInput = m_debugOrientationInput;

				// Cancel out mouse movement above an epsilon to prevent the camera jumping around
				const float cameraMoveEpsilonSq = 0.05f;
				Vector2 vecInput = curInput - lastInput;
				if (vecInput.LengthSquared() > cameraMoveEpsilonSq)
				{
					vecInput = Vector2::Vector2Zero();
				}

				m_debugMouseLookAngleEuler += Vector(vecInput.GetY() * a_dt * rotSpeed, 0.0f, -vecInput.GetX() * a_dt * rotSpeed);
				Quaternion rotation(m_debugMouseLookAngleEuler);
				Matrix rotMat = Matrix::Identity();
				rotation.ApplyToMatrix(rotMat);
				Vector newTarget = rotMat.TransformInverse(Vector(0.0f, Camera::sc_defaultCameraTargetDistance, 0.0f));
				m_currentCamera->SetTarget(camPos + newTarget);

				m_debugOrientationInput = curInput;
			}


			// Draw camera position on top of everything
			if (DebugMenu::Get().IsDebugMenuEnabled())
			{
				const Vector & camPos = m_currentCamera->GetPosition();
				char buf[32];
				sprintf(buf, "%.2f, %.2f, %.2f", camPos.GetX(), camPos.GetY(), camPos.GetZ());
				FontManager::Get().DrawDebugString2D(buf, Vector2(0.8f, 0.95f));
			}
		}
	}

	m_currentCamera->Update();
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

void Camera::SetRotation(const Vector & a_rotationEuler)
{
	Quaternion rotation(a_rotationEuler);
	Matrix rotMat = Matrix::Identity();
	rotation.ApplyToMatrix(rotMat);
	Vector newTarget = rotMat.Transform(Vector(0.0f, 1.0f, 0.0f));
	SetTarget(GetPosition() + newTarget);
}