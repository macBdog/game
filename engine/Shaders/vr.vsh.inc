const char * vrVertexShader = 
"uniform mat4 Texm;\n"
"varying vec2 outputTexCoord;\n"
"void main(void)\n"
"{\n"
"	gl_FrontColor = gl_Color;\n"
"	gl_Position = gl_Vertex;\n"
"	outputTexCoord = gl_MultiTexCoord0.xy;\n"
"}\n";
