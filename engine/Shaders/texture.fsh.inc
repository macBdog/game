/*
	uniform sampler2D tex;
	uniform float time;
	uniform float frameTime;
	
	varying out vec3 colour;

	void main(void)
	{
		gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * gl_Color;
	}
*/
const char * textureFragmentShader = "uniform sampler2D tex; uniform float time; varying out vec3 colour; void main(void) { gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * gl_Color; }";