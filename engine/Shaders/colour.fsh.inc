/*
	uniform sampler2D tex;
	uniform float time;
	uniform float frameTime;
	uniform float viewWidth;
	uniform float viewHeight;

	void main(void)
	{
		colour = gl_Color;
	}
*/
const char * colourFragmentShader = "uniform sampler2D tex; uniform float time; uniform float frameTime; uniform float viewWidth; uniform float viewHeight; void main(void) { gl_FragColor = gl_Color; }";