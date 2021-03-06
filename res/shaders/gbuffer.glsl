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
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) flat out uint textureIndex;
layout(location = 3) out vec3 viewVec;
layout(location = 4) out vec3 fragPos;

void main() {

	uint transformIndex = gl_InstanceIndex & 0x000FFFFF;
	textureIndex = gl_InstanceIndex >> 20;

    mat4 transform = model.transform[transformIndex];

    vec4 worldPos = transform * vec4(inPosition, 1.0);

    //viewVec = camera.position - worldPos.xyz;
    viewVec = worldPos.xyz - camera.position;
    gl_Position = camera.proj * camera.view * worldPos;
    fragPos = gl_Position.xyz;
    fragNormal = transpose(inverse(mat3(transform))) * inNormal;
    fragTexCoord = inTexCoord;
}

#endif


#ifdef FRAGMENT ////////////////////////////////////////////////////

layout(binding = 2) uniform sampler2D texSampler[1000];

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) flat in uint textureIndex;
layout(location = 3) in vec3 viewVec;
layout(location = 4) in vec3 fragPos;

layout(location = 0) out vec4 colour;
layout(location = 1) out vec2 normal;
layout(location = 2) out vec2 pbr;


vec2 encodeNormal(vec3 n)
{
    if (n.z < -0.999)
        n = normalize(vec3(0.0001,0.0001,-1)); // Temp fix for encoding bug when n.z < ~ -0.999, though this has no visible performace impact, maybe find a better fix

    float p = sqrt(n.z*8.f+8.f);
    return vec2(n.xy/p + 0.5f);
}

mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( normalize(T * invmax), normalize(B * invmax), N );
}

vec3 perturbNormal(vec3 N, vec3 V, vec2 texcoord, vec3 nTex)
{
    vec3 map = nTex * 2.f - 1.f;
    mat3 TBN = cotangentFrame( N, -V, texcoord );
    return normalize( TBN * map );
}

void main()
{
	vec4 albedoSpec = texture(texSampler[textureIndex], fragTexCoord);
	vec4 normalRough = texture(texSampler[textureIndex+1], fragTexCoord);

    colour = albedoSpec;
    normal = encodeNormal(normalize(perturbNormal(normalize(fragNormal), normalize(viewVec), fragTexCoord, normalRough.xyz)));
    //normal = encodeNormal(normalize(fragNormal));
    //normal = encodeNormal(normalize(normalRough.xyz));

    //colour = vec4(fragNormal,1.f);

    pbr.x = normalRough.w; // Roughness

	const float C = 1.0;
	const float far = 1000000.0;
	const float offset = 1.0;
	//gl_FragDepth = (log(C * fragPos.z + offset) / log(C * far + offset));
	gl_FragDepth = (log2(fragPos.z + offset) / log2(far + offset));
}

#endif