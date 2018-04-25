#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Text {
    mat4 projView;
    vec4 colour;
} text;

#ifdef VERTEX ////////////////////////////////////////////////////

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
	fragTexCoord = inTexCoord;
	gl_Position = text.projView * vec4(inPosition, 1.0);
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 colour;

void main()
{
	colour = vec4(text.colour.rgb, text.colour.a * texture(texSampler, fragTexCoord).r);
}

#endif