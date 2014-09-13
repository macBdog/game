const char * textureFragmentShader = 
"uniform sampler2D DiffuseTexture; \n"
"varying vec2 OutTexCoord; \n"
"void main(void) \n"
"{ \n"
"	gl_FragColor = texture2D(DiffuseTexture, OutTexCoord) * gl_Color; \n"
"}\n";
