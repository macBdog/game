const char * particleVertexShader = 
"#version 400\n"
"uniform float Time; \n"
"uniform float LifeTime; \n"
"uniform mat4 ObjectMatrix;\n"
"in vec3 ParticlePosition;\n"
"in vec3 ParticleVelocity;\n"
"in vec4 ParticleColour;\n"
"in float ParticleSize;\n"
"in float ParticleLifeTime;\n"
"in float ParticleBirthTime;\n"
"out Particle \n"
"{ \n"
"	vec3 Position;\n"
"	vec4 Colour;\n"
"	float Size;\n"
"} particle;\n"
"void main() \n"
"{ \n"
"	float lifeTime = 0.0;\n"
"	float lifeFrac = 0.0;\n"
"   vec3 acceleration = vec3(0, 0, -10.0);\n"
"	particle.Colour = vec4(0.0);\n"
"	particle.Position = vec3(-9999999.0);\n"
"	if (Time > ParticleBirthTime)\n"
"	{\n"
"		vec3 emitterPos = transpose(ObjectMatrix)[3].xyz;\n"
"		lifeTime = Time - ParticleBirthTime;\n"
"		lifeFrac = clamp(lifeTime / ParticleLifeTime, 0.0, 1.0);\n"
"		particle.Position = emitterPos + ParticlePosition + ((ParticleVelocity * lifeTime) + (acceleration * lifeTime * lifeTime * 0.5));\n"
"		particle.Colour = vec4(ParticleColour.xyz, 1.0 - lifeFrac);\n"
"		particle.Size = ParticleSize * (1.0 - lifeFrac);\n"
"	}\n"
"} \n"
;