const char * postFragmentShader = 
"#version 400\n"
"uniform sampler2D DiffuseTexture;\n"
"uniform sampler2D NormalTexture; \n"
"uniform sampler2D SpecularTexture; \n"
"uniform sampler2D GBuffer1; \n"
"uniform sampler2D GBuffer2; \n"
"uniform sampler2D GBuffer3; \n"
"uniform sampler2D GBuffer4; \n"
"uniform sampler2D GBuffer5; \n"
"uniform sampler2D GBuffer6; \n"
"uniform sampler2D DepthBuffer; \n"
"layout(location = 0) out vec4 FragmentColour;\n"
"layout(location = 1) out vec4 GBuffer1Colour;\n"
"layout(location = 2) out vec4 GBuffer2Colour;\n"
"layout(location = 3) out vec4 GBuffer3Colour;\n"
"layout(location = 4) out vec4 GBuffer4Colour;\n"
"layout(location = 5) out vec4 GBuffer5Colour;\n"
"layout(location = 6) out vec4 GBuffer6Colour;\n"
"in vec4 Colour;\n"
"in vec2 OutTexCoord;\n"
"void main()\n"
"{ \n"
"	GBuffer1Colour = texture2D(GBuffer1, OutTexCoord) * Colour;\n"
"	FragmentColour = GBuffer1Colour;\n"
"}\n"
;