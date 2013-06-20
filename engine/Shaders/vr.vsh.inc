const char * vrVertexShader = 
"varying vec2 OutTexCoord; \n"
"void main(void) \n"
"{ \n"
"	gl_FrontColor = gl_Color; \n"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
"	OutTexCoord = gl_MultiTexCoord0.xy; \n"
"} \n";
