const char * particleVertexShader = R"(
#version 400
uniform float Time; 
uniform float LifeTime; 
uniform mat4 ObjectMatrix;
in vec3 ParticlePosition;
in vec3 ParticleVelocity;
in vec4 ParticleColour;
in float ParticleSize;
in float ParticleLifeTime;
in float ParticleBirthTime;
out Particle 
{ 
	vec3 Position;
	vec4 Colour;
	float Size;
} particle;
void main() 
{ 
	float lifeTime = 0.0;
	float lifeFrac = 0.0;
   vec3 acceleration = vec3(0, 0, -10.0);
	particle.Colour = vec4(0.0);
	particle.Position = vec3(-9999999.0);
	if (Time > ParticleBirthTime)
	{
		vec3 emitterPos = transpose(ObjectMatrix)[3].xyz;
		lifeTime = Time - ParticleBirthTime;
		lifeFrac = clamp(lifeTime / ParticleLifeTime, 0.0, 1.0);
		particle.Position = emitterPos + ParticlePosition + ((ParticleVelocity * lifeTime) + (acceleration * lifeTime * lifeTime * 0.5));
		particle.Colour = vec4(ParticleColour.xyz, 1.0 - lifeFrac);
		particle.Size = ParticleSize * (1.0 - lifeFrac);
	}
})";