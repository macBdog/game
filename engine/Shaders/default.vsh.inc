/*
	void main(void) 
	{
		gl_Position = ftransform();
		gl_FrontColor = gl_Color;
	}
*/
const char * defaultVertexShader = "void main(void) {\ngl_Position = ftransform();\ngl_FrontColor = gl_Color;\n}\n";