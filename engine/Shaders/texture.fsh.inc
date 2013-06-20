const char * textureFragmentShader = 
"uniform sampler2D Texture0; \n"
"varying vec2 OutTexCoord; \n"
"void main(void) \n"
"{ \n"
"	gl_FragColor = texture2D(Texture0, OutTexCoord) * gl_Color; \n"
"}\n";