#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform Data {
    mat4 projView; // 0
    vec4 colour; // 64
    float depth; // 80
    int type; // 84
    float PAD1; // 88
    float PAD2; // 92
} data;

#ifdef VERTEX ////////////////////////////////////////////////////

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
	fragTexCoord = inTexCoord;
	gl_Position = data.projView * vec4(inPosition, -data.depth, 1.0);
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 colour;

void main()
{
	if (data.type == 0) 	// text
		colour = vec4(data.colour.rgb, data.colour.a * texture(texSampler, fragTexCoord).r);
	else 			// poly
        colour = data.colour;
		//colour = poly.colour * texture(texSampler, fragTexCoord);
}

#endif