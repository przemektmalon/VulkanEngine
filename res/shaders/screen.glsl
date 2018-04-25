#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef VERTEX ////////////////////////////////////////////////////

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {

    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(binding = 0) uniform sampler2D colSampler;
layout(binding = 1) uniform sampler2D overlaySampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {

	vec4 scene = texture(colSampler, fragTexCoord);
	vec4 overlay = texture(overlaySampler, fragTexCoord);

    outColor.rgb = (1.f - overlay.a) * scene.rgb + (overlay.a * overlay.rgb);
}

#endif