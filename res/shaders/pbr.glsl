#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_compute_shader : enable

layout(local_size_x=16, local_size_y=16, local_size_z=1) in;

layout(binding=0, rgba32f) uniform writeonly image2D outColour;

layout(binding=1, rgba8) uniform readonly image2D gAlbedoSpec;
layout(binding=2, rg32f) uniform readonly image2D gNormal;
layout(binding=3, rgba8) uniform readonly image2D gPBR;
layout(binding=4) uniform sampler2D gDepth;

#define MAX_LIGHTS_PER_TILE 256

shared uint maxZ;
shared uint minZ;

shared uint farZMin;
shared uint nearZMax;

shared uint tilePointLightIndices[MAX_LIGHTS_PER_TILE];
shared uint currentTilePointLightIndex;

shared uint tileSpotLightIndices[MAX_LIGHTS_PER_TILE];
shared uint currentTileSpotLightIndex;

struct PointLight
{
	vec4 posRad;
	vec4 colQuad;
	vec4 linearFadeTexHandle;
	mat4 pv[6];
};

struct SpotLight
{
	vec4 posRad;
	vec4 colQuad;
	vec4 linearFadeTexHandle;
	vec4 dirInner;
	vec4 outer;
	mat4 pv;
};

layout(binding = 10) uniform PointLightBuffer {
	PointLight data[150];
} pointLights;

layout(binding = 11) uniform SpotLightBuffer {
	SpotLight data[150];
} spotLights;

layout(binding = 12) uniform LightCounts {
	uint point;
	uint spot;
} lightCounts;

layout(binding = 9) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 position;
    vec4 viewRays;
} camera;

vec3 decodeNormal(vec2 enc)
{
    vec2 fenc = enc*4.f-2.f;
    float f = dot(fenc,fenc);
    float g = sqrt(1.f-f/4.f);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1.f-f/2.f;
    return n;
}

const float PI = 3.14159265359;

bool sphereVsAABB(vec3 sphereCenter, float radius, vec3 AABBCenter, vec3 AABBHalfSize)
{
	vec3 delta = max(vec3(0.f), abs(AABBCenter - sphereCenter) - AABBHalfSize);
	float distSq = dot(delta, delta);
	return distSq <= radius * radius;
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

void main()
{
	uvec2 renderSize = textureSize(gDepth, 0);

	uvec3 gid = gl_GlobalInvocationID;
	ivec2 pixel = ivec2(gid.xy);
	
	uint localID = gl_LocalInvocationIndex;

	float depth = texelFetch(gDepth, pixel, 0).r;

	vec3 viewPos = camera.position.xyz;

	uint z = floatBitsToInt(depth);

	if(localID == 0)
	{
		minZ = 0xFFFFFFFF;
		maxZ = 0;
		currentTilePointLightIndex = 0;
		currentTileSpotLightIndex = 0;
	}

	barrier();

	if(z != 1)
	{
		atomicMax(maxZ, z);
		atomicMin(minZ, z);
	}

	//barrier();
	
	if(localID == 0)
	{
		farZMin = maxZ;
		nearZMax = minZ;
	}

	barrier();

	float maxZf = uintBitsToFloat(maxZ);
	float minZf = uintBitsToFloat(minZ);


	uint halfZ = floatBitsToUint(minZf + ((maxZf - minZf) / 2));

	if(z != 1)
	{
		if(z > halfZ)
		{
			atomicMin(farZMin, z);
		}
		else
		{
			atomicMax(nearZMax, z);
		}
	}

	//barrier();

	float farZMinf = uintBitsToFloat(farZMin);
	float nearZMaxf = uintBitsToFloat(nearZMax);

	float zNear = 0.1;
	float zFar = 5000.f;

    float realDepth = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * depth - 1.0) * (zFar - zNear));

    float minZView = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * minZf - 1.0) * (zFar - zNear));
    float maxZView = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * maxZf - 1.0) * (zFar - zNear));
    float farZMinView = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * farZMinf - 1.0) * (zFar - zNear));
    float nearZMaxView = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * nearZMaxf - 1.0) * (zFar - zNear));

	vec4 viewRays = camera.viewRays;

	vec3 viewRay;
	viewRay.x = mix(viewRays.z, viewRays.x, float(pixel.x)/renderSize.x);
	viewRay.y = mix(viewRays.w, viewRays.y, float(pixel.y)/renderSize.y);

	mat4 vwt = camera.view;
	vwt[3] = vec4(0,0,0,vwt[3][3]);
	vwt = transpose(vwt);
	viewRay = vec3(vwt * vec4(viewRay.xy,1.f,1.f));

	vec3 worldPos = viewPos + (viewRay * -realDepth);

	const uvec3 groupID = gl_WorkGroupID;

	//barrier();

	float xMixRatio1 = float(gl_WorkGroupID.x)/(float(renderSize.x)/float(gl_WorkGroupSize.x));
	float yMixRatio1 = float(gl_WorkGroupID.y)/(float(renderSize.y)/float(gl_WorkGroupSize.y));

	float xMixRatio2 = float(gl_WorkGroupID.x+1)/(float(renderSize.x)/float(gl_WorkGroupSize.x));
	float yMixRatio2 = float(gl_WorkGroupID.y+1)/(float(renderSize.y)/float(gl_WorkGroupSize.y));

	//FAR LIGHTS
	vec3 viewRayTLFar;
	viewRayTLFar.x = mix(viewRays.z, viewRays.x, xMixRatio1);
	viewRayTLFar.y = mix(viewRays.w, viewRays.y, yMixRatio1);
	viewRayTLFar.z = 1.f;

	mat3 m3View = mat3(vwt);

	//viewRayTLFar = vec3(mat3(view) * vec3(viewRayTLFar.xy,1.f));
	viewRayTLFar = m3View * vec3(viewRayTLFar);

	vec3 viewRayBRFar;
	viewRayBRFar.x = mix(viewRays.z, viewRays.x, xMixRatio2);
	viewRayBRFar.y = mix(viewRays.w, viewRays.y, yMixRatio2);
	viewRayBRFar.z = 1.f;

	//viewRayBRFar = vec3(mat3(view) * vec3(viewRayBRFar.xy,1.f));
	viewRayBRFar = m3View * vec3(viewRayBRFar);

	vec3 TLFar = viewPos + (viewRayTLFar * -maxZView);
	vec3 BRFar = viewPos + (viewRayBRFar * -farZMinView);

	vec3 AABBFarHalfSize = (abs(BRFar - TLFar) / 2.f);
	vec3 AABBFarCenter = BRFar - (BRFar - TLFar) / 2.f;
	//FAR LIGHTS

	//NEAR LIGHTS
	vec3 viewRayTLNear;
	viewRayTLNear.x = mix(viewRays.z, viewRays.x, xMixRatio1);
	viewRayTLNear.y = mix(viewRays.w, viewRays.y, yMixRatio1);
	viewRayTLNear.z = 1.f;

	//viewRayTLNear = vec3(mat3(view) * vec3(viewRayTLNear.xy,1.f));
	viewRayTLNear = m3View * vec3(viewRayTLNear);

	vec3 viewRayBRNear;
	viewRayBRNear.x = mix(viewRays.z, viewRays.x, xMixRatio2);
	viewRayBRNear.y = mix(viewRays.w, viewRays.y, yMixRatio2);
	viewRayBRNear.z = 1.f;

	//viewRayBRNear = vec3(mat3(view) * vec3(viewRayBRNear.xy,1.f));
	viewRayBRNear = m3View * vec3(viewRayBRNear);

	vec3 TLNear = viewPos + (viewRayTLNear * -nearZMaxView);
	vec3 BRNear = viewPos + (viewRayBRNear * -minZView);

	vec3 AABBNearHalfSize = (abs(BRNear - TLNear) / 2.f);
	vec3 AABBNearCenter = BRNear - (BRNear - TLNear) / 2.f;
	//NEAR LIGHTS

	uint threadCount = gl_WorkGroupSize.x*gl_WorkGroupSize.y;
	uint passCount = (lightCounts.point + threadCount - 1) / threadCount;

	vec3 lightIn = vec3(0,0,0);

	for(uint i = 0; i < passCount; ++i)
	{	
		uint lightIndex =  (i * threadCount) + gl_LocalInvocationIndex;

		if(lightIndex > lightCounts.point)
			continue;

		vec4 lightPos = pointLights.data[lightIndex].posRad;
		float rad = lightPos.w;

		bool inFrustum = sphereVsAABB(lightPos.xyz, rad, AABBFarCenter, AABBFarHalfSize) || sphereVsAABB(lightPos.xyz, rad, AABBNearCenter, AABBNearHalfSize);


		if(inFrustum)
		{
			lightIn = lightIn + pointLights.data[lightIndex].colQuad.xyz;
			uint nextTileLightIndex = atomicAdd(currentTilePointLightIndex,1);
			tilePointLightIndices[nextTileLightIndex] = lightIndex;
			
		}
	}

	passCount = (lightCounts.spot + threadCount - 1) / threadCount;
	
	for(uint i = 0; i < passCount; ++i)
	{
		uint lightIndex =  (i * threadCount) + gl_LocalInvocationIndex;

		if (lightIndex > lightCounts.spot)
			continue;

		vec4 lightPos = spotLights.data[lightIndex].posRad;
		float rad = lightPos.w * 0.5;
		lightPos.xyz = lightPos.xyz + (spotLights.data[lightIndex].dirInner.xyz * rad);

		bool inFrustum = sphereVsAABB(lightPos.xyz, rad, AABBFarCenter, AABBFarHalfSize) || sphereVsAABB(lightPos.xyz, rad, AABBNearCenter, AABBNearHalfSize);

		//if (inFrustum)
		{
			uint nextTileLightIndex = atomicAdd(currentTileSpotLightIndex,1);
			tileSpotLightIndices[nextTileLightIndex] = lightIndex;
		}
	}

	barrier();

	vec3 litPixel = vec3(0.0);

	float ambient = 0.07;

	vec4 albedoSpec = vec4(0.0);
	float ssaoVal = 1.0;

	vec3 normal = decodeNormal(imageLoad(gNormal, pixel).xy);

	if(depth == 1.f)
	{
		//litPixel += texture(skybox, -viewRay).rgb * 0.05;
	}
	else if (length(normal) > 0.8)
	{
		vec4 pbr = imageLoad(gPBR, pixel);
		
		float roughness = pbr.r;

		albedoSpec = imageLoad(gAlbedoSpec, pixel.xy);

		albedoSpec.xyz = albedoSpec.xyz * ssaoVal;
		
		litPixel += albedoSpec.xyz * ambient;

		float metallic = albedoSpec.a;
		vec3 V = normalize(viewPos - worldPos);

		vec3 F0 = vec3(0.04);
		F0 = mix(F0, albedoSpec.rgb, metallic);

		for(uint j = 0; j < currentTilePointLightIndex; ++j)
		{
			uint i = tilePointLightIndices[j];
			vec4 lightPos = pointLights.data[i].posRad;
			vec4 lightCol = pointLights.data[i].colQuad;
			vec4 linearFadeTexHandle = pointLights.data[i].linearFadeTexHandle;
			float linearAtt = linearFadeTexHandle.x;
			float quadAtt = lightCol.w;
			vec3 lightDir = normalize(lightPos.xyz - worldPos);

			vec3 H = normalize(V + lightDir);
			float dist = length(lightPos.xyz - worldPos);
			float attenuation = 1.f / (1.f + linearAtt * dist + quadAtt * dist * dist);
			vec3 radiance = lightCol.rgb * attenuation;

			float NDF = distributionGGX(normal, H, roughness);
			float G = geometrySmith(normal, V, lightDir, roughness);
			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			vec3 kS = F;
			vec3 kD = vec3(1.f) - kS;
			kD *= 1.0 - metallic;

			vec3 nominator = NDF * G * F;
			float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, lightDir), 0.0) + 0.001;
			vec3 specular = nominator / denominator;

			float NdotL  = max(dot(normal, lightDir), 0.0);

			litPixel += ((kD * albedoSpec.rgb / PI + specular) * radiance * NdotL);
		}

		for(uint j = 0; j < currentTileSpotLightIndex; ++j)
		{
			uint i = tileSpotLightIndices[j];
			vec4 posRad = spotLights.data[i].posRad;
			vec4 colQuad = spotLights.data[i].colQuad;
			vec4 linearFadeTexHandle = spotLights.data[i].linearFadeTexHandle;
			vec4 dirInner = spotLights.data[i].dirInner;
			vec4 outer = spotLights.data[i].outer;

			uint fadeData = floatBitsToUint(linearFadeTexHandle.y);

			vec3 lightDir = normalize(posRad.xyz - worldPos);
			float theta = dot(lightDir, normalize(-dirInner.xyz));
			float epsilon =  dirInner.w - outer.x;
			float intensity = clamp((theta - outer.x) / epsilon, 0.0, 1.0);

			vec3 lightPos = posRad.xyz;

			float linearAtt = linearFadeTexHandle.x;
			float quadAtt = colQuad.w;

			vec3 H = normalize(V + lightDir);
			float dist = length(lightPos.xyz - worldPos);
			float attenuation = 1.f / (1.f + linearAtt * dist + quadAtt * dist * dist);
			vec3 radiance = colQuad.rgb * intensity * attenuation;

			float NDF = distributionGGX(normal, H, roughness);
			float G = geometrySmith(normal, V, lightDir, roughness);
			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			vec3 kS = F;
			vec3 kD = vec3(1.f) - kS;
			kD *= 1.0 - metallic;

			vec3 nominator = NDF * G * F;
			float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, lightDir), 0.0) + 0.001;
			vec3 specular = nominator / denominator;

			float NdotL  = max(dot(normal, lightDir), 0.0);

			litPixel += ((kD * albedoSpec.rgb / PI + specular) * radiance * NdotL);
		}
	}
	
	imageStore(outColour, pixel, vec4(litPixel,1.f));
}