#ifndef _ENGINE_VR_RENDER
#define _ENGINE_VR_RENDER
#pragma once

//\brief VRRender handles all the rendering and Gl specific Oculus API integration tasks
class VRRender
{
public:

	VRRender() 
		: m_renderInit(false)
		, m_winSizeW(0)
		, m_winSizeH(0)
		, m_renderTime(0.0f)
		, m_lastRenderTime(0.0f)
		, m_lookDir(0.0f)
		, m_lookPos(0.0f)
		, m_fboId(0) {};

	bool InitRendering(int a_winSizeW, int a_winSizeH);
	bool InitRenderingNoHMD();
	void Startup();
	void Update(float a_dt);
	
	//\brief Calls OVR's begin frame and end frame calls with RenderManager::DrawScene sandwiched in between
	bool DrawToHMD();

	inline const Vector& GetLookDir() const { return m_lookDir; }
	inline const Vector& GetLookPos() const { return m_lookPos; }

	void Shutdown();
	void DeinitRendering();

private:

	bool m_renderInit;												///< Has InitRendering been called successfully
	int m_winSizeW;
	int m_winSizeH;
	float m_renderTime;												///< How long the game has been rendering frames for (accumulated frame delta)
	float m_lastRenderTime;											///< How long the last frame took
	Vector m_lookDir;												///< Cache off the latest head sensor reading for feedback into the game
	Vector m_lookPos;												///< Cache off the latest head sensor reading for feedback into the game
	GLuint m_fboId;
};


#endif //_ENGINE_VR_RENDER