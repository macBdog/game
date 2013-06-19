const char * textureVertexShader = 
"varying vec2 outputTexCoord;\n"
"void main(void)\n"
"{\n"
"	gl_FrontColor = gl_Color;\n"
"	outputTexCoord = gl_MultiTexCoord0.xy;\n"
"	gl_Position = gl_Vertex;\n"
"}\n";