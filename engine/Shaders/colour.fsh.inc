/*
	uniform sampler2D tex;
	uniform float time;
	uniform float frameTime;

	varying out vec3 colour;

	void main(void)
	{
		colour = gl_Color;
	}
*/
const char * colourFragmentShader = "uniform sampler2D tex; uniform float time; uniform float frameTime; varying out vec3 colour; void main(void) { colour = gl_Color; }";