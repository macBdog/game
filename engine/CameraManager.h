#ifndef _ENGINE_CAMERA_MANAGER
#define _ENGINE_CAMERA_MANAGER
#pragma once

#include "..\core\Matrix.h"
#include "..\core\Vector.h"

#include "Singleton.h"

class OculusCamera;

class Camera
{
public:

	 //\brief Contruct a valid camera at the origin look down the Y axis
	Camera() 
		: m_translationSpeed(sc_defaultCameraSpeed)
		, m_rotationSpeed(sc_defaultCameraRotSpeed)
		, m_pos(Vector::Zero())
		, m_orientation(0.0f)
		, m_orientationInput(0.0f)
    {
		Update();
    }

	//\brief Reset the camera based on a new look at target
    void LookAt(Vector a_worldPos);

	//\brief Update the camera matrix from the inputs
	virtual void Update();

	//\ingroup Accessors
	inline const Matrix & GetMatrix() const { return m_mat; }
	inline const Matrix & GetViewMatrix() const { return m_viewMat; }
	inline Vector GetInversePos() const { return Vector(-m_pos.GetX(), -m_pos.GetY(), -m_pos.GetZ()); }
	inline Vector GetPosition() const { return m_pos; }
	inline float GetRotationSpeed() const { return m_rotationSpeed; }
	inline float GetTranslationSpeed() const { return m_translationSpeed; }
	inline Vector2 GetOrientation() const { return m_orientation; }
	inline Vector2 GetOrientationInput() const { return m_orientationInput; }

	//\ingroup Mutators
	inline void SetPos(const Vector & a_pos) { m_pos = a_pos; }
	inline void SetOrientation(const Vector2 & a_orient) { m_orientation = a_orient; }
	inline void SetOrientationInput(const Vector2 & a_input) { m_orientationInput = a_input; }

protected:

	static const float sc_defaultCameraSpeed;		///< Debug camera translation constant
	static const float sc_defaultCameraRotSpeed;	///< Debug camera rotation speed degrees per second

	float m_translationSpeed;		///< Camera translation speed
	float m_rotationSpeed;			///< Camera rotation speed degrees per second
	Matrix m_mat;                   ///< The matrix defining the new coordinate system as calculated by the camera
	Matrix m_viewMat;				///< The matrix defining the direction the camera is looking
	Vector m_pos;                   ///< Position of the camera in the world
	Vector2 m_orientation;			///< Angle of the camera around it's view axis
	Vector2 m_orientationInput;		///< Input to orientation given mouse corrds
};

class CameraManager : public Singleton<CameraManager>
{

public:

	//\brief Contruct a valid camera at the origin look down the Y axis
	CameraManager() 
		: m_currentCamera(&m_gameCamera) 
		, m_oculusCamera(NULL)
	{
	}

	//\brief Stubbed out for loading cameras for each scene
	void Startup(bool a_useVrCamera);
	void Shutdown();
	void Update(float a_dt);

	//\brief Accessors for rendering 
	inline Matrix GetCameraMatrix() const { return m_currentCamera->GetMatrix(); }
	inline Matrix GetViewMatrix() const { return m_currentCamera->GetViewMatrix(); }
	inline Vector GetWorldPos() const { return m_currentCamera->GetInversePos(); }
	float GetVRProjectionCentreOffset();
	float GetVRAspect();
	float GetVRFOV();
	float GetVRIPD();
	
	///\brief Mutators affecting the currently active camera
	void SetPosition(const Vector & a_newPos);
	void SetRotation(const Vector & a_newRot);
	void SetFOV(const float & a_newFov);

	void GetInverseMat(Matrix & a_mat_OUT);

private:

	Camera m_gameCamera;							///< Camera only modified by game/script 
	Camera m_debugCamera;							///< Camera modified while debug menu is active
	OculusCamera * m_oculusCamera;					///< Camera used for input from Oculus branded  VR/head mounted displays
	Camera * m_currentCamera;						///< Pointer to either camera
};

#endif //_ENGINE_CAMERA_MANAGER