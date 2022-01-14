const char * colourFragmentShader = R"(
#version 400
in vec4 Colour;
uniform vec3 MaterialAmbient; 
layout(location = 0) out vec4 FragmentColour;
layout(location = 1) out vec4 GBuffer1Colour;
layout(location = 2) out vec4 GBuffer2Colour;
layout(location = 3) out vec4 GBuffer3Colour;
layout(location = 4) out vec4 GBuffer4Colour;
layout(location = 5) out vec4 GBuffer5Colour;
layout(location = 6) out vec4 GBuffer6Colour;
void main()
{ 
	FragmentColour = Colour * vec4(MaterialAmbient, 1.0);
	GBuffer1Colour = Colour * vec4(MaterialAmbient, 1.0);
}
)";