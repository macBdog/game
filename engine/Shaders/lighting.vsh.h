const char * lightingVertexShader = R"(
#version 400
in vec3 VertexPosition;
in vec4 VertexColour;
in vec2 VertexUV;
in vec3 VertexNormal;
out vec2 OutTexCoord;
out vec4 LightVertexPos; 
out vec3 LightVertexNormal; 
uniform mat4 ObjectMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
out vec4 Colour;
void main(void)  
{ 
	LightVertexPos = vec4(VertexPosition, 1.0) * ObjectMatrix;
	LightVertexNormal = VertexNormal;
	OutTexCoord = VertexUV;
	gl_Position = vec4(VertexPosition, 1.0) * ObjectMatrix * ViewMatrix * ProjectionMatrix;
})";