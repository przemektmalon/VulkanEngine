#version 450

/*
 This is a modified version of Scalable Ambient Obscurance (SAO) by Morgan McGuire and Michael Mara, NVIDIA Research

 Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

 Copyright (c) 2011-2012, NVIDIA
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifdef VERTEX ////////////////////////////////////////////////////

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec2 TexCoord;

void main()
{
    gl_Position = vec4(position, 0.f, 1.0f);
    TexCoord = texCoord;
}

#endif

#ifdef FRAGMENT ////////////////////////////////////////////////////

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 outColour;

layout(binding = 0) uniform Config {
    // Number of samples
    int samples; // = 50

    // Turns for samples, keep this and 'samples' coprime for best results
    int spiralTurns; // = 30 

    // Height in pixels of a 1m object viewed from 1m away. Compute from projection matrix
    float projScale; // = 500

    // AO radius
    float radius; // = 1.f

    // Bias to AO in smooth corners
    float bias; // = 0.01f

    // Intensity divided by radius^6
    float intensity; // = 0.00001f

        // Proj info
    /* vec4(-2.0f / (width*P[0][0]), 
          -2.0f / (height*P[1][1]),
          ( 1.0f - P[0][2]) / P[0][0], 
          ( 1.0f + P[1][2]) / P[1][1])

    where P is the projection matrix */
    vec4 projInfo;

} config;

layout(binding = 1) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
    vec4 viewRays;
} camera;

/** Negative, "linear" values in world-space units */
layout (binding = 2) uniform sampler2D depthBuffer;

/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling. 
    This need not match the real far plane*/
#define FAR 10000.f // projection matrix's far plane
#define FAR_PLANE_Z (-10000.0)

/** Reconstruct camera-space P.xyz from screen-space S = (x, y) in
    pixels and camera-space z < 0.  Assumes that the upper-left pixel center
    is at (0.5, 0.5) [but that need not be the location at which the sample tap 
    was placed!]

    Costs 3 MADD.  Error is on the order of 10^3 at the far plane, partly due to z precision.
  */
vec3 reconstructCSPosition(vec2 S, float z) {
    return vec3((S.xy * config.projInfo.xy + config.projInfo.zw) * z, z);

}

/** Reconstructs screen-space unit normal from screen-space position */
vec3 reconstructCSFaceNormal(vec3 C) {
    return normalize(cross(dFdx(C), dFdy(C)));
}

const float NEAR = 0.1f; // projection matrix's near plane

float LinearizeDepth(float depth)
{    
    vec3 c = vec3(NEAR * FAR, NEAR - FAR, FAR);
    float r = c.x / ((depth * c.y) + c.z);
    return r;
}

/////////////////////////////////////////////////////////

/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
vec2 tapLocation(int sampleNumber, float spinAngle, out float ssR){
    // Radius relative to ssR
    float alpha = float(sampleNumber + 0.5) * (1.0 / float(config.samples));
    float angle = alpha * (float(config.spiralTurns) * 6.28) + spinAngle;

    ssR = alpha;
    return vec2(cos(angle), sin(angle));
}


/** Used for packing Z into the GB channels */
float CSZToKey(float z) {
    return clamp(z * (1.0 / FAR_PLANE_Z), 0.0, 1.0);
}
 
/** Read the camera-space position of the point at screen-space pixel ssP */
vec3 getPosition(ivec2 ssP) {
    vec3 P;
    P.z = LinearizeDepth(texelFetch(depthBuffer, ssP, 0).r);
    //float depth = -(exp2(P.z * log2(FAR + 1.0)) - 1.f);
    //float depth = P.z;
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), P.z);

    return P;
}


/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1 */
vec3 getOffsetPosition(ivec2 ssC, vec2 unitOffset, float ssR) {

    ivec2 ssP = ivec2(ssR * unitOffset) + ssC;
    
    vec3 P;

    ivec2 mipP = clamp(ssP, ivec2(0), textureSize(depthBuffer, 0) - ivec2(1));
    P.z = LinearizeDepth(texelFetch(depthBuffer, mipP, 0).r);
    
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), P.z);

    return P;
}

/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius 

    Note that units of H() in the HPG12 paper are meters, not
    unitless.  The whole falloff/sampling function is therefore
    unitless.  In this implementation, we factor out (9 / radius).

    Four versions of the falloff function are implemented below
*/
float sampleAO(in ivec2 ssC, in vec3 C, in vec3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle) {
    // Offset on the unit disk, spun for this pixel
    float ssR;
    vec2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
    ssR *= ssDiskRadius;
    
    // The occluding point in camera space
    vec3 Q = getOffsetPosition(ssC, unitOffset, ssR);

    vec3 v = Q - C;

    float vv = dot(v, v);
    float vn = dot(v, n_C);

    const float epsilon = 0.01;
    
    // A: From the HPG12 paper
    // Note large epsilon to avoid overdarkening within cracks
    //return float(vv < radius2) * max((vn - bias) / (epsilon + vv), 0.0) * radius2 * 0.6;

    float radius2 = config.radius * config.radius;
    float invRadius2 = 1.f / radius2;

    // B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
    float f = max(radius2 - vv, 0.0); return f * f * f * max((vn - config.bias) / (epsilon + vv), 0.0);

    // C: Medium contrast (which looks better at high radii), no division.  Note that the 
    // contribution still falls off with radius^2, but we've adjusted the rate in a way that is
    // more computationally efficient and happens to be aesthetically pleasing.
    //return 4.0 * max(1.0 - vv * invRadius2, 0.0) * max(vn - bias, 0.0);

    // D: Low contrast, no division operation
    //return 2.0 * float(vv < radius * radius) * max(vn - bias, 0.0);
}

void main() {

    // Pixel being shaded 
    ivec2 ssC = ivec2(gl_FragCoord.xy);

    // World space point being shaded
    vec3 C = getPosition(ssC);

    // Packing bilateral key
    float key = CSZToKey(-C.z);
    float temp = floor(key * 256.0);

    // Integer part
    outColour.g = temp * (1.0 / 256.0);

    // Fractional part
    outColour.b = key * 256.0 - temp;


    // Hash function used in the HPG12 AlchemyAO paper
    float randomPatternRotationAngle = (3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10;

    // Reconstruct normals from positions. These will lead to 1-pixel black lines
    // at depth discontinuities, however the blur will wipe those out so they are not visible
    // in the final image.
    vec3 n_C = reconstructCSFaceNormal(C);

    // Choose the screen-space sample radius
    // proportional to the projected area of the sphere
    float ssDiskRadius = -config.projScale * config.radius / C.z;
    //float ssDiskRadius =  projScale / C.z;
    
    float sum = 0.0;
    for (int i = 0; i < config.samples; ++i) {
        sum += sampleAO(ssC, C, n_C, ssDiskRadius, i, randomPatternRotationAngle);
    }

    float A = max(0.0, 1.0 - sum * config.intensity * (5.0 / float(config.samples)));

    // Bilateral box-filter over a quad for free, respecting depth edges
    // (the difference that this makes is subtle)
    if (abs(dFdx(C.z)) < 0.02) {
        A -= dFdx(A) * ((ssC.x & 1) - 0.5);
    }
    if (abs(dFdy(C.z)) < 0.02) {
        A -= dFdy(A) * ((ssC.y & 1) - 0.5);
    }

    outColour.r = A;
}

#endif