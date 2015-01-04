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
	if (m_HMD != NULL)
	{
		// Query the HMD for the current tracking state.
		ovrTrackingState trackingState = ovrHmd_GetTrackingState(m_HMD, ovr_GetTimeInSeconds());
		if (trackingState.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
		{
			ovrPosef headPose = trackingState.HeadPose.ThePose;
			ovrQuatf headOrient = headPose.Orientation;
			ovrVector3f headPos = headPose.Position;
    
			// Oculus world is Y up and looking down +Z, apply transformation to -Y look Z up
			m_mat = Matrix::Identity();
			m_mat.SetPos(m_pos);
			Quaternion pitchNeg = Quaternion(Vector(1.0f, 0.0f, 0.0f), MathUtils::Deg2Rad(90));
			Matrix rotMat = pitchNeg.GetRotationMatrix();
			m_mat = m_mat.Multiply(rotMat);

			// Apply oculus sensor fusion orientation to camera mat
			Quaternion quat = Quaternion(headOrient.x, headOrient.y, headOrient.z, headOrient.w);
			quat.ApplyToMatrix(m_mat);

			Vector headTrans = m_mat.GetPos() + Vector(headPos.x, headPos.y, headPos.z);
			m_mat.SetPos(headTrans);
		}
	}
}
