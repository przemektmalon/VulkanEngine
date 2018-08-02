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

layout(binding = 0) uniform sampler2D colour[10];
layout(binding = 1) uniform sampler2D depth[10];

layout(push_constant) uniform Input {
    uint count;
} input;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {

	vec4 finalColour = texture(colour[0], fragTexCoord);
	float finalDepth = texture(depth[0], fragTexCoord);

	for (int i = 1; i < input.count; ++i)
	{
		float depth = texture(depth[i], fragTexCoord);
		if (depth < finalDepth)
		{
			finalColour = texture(colour[i], fragTexCoord);
			finalDepth = depth;
		}
	}

	outColor = finalColour;
}

#endif