const char * colourFragmentShader = 
"#version 400\n"
"in vec4 Colour;\n"
"layout(location = 0) out vec4 FragmentColour;\n"
"void main()\n"
"{ \n"
"	FragmentColour = Colour;\n"
"}\n";