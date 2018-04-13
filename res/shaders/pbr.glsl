#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_compute_shader : enable

layout(local_size_x=16, local_size_y=16, local_size_z=1) in;

layout(binding=0, rgba32f) uniform writeonly image2D outColour;

layout(binding=1, rgba8) uniform readonly image2D gAlbedoSpec;
layout(binding=2, rg32f) uniform readonly image2D gNormal;
layout(binding=3, rgba8) uniform readonly image2D gPBR;
layout(binding=4) uniform sampler2D gDepth;

vec3 decodeNormal(vec2 enc)
{
    vec2 fenc = enc*4.f-2.f;
    float f = dot(fenc,fenc);
    float g = sqrt(1.f-f/4.f);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1.f-f/2.f;
    return n;
}

void main()
{
	//uvec2 renderSize = textureSize(gAlbedoSpec, 0);

	uvec3 gid = gl_GlobalInvocationID;
	ivec2 pixel = ivec2(gid.xy);
	
	uint localID = gl_LocalInvocationIndex;

	vec4 albedoSpec = imageLoad(gAlbedoSpec, pixel);
	vec3 normal = decodeNormal(imageLoad(gNormal, pixel).xy);
	vec4 pbr = imageLoad(gPBR, pixel);

	imageStore(outColour, pixel, vec4(normal*0.5 + 0.5,1.f));
}