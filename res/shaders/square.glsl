#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef VERTEX ////////////////////////////////////////////////////

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform ProjViewUBO {
    mat4 view;
    mat4 proj;
} m;

layout(binding = 1) uniform TranformUBO {
    mat4 transform[1000];
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) flat out uint textureIndex;

void main() {

	uint transformIndex = gl_InstanceIndex & 0x000FFFFF;
	textureIndex = gl_InstanceIndex >> 20;

    gl_Position = m.proj * m.view * model.transform[transformIndex] * vec4(inPosition, 1.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(binding = 2) uniform sampler2D texSampler[1000];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) flat in uint textureIndex;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 normal;

void main() {
    outColor = texture(texSampler[textureIndex], fragTexCoord);
    normal = vec4(0.1f,0.1f,0.1f,0.1f);
}

#endif