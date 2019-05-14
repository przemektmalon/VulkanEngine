#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 position;
    vec4 viewRays;
} camera;


#ifdef VERTEX ////////////////////////////////////////////////////

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 1) uniform TranformUBO {
    mat4 transform[1000];
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord; // Not needed but for kept for simplicity


layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPos;
layout(location = 2) flat out uint materialIndex;

void main() {

	uint transformIndex = gl_InstanceIndex & 0x000FFFFF;
	materialIndex = gl_InstanceIndex >> 20;

    mat4 transform = model.transform[transformIndex];
    vec4 worldPos = transform * vec4(inPosition, 1.0);

    gl_Position = camera.proj * camera.view * worldPos;
    fragPos = gl_Position.xyz;
    fragNormal = transpose(inverse(mat3(transform))) * inNormal;
}

#endif


#ifdef FRAGMENT ////////////////////////////////////////////////////


layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) flat in uint materialIndex;

// NOTE: optimizations may be possible with packing outputs together
layout(location = 0) out vec4 colour; // RGB colour, Alpha unised
layout(location = 1) out vec2 normal;
layout(location = 2) out vec2 pbrOut;

struct PBRData {
	vec3 colour;
	float metallic;
	float roughness;
	vec3 __PADDING;
};

layout(binding = 2) uniform PBRDataBuffer {
    PBRData data[100];
} pbrIn;

vec2 encodeNormal(vec3 n)
{
    if (n.z < -0.999)
        n = normalize(vec3(0.001,0.001,-1)); // Temp fix for encoding bug when n.z < ~ -0.999, though this has no visible performace impact, maybe find a better fix

    float p = sqrt(n.z*8.f+8.f);
    return vec2(n.xy/p + 0.5f);
}

void main()
{
	/// TODO: corrrectly get the material index
	//materialIndex = 0;
	
	float metallic  = pbrIn.data[0].metallic;
	float roughness = pbrIn.data[0].roughness;

	colour = vec4(pbrIn.data[0].colour, 1.0);
	normal = encodeNormal(normalize(fragNormal));
	pbrOut = vec2(pbrIn.data[0].roughness, 0.9);
	
	//albedo_metallic = vec4(data.colour, data.metallic);
	//normal_roughness = vec4(encodeNormal(normalize(fragNormal)), data.roughness, 1.0);

	const float C = 1.0;
	const float far = 1000000.0;
	const float offset = 1.0;
	//gl_FragDepth = (log(C * fragPos.z + offset) / log(C * far + offset));
	gl_FragDepth = (log2(fragPos.z + offset) / log2(far + offset));
}

#endif