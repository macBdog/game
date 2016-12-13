const char * textureFragmentShader = 
"#version 400\n"
"in vec4 Colour;\n"
"in vec2 OutTexCoord;\n"
"layout(location = 0) out vec4 FragmentColour;\n"
"uniform sampler2D DiffuseTexture;\n"
"void main()\n"
"{ \n"
"	FragmentColour = texture2D(DiffuseTexture, OutTexCoord) * Colour;\n"
"}\n";
