const char * colourVertexShader = 
"void main(void) \n"
"{ \n"
"	gl_Position = ftransform(); \n"
"	gl_FrontColor = gl_Color; \n"
"}\n";