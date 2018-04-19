#version 450

#ifdef GEOMETRY ////////////////////////////////////////////////////

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(push_constant) uniform Light {
	vec3 pos;
	float farPlane;
} light;

layout(location = 0) out vec4 FragPos;

mat4 pvs[6] = mat4[](
	mat4(
		vec4(0,0,1,1),
		vec4(0,-1,0,0),
		vec4(-1,0,0,0),
		vec4(0,0,0,0)
	),
	mat4(
		vec4(0,0,-1,-1),
		vec4(0,-1,0,0),
		vec4(1,0,0,0),
		vec4(0,0,0,0)
	),
	mat4(
		vec4(1,0,0,0),
		vec4(0,0,1,1),
		vec4(0,1,0,0),
		vec4(0,0,0,0)
	),
	mat4(
		vec4(1,0,0,0),
		vec4(0,0,-1,-1),
		vec4(0,-1,0,0),
		vec4(0,0,0,0)
	),
	mat4(
		vec4(1,0,0,0),
		vec4(0,-1,0,0),
		vec4(0,0,1,1),
		vec4(0,0,0,0)
	),
	mat4(
		vec4(-1,0,0,0),
		vec4(0,-1,0,0),
		vec4(0,0,-1,-1),
		vec4(0,0,0,0)
	)
);

void main()
{
	pvs[0][3].x = light.pos.z;
	pvs[0][3].y = light.pos.y;
	pvs[0][3].z = -light.pos.x;
	pvs[0][3].w = -light.pos.x;

	pvs[1][3].x = -light.pos.z;
	pvs[1][3].y = light.pos.y;
	pvs[1][3].z = light.pos.x;
	pvs[1][3].w = light.pos.x;

	pvs[2][3].x = -light.pos.x;
	pvs[2][3].y = -light.pos.z;
	pvs[2][3].z = -light.pos.y;
	pvs[2][3].w = -light.pos.y;

	pvs[3][3].x = -light.pos.x;
	pvs[3][3].y = light.pos.z;
	pvs[3][3].z = light.pos.y;
	pvs[3][3].w = light.pos.y;

	pvs[4][3].x = -light.pos.x;
	pvs[4][3].y = light.pos.y;
	pvs[4][3].z = -light.pos.z;
	pvs[4][3].w = -light.pos.z;

	pvs[5][3].x = light.pos.x;
	pvs[5][3].y = light.pos.y;
	pvs[5][3].z = light.pos.z;
	pvs[5][3].w = light.pos.z;

    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle's vertices
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = pvs[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
}

#endif

#ifdef VERTEX ////////////////////////////////////////////////////

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) in vec3 p;

layout(binding = 0) uniform TranformUBO {
    mat4 transform[1000];
} model;

void main()
{
	uint transformIndex = gl_InstanceIndex & 0x000FFFFF;
	mat4 transform = model.transform[transformIndex];
	gl_Position = transform * vec4(p, 1.f);
}

#endif

#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(location = 0) in vec4 FragPos;

layout(push_constant) uniform Light {
	vec3 pos;
	float farPlane;
} light;

void main()
{
	float lightDist = length(FragPos.xyz - light.pos);
	lightDist /= light.farPlane;
	gl_FragDepth = lightDist;
}
#endif

