#ifndef _ENGINE_CAMERA_MANAGER
#define _ENGINE_CAMERA_MANAGER
#pragma once

#include "..\core\Matrix.h"
#include "..\core\Vector.h"

#include "Singleton.h"

class Camera
{
public:

	 //\brief Contruct a valid camera at the origin look down the Y axis
	Camera() 
		: m_translationSpeed(sc_defaultCameraSpeed)
		, m_rotationSpeed(sc_defaultCameraRotSpeed)
		, m_pos(Vector::Zero())
		, m_target(0.0f, sc_defaultCameraTargetDistance, 0.0f)
    {
		Update();
    }

	//\brief Reset the camera based on a new look at target
    void LookAt(Vector a_worldPos);

	//\brief Update the camera matrix from the inputs
	virtual void Update();

	//\brief The purpose of the camera class is to supply a camera matrix to the render manager
	virtual const Matrix & GetCameraMatrix() const { return m_mat; }
	
	//\ingroup Accessors for components that are used to calculate the camera matrix
	inline const Vector & GetPosition() const { return m_pos; }
	inline const Vector & GetTarget() const { return m_target; }
	inline float GetRotationSpeed() const { return m_rotationSpeed; }
	inline float GetTranslationSpeed() const { return m_translationSpeed; }

	//\ingroup Mutators
	inline void SetPosition(const Vector & a_worldPos) { m_pos = a_worldPos; }
	inline void SetTarget(const Vector & a_worldPos) { m_target = a_worldPos; }

	static const float sc_defaultCameraSpeed;			///< Debug camera translation constant
	static const float sc_defaultCameraRotSpeed;		///< Debug camera rotation speed degrees per second
	static const float sc_defaultCameraTargetDistance;	///< How far into the distance the camera target is

protected:

	float m_translationSpeed;		///< Camera translation speed
	float m_rotationSpeed;			///< Camera rotation speed degrees per second
	Matrix m_mat;                   ///< The matrix defining the new coordinate system as calculated by the camera
	Vector m_pos;                   ///< Position of the camera in the world
	Vector m_target;				///< Position of where the camera is looking in the world
};

class CameraManager : public Singleton<CameraManager>
{

public:

	//\brief Contruct a valid camera at the origin look down the Y axis
	CameraManager() 
		: m_currentCamera(&m_gameCamera) 
		, m_debugOrientationInput(0.0f)
		, m_debugMouseLookAngleEuler(0.0f)
	{ }

	//\brief Stubbed out for loading cameras for each scene
	void Startup() { ; }
	void Shutdown() { ; }
	void Update(float a_dt);

	//\brief Accessors for rendering 
	inline Matrix GetCameraMatrix() const { return m_currentCamera->GetCameraMatrix(); }
	inline Vector GetWorldPos() const { return m_currentCamera->GetPosition(); }
	
	///\brief Mutators affecting the currently active camera
	inline void SetPosition(const Vector & a_newPos) { m_currentCamera->SetPosition(a_newPos); }
	inline void SetRotation(const Vector & a_newRot) { /*m_currentCamera->SetRot(a_newRot);*/ }
	inline void SetFOV(const float & a_newFov) { /*m_currentCamera->SetFOV(a_newFov);*/ }
	inline void SetTarget(const Vector & a_newTarget) { m_currentCamera->SetTarget(a_newTarget); }

private:

	Camera m_gameCamera;							///< Camera only modified by game/script 
	Camera m_debugCamera;							///< Camera modified while debug menu is active
	Camera * m_currentCamera;						///< Pointer to either camera
	Vector2 m_debugOrientationInput;				///< Input to orientation given mouse coords
	Vector m_debugMouseLookAngleEuler;				///< Where the debug camera is looking
};

#endif //_ENGINE_CAMERA_MANAGER