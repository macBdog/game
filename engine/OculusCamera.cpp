#include "..\core\MathUtils.h"
#include "..\core\Quaternion.h"

#include "Log.h"

#include "OculusCamera.h"

void OculusCamera::Startup()
{
	OVR::System::Init();

	m_manager = *OVR::DeviceManager::Create();
	m_HMD = *m_manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	if (m_HMD == NULL)
	{
		Log::Get().WriteEngineErrorNoParams("Failed to find Oculus HMD device.");
		Shutdown();
		return;
	}

    // Populate hmdInfo with device name, screen width, height etc
    OVR::HMDInfo hmdInfo;
    m_HMD->GetDeviceInfo(&hmdInfo);
	m_stereoConfig.SetHMDInfo(hmdInfo);
	m_stereoConfig.SetFullViewport(OVR::Util::Render::Viewport(0,0, hmdInfo.HResolution, hmdInfo.VResolution));
	m_stereoConfig.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);
	
	m_sensors[0] = *m_HMD->GetSensor();
	OVR::SensorFusion SFusion;
    if (m_sensors[0])
	{
		m_fusionResults[0] = new OVR::SensorFusion();
		m_fusionResults[0]->AttachToSensor(m_sensors[0]);
	}
	else
	{
		Log::Get().WriteEngineErrorNoParams("Could not attach to Oculus HMD sensor.");
		Shutdown();
		return;
	}

	m_initialised = true;
}

void OculusCamera::Shutdown()
{
	for (int i = 0; i < s_numSensors; ++i)
	{
		m_sensors[i].Clear();
	}
    m_HMD.Clear();
    m_manager.Clear();

    OVR::System::Destroy();

	m_initialised = false;
}

void OculusCamera::Update()
{	
    // Rotate the matrix about the rotation from the oculus sensor
    OVR::Quatf oculusQuat = m_fusionResults[0]->GetOrientation();
	Quaternion quat = Quaternion(oculusQuat.x, oculusQuat.z, oculusQuat.y, oculusQuat.w);
	m_mat = Matrix::Identity();
	quat.ApplyToMatrix(m_mat);

	// Oculus world is Y up and looking down +Z, apply transformation to -Y look Z up
	Quaternion pitchNeg = Quaternion(Vector(1.0f, 0.0f, 0.0f), MathUtils::Deg2Rad(90));
	Matrix rotMat = pitchNeg.GetRotationMatrix();
	m_mat = m_mat.Multiply(rotMat);
	m_mat.SetPos(m_pos);

	// TODO: set view mat from oculus mat
	//m_viewMat = m_mat.Inverse();
}
