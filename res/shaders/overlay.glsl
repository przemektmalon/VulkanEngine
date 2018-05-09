#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef VERTEX ////////////////////////////////////////////////////

layout(push_constant) uniform Text {
    mat4 projView;
    float depth;
    float PAD1;
    float PAD2;
    float PAD3;
} text;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
	fragTexCoord = inTexCoord;
	gl_Position = text.projView * vec4(inPosition, -text.depth, 1.0);
}

#endif
#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(push_constant) uniform Text {
    layout(offset = 80) vec4 colour;
    layout(offset = 96) int type;
} text;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 colour;

void main()
{
	if (text.type == 0) 	// text
		colour = vec4(text.colour.rgb, text.colour.a * texture(texSampler, fragTexCoord).r);
	else 			// poly
		colour = text.colour;
		//colour = poly.colour * texture(texSampler, fragTexCoord);
}

#endif