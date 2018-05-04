#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef VERTEX ////////////////////////////////////////////////////

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(set = 1, binding = 1) uniform Proj {
	mat4 proj;
} mat;

void main()
{
	fragTexCoord = inTexCoord;
	gl_Position = mat.proj * vec4(inPosition, 1.0);
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 colour;

void main()
{
	colour = texture(texSampler, fragTexCoord);
}

#endif