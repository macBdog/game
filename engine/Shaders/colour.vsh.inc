const char * colourVertexShader = 
"void main(void) \n"
"{ \n"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
"	gl_FrontColor = gl_Color; \n"
"}\n";