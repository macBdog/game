#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "OVR_CAPI_GL.h"

#include "..\core\MathUtils.h"
#include "..\core\Quaternion.h"

#include "Log.h"

#include "OculusCamera.h"

void OculusCamera::Startup()
{

}

void OculusCamera::Update(ovrSession * a_session)
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
}
