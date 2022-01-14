const char * globalVertexShader = R"(
#version 400
in vec3 VertexPosition;
in vec4 VertexColour;
in vec2 VertexUV;
in vec3 VertexNormal;
uniform float Time;  
uniform float LifeTime; 
uniform float FrameTime; 
uniform float ViewWidth; 
uniform float ViewHeight; 
uniform vec3 ShaderData; 
uniform mat4 ObjectMatrix; 
uniform mat4 ViewMatrix; 
uniform mat4 ProjectionMatrix; 
float rand(vec2 co) 
{ 
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); 
}
)";