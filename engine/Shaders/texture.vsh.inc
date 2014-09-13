const char * textureVertexShader = 
"uniform mat4 ObjectMatrix; \n"
"uniform mat4 ViewMatrix; \n"
"uniform mat4 ProjectionMatrix; \n"
"varying vec2 OutTexCoord; \n"
"void main(void) \n"
"{ \n"
"	gl_FrontColor = gl_Color; \n"
"	gl_Position = gl_Vertex * ObjectMatrix * ViewMatrix * ProjectionMatrix; \n"
"	OutTexCoord = gl_MultiTexCoord0.xy; \n"
"} \n";