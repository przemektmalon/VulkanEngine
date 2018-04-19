#version 450

layout(push_constant) uniform Light {
	mat4 projView;
	vec4 pos;
	float farPlane;
} light;

#ifdef VERTEX ////////////////////////////////////////////////////

layout (location = 0) in vec3 p;

layout(binding = 0) uniform TranformUBO {
    mat4 transform[1000];
} model;

void main()
{
	uint transformIndex = gl_InstanceIndex & 0x000FFFFF;
	mat4 transform = model.transform[transformIndex];
	gl_Position = light.projView * transform * vec4(p, 1.f);
}

#endif

#ifdef FRAGMENT ////////////////////////////////////////////////////


void main()
{
	//gl_FragDepth = gl_FragCoord.z / 10.f;
}

#endif
