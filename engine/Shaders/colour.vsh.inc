const char * colourVertexShader = 
"uniform mat4 ObjectMatrix; \n"
"uniform mat4 ViewMatrix; \n"
"uniform mat4 ProjectionMatrix; \n"
"void main(void) \n"
"{ \n"
"	gl_Position = gl_Vertex * ObjectMatrix * ViewMatrix * ProjectionMatrix;\n"
"	gl_FrontColor = gl_Color; \n"
"}\n";