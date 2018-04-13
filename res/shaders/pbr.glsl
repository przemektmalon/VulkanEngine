#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_compute_shader : enable

layout(local_size_x=16, local_size_y=16, local_size_z=1) in;

layout(binding=0, rgba32f) uniform writeonly image2D outColour;

layout(binding=1, rgba8) uniform readonly image2D gAlbedoSpec;
layout(binding=2, rgba32f) uniform readonly image2D gNormal;
layout(binding=3, rgba8) uniform readonly image2D gPBR;
layout(binding=4) uniform sampler2D gDepth;

void main()
{
	//uvec2 renderSize = textureSize(gAlbedoSpec, 0);

	uvec3 gid = gl_GlobalInvocationID;
	ivec2 pixel = ivec2(gid.xy);
	
	uint localID = gl_LocalInvocationIndex;

	vec4 albedoSpec = imageLoad(gAlbedoSpec, pixel);
	vec4 normal = imageLoad(gNormal, pixel);
	vec4 pbr = imageLoad(gPBR, pixel);

	imageStore(outColour, pixel, albedoSpec);
}