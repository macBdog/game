#include "OVR_CAPI.h"

#include "..\core\MathUtils.h"
#include "..\core\Quaternion.h"

#include "Log.h"

#include "OculusCamera.h"

void OculusCamera::Startup(ovrHmd a_hmd)
{
	m_HMD = a_hmd;
}

void OculusCamera::Update()
{
	// Refresh the normal camera matrix by reconstruction
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

	if (m_HMD != NULL)
	{
		// Query the HMD for the current tracking state.
		ovrTrackingState trackingState = ovrHmd_GetTrackingState(m_HMD, ovr_GetTimeInSeconds());
		if (trackingState.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
		{
			ovrPosef headPose = trackingState.HeadPose.ThePose;
			ovrQuatf headOrient = headPose.Orientation;
			ovrVector3f headPos = headPose.Position;

			// Set the modified matrix to be the product of the game's normal camera plus HMD movement and orientation
			m_modifiedMat = m_mat;

			// Apply oculus sensor fusion orientation to camera mat
			Quaternion quat = Quaternion(-headOrient.x, headOrient.z, -headOrient.y, headOrient.w);
			quat.ApplyToMatrix(m_modifiedMat);

			m_modifiedMat.SetPos(m_pos + Vector(headPos.x, headPos.y, headPos.z));
		}
	}
}

