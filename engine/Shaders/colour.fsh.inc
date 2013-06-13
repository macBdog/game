const char * colourFragmentShader = 
"uniform sampler2D tex; \n" 
"uniform float time; \n"
"uniform float frameTime; \n"
"uniform float viewWidth; \n"
"uniform float viewHeight;\n"
"void main(void) \n"
"{ \n"
"	gl_FragColor = gl_Color; \n"
"}\n";