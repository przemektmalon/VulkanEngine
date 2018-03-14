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
    mat4 transform[3];
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out uint instanceIndex;

void main() {
    gl_Position = m.proj * m.view * model.transform[gl_InstanceIndex] * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    instanceIndex = gl_InstanceIndex;
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(binding = 2) uniform sampler2D texSampler[1000];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint instanceIndex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[instanceIndex], fragTexCoord);
}

#endif