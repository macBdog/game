const char * lightingFragmentShader = 
"#version 400\n"
"in vec2 OutTexCoord; \n"
"in vec4 LightVertexPos; \n"
"in vec3 LightVertexNormal; \n"
"uniform sampler2D DiffuseTexture; \n"
"uniform sampler2D NormalTexture; \n"
"uniform sampler2D SpecularTexture; \n"
"uniform int MaterialShininess; \n"
"uniform int NumActiveLights; \n"
"uniform vec3 MaterialAmbient; \n"
"uniform vec3 MaterialDiffuse; \n"
"uniform vec3 MaterialSpecular; \n"
"uniform vec3 MaterialEmission; \n"
"uniform mat4 ObjectMatrix; \n"
"uniform mat4 ViewMatrix; \n"
"uniform mat4 ProjectionMatrix; \n"
"uniform float Lights[4*((4*3)+1)]; \n"
"layout(location = 0) out vec4 FragmentColour;\n"
"layout(location = 1) out vec4 GBuffer1Colour;\n"
"void main(void) \n"
"{ \n"
"	vec4 fragColour = vec4(0.0, 0.0, 0.0, 0.0); \n"
"	int numLightFloats = 4*((4*3)+1);	\n"
"	float lightContrib = 1.0 / NumActiveLights; \n"
"	vec4 texNorm = texture2D(NormalTexture, OutTexCoord); \n"
"   vec3 lightVertexAndMap = normalize(texNorm.xyz + LightVertexNormal.xyz); \n"
"	for (int i = 0; i < numLightFloats; i += (4*3)+1) \n"
"	{ \n"
"		float lightWeight = Lights[i];	\n"
"		vec4 lightPos = vec4(Lights[i+1], Lights[i+2], Lights[i+3], 1.0); \n"
"		vec3 lightAmbient = vec3(Lights[i+4], Lights[i+5], Lights[i+6]); \n"
"		vec3 lightDiffuse = vec3(Lights[i+7], Lights[i+8], Lights[i+9]); \n"
"		vec3 lightSpecular = vec3(Lights[i+10], Lights[i+11], Lights[i+12]); \n"
" \n"
"		vec3 Iamb = lightAmbient * lightContrib * lightWeight * MaterialAmbient; \n"
" \n"
"		vec4 vertexLight = normalize(LightVertexPos);	\n"
"		vec4 lightDirection = lightPos - LightVertexPos;	\n"
"		vec4 lightDirNorm = normalize(lightDirection);	\n"
" \n"
"		float attenuation = (1.0 / pow(length(lightDirection), 2));	\n"
"		float lambert = max(dot(lightVertexAndMap, lightDirection.xyz), 0.0);	\n"
"		vec3 Idiff = attenuation * lambert * lightDiffuse * lightWeight * MaterialDiffuse; \n"
" \n"
"		vec3 camPos = inverse(transpose(ViewMatrix))[3].xyz; \n"
"		vec3 cameraDirection = normalize(vertexLight.xyz - camPos); \n"
"		vec4 specMap = texture2D(SpecularTexture, OutTexCoord); \n"
"		vec3 Ispec = lightSpecular * pow(max(dot(vertexLight.xyz, reflect(cameraDirection, lightDirNorm.xyz)), 0.0), (MaterialShininess / 1024)) * lightWeight; \n"
"		Ispec = Ispec * (specMap.xyz); \n"
" \n"
"		vec4 texFrag = texture2D(DiffuseTexture, OutTexCoord); \n"
"		fragColour += texFrag * vec4((Iamb + Idiff + Ispec), texFrag.a); \n"
"	} \n"
"	GBuffer1Colour = fragColour; \n"
" } \n";