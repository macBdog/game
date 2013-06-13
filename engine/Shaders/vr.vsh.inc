const char * vrVertexShader = 
"void main(void)\n"
"{\n"
"	gl_FrontColor = gl_Color;\n"
"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"}\n";