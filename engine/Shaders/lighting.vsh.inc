const char * lightingVertexShader = 
"uniform float Time; \n" 
"uniform mat4 ObjMat; \n"
"varying vec2 OutTexCoord; \n"
"varying vec3 lightVertexPos; \n"
"varying vec3 lightVertexNormal; \n"
"void main(void)  \n"
"{ \n"
"	lightVertexPos = ObjMat * gl_Vertex; \n"
"	lightVertexNormal = gl_Normal; \n"
"	gl_FrontColor = gl_Color;  \n"
"	OutTexCoord = gl_MultiTexCoord0.xy; \n"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
"} \n";