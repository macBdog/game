/*
	uniform sampler2D tex;		// The input texture
	uniform float time;			// How long the app has been running for in seconds
	uniform float frameTime;	// Time since the last frame was drawn
	uniform float viewWidth;	// Framebuffer render width in pixels
	uniform float viewHeight;	// Framebuffer render height in pixels
*/
const char * globalFragmentShader = "uniform sampler2D tex; uniform float time; uniform float frameTime; uniform float viewWidth; uniform float viewHeight;";