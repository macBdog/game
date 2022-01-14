const char * lightingFragmentShader = R"(
#version 400
in vec2 OutTexCoord; 
in vec4 LightVertexPos; 
in vec3 LightVertexNormal; 
uniform sampler2D DiffuseTexture; 
uniform sampler2D NormalTexture; 
uniform sampler2D SpecularTexture; 
uniform int MaterialShininess; 
uniform int NumActiveLights; 
uniform vec3 MaterialAmbient; 
uniform vec3 MaterialDiffuse; 
uniform vec3 MaterialSpecular; 
uniform vec3 MaterialEmission; 
uniform mat4 ObjectMatrix; 
uniform mat4 ViewMatrix; 
uniform mat4 ProjectionMatrix; 
uniform float Lights[4*((4*3)+1)]; 
layout(location = 0) out vec4 FragmentColour;
layout(location = 1) out vec4 GBuffer1Colour;
void main(void) 
{ 
	vec4 fragColour = vec4(0.0, 0.0, 0.0, 0.0); 
	int numLightFloats = 4*((4*3)+1);	
	float lightContrib = 1.0 / NumActiveLights; 
	vec4 texNorm = texture2D(NormalTexture, OutTexCoord); 
   vec3 lightVertexAndMap = normalize(texNorm.xyz + LightVertexNormal.xyz); 
	for (int i = 0; i < numLightFloats; i += (4*3)+1) 
	{ 
		float lightWeight = Lights[i];	
		vec4 lightPos = vec4(Lights[i+1], Lights[i+2], Lights[i+3], 1.0); 
		vec3 lightAmbient = vec3(Lights[i+4], Lights[i+5], Lights[i+6]); 
		vec3 lightDiffuse = vec3(Lights[i+7], Lights[i+8], Lights[i+9]); 
		vec3 lightSpecular = vec3(Lights[i+10], Lights[i+11], Lights[i+12]); 
 
		vec3 Iamb = lightAmbient * lightContrib * lightWeight * MaterialAmbient; 
 
		vec4 vertexLight = normalize(LightVertexPos);	
		vec4 lightDirection = lightPos - LightVertexPos;	
		vec4 lightDirNorm = normalize(lightDirection);	
 
		float attenuation = (1.0 / pow(length(lightDirection), 2));	
		float lambert = max(dot(lightVertexAndMap, lightDirection.xyz), 0.0);	
		vec3 Idiff = attenuation * lambert * lightDiffuse * lightWeight * MaterialDiffuse; 
 
		vec3 camPos = inverse(transpose(ViewMatrix))[3].xyz; 
		vec3 cameraDirection = normalize(vertexLight.xyz - camPos); 
		vec4 specMap = texture2D(SpecularTexture, OutTexCoord); 
		vec3 Ispec = lightSpecular * pow(max(dot(vertexLight.xyz, reflect(cameraDirection, lightDirNorm.xyz)), 0.0), (MaterialShininess / 1024)) * lightWeight; 
		Ispec = Ispec * (specMap.xyz); 
 
		vec4 texFrag = texture2D(DiffuseTexture, OutTexCoord); 
		fragColour += texFrag * vec4((Iamb + Idiff + Ispec), texFrag.a); 
	} 
	GBuffer1Colour = fragColour; 
}
)";