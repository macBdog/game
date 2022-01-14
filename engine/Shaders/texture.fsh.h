const char * textureFragmentShader = R"(
#version 400
uniform sampler2D DiffuseTexture;
uniform sampler2D NormalTexture; 
uniform sampler2D SpecularTexture; 
uniform sampler2D GBuffer1; 
uniform sampler2D GBuffer2; 
uniform sampler2D GBuffer3; 
uniform sampler2D GBuffer4; 
uniform sampler2D GBuffer5; 
uniform sampler2D GBuffer6; 
uniform sampler2D DepthBuffer; 
layout(location = 0) out vec4 FragmentColour;
layout(location = 1) out vec4 GBuffer1Colour;
layout(location = 2) out vec4 GBuffer2Colour;
layout(location = 3) out vec4 GBuffer3Colour;
layout(location = 4) out vec4 GBuffer4Colour;
layout(location = 5) out vec4 GBuffer5Colour;
layout(location = 6) out vec4 GBuffer6Colour;
in vec4 Colour;
in vec2 OutTexCoord;
void main()
{ 
	GBuffer1Colour = texture2D(DiffuseTexture, OutTexCoord) * Colour;
}
)";
