const char * globalFragmentShader = 
"uniform sampler2D DiffuseTexture; \n"
"uniform sampler2D NormalTexture; \n"
"uniform sampler2D SpecularTexture; \n"
"uniform float Time; \n"
"uniform float LifeTime; \n"
"uniform float FrameTime; \n"
"uniform float ViewWidth; \n"
"uniform float ViewHeight; \n"
"uniform mat4 ObjectMatrix; \n"
"uniform mat4 ViewMatrix; \n"
"uniform mat4 ProjectionMatrix; \n"
"varying vec2 OutTexCoord; \n";