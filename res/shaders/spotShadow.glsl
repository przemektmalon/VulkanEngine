#version 450


#ifdef VERTEX ////////////////////////////////////////////////////

layout (location = 0) in vec3 p;

struct SpotLight
{
	vec4 posRad;
	vec4 colQuad;
	vec4 linearFadeTexHandle;
	vec4 dirInner;
	vec4 outer;
	mat4 pv;
};

layout(binding = 0) uniform TranformUBO {
    mat4 transform[1000];
} model;

layout(binding = 1) uniform SpotLightBuffer {
	SpotLight data[150];
} spotLights;

void main()
{
	uint transformIndex = gl_InstanceIndex & 0x000FFFFF;
	mat4 transform = model.transform[transformIndex];
	uint lightIndex = gl_InstanceIndex >> 20;
	gl_Position = spotLights.data[lightIndex].pv * transform * vec4(p, 1.f);
}

#endif

#ifdef FRAGMENT ////////////////////////////////////////////////////


void main()
{
	//gl_FragDepth = gl_FragCoord.z / 10.f;
}

#endif
