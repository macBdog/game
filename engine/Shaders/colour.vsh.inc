/*
	void main(void) 
	{
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	}
*/
const char * colourVertexShader = "void main(void) { gl_Position = ftransform(); gl_FrontColor = gl_Color;  }";