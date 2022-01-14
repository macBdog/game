const char * particleFragmentShader = R"(
#version 400
uniform sampler2D DiffuseTexture;
uniform sampler2D GBuffer1; 
uniform sampler2D DepthBuffer; 
layout(location = 0) out vec4 FragmentColour;
layout(location = 1) out vec4 GBuffer1Colour;

in Fragment
{
	vec2 UV;
	vec4 Colour;
} fragment;

void main()
{ 
	float d = dot(fragment.UV, fragment.UV);
	if(d > 1.0)
	{
		discard;
	}
	GBuffer1Colour = vec4(fragment.Colour.rgb, fragment.Colour.a * 1.0 - d);
}
)";
