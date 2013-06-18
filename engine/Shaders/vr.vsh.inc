const char * vrVertexShader = 
"uniform mat4 Texm;\n"
"void main(void)\n"
"{\n"
"	gl_FrontColor = gl_Color;\n"
"	vec4 wPos = gl_Vertex;\n"
"	gl_Position = wPos;\n"
"	vec2 tc = gl_MultiTexCoord0.xy;\n"
"	gl_TexCoord[0].xy = tc;\n"
"}\n";

/*
    "uniform mat4 View;\n"
    "uniform mat4 Texm;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    "varying  vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = View * Position;\n"
    "   oTexCoord = vec2(Texm * vec4(TexCoord,0,1));\n"
    "   oTexCoord.y = 1.0-oTexCoord.y;\n"
	"}\n";
*/