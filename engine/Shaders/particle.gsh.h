const char * particleGeometryShader = R"(
#version 400
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
in Particle 
{ 
	vec3 Position;
	vec4 Colour;
   float Size;
} particle[];

out Fragment
{
	vec2 UV;
	vec4 Colour;
} fragment;

void main()
{
	fragment.Colour = particle[0].Colour;
	vec4 center = vec4(particle[0].Position, 1);
	mat4 bbMat = inverse(transpose(ViewMatrix));
	float sZ = particle[0].Size;
	vec4 bR = bbMat[0] * sZ;
	vec4 bU = bbMat[1] * sZ;

	vec2 uv = vec2(-1, -1);
	vec4 p = center - bR - bU;
	fragment.UV = uv;
	gl_Position = p * ViewMatrix * ProjectionMatrix;
	EmitVertex();

	uv = vec2(1, -1);
	p = center + bR - bU;
	fragment.UV = uv;
	gl_Position = p * ViewMatrix * ProjectionMatrix;
	EmitVertex();

	uv = vec2(-1, 1);
	p = center - bR + bU;
	fragment.UV = uv;
	gl_Position = p * ViewMatrix * ProjectionMatrix;
	EmitVertex();

	uv = vec2(1, 1);
	p = center + bR + bU;
	fragment.UV = uv;
	gl_Position = p * ViewMatrix * ProjectionMatrix;
	EmitVertex();
	EndPrimitive();
})";