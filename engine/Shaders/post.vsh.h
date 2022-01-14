const char * postVertexShader = R"(
#version 400
in vec3 VertexPosition;
in vec4 VertexColour;
in vec2 VertexUV;
in vec3 VertexNormal;
out vec4 Colour;
out vec2 OutTexCoord;
uniform mat4 ObjectMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
void main() 
{ 
	gl_Position = vec4(VertexPosition, 1.0) * ObjectMatrix * ViewMatrix * ProjectionMatrix;
	Colour = VertexColour;
	OutTexCoord = VertexUV;
})";