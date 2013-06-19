const char * textureFragmentShader = 
"uniform sampler2D tex; \n"
"uniform float time; \n"
"uniform float frameTime; \n"
"uniform float viewWidth; \n"
"uniform float viewHeight; \n"
"varying vec2 outputTexCoord;\n"
"void main(void) \n"
"{ \n"
"	gl_FragColor = texture2D(tex, outputTexCoord) * gl_Color; \n"
"}\n";