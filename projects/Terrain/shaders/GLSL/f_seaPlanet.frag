#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.141592653589793238462
#define NUMLIGHTS 2
#define RADIUS 2020
 

struct LightPD
{
    vec4 position;		// vec3
    vec4 direction;		// vec3
};

struct LightProps
{
    int type;			// int   0: no light   1: directional   2: point   3: spot

    vec4 ambient;		// vec3
    vec4 diffuse;		// vec3
    vec4 specular;		// vec3

    vec4 degree;		// vec3	(constant, linear, quadratic)
    vec4 cutOff;		// vec2 (cuttOff, outerCutOff)
};

struct PreCalcValues
{
	vec3 halfwayDir[NUMLIGHTS];		// Bisector of the angle viewDir-lightDir
	vec3 lightDirFrag[NUMLIGHTS];	// Light direction from lightSource to fragment
	float attenuation[NUMLIGHTS];	// How light attenuates with distance
	float intensity[NUMLIGHTS];
	
} pre;

struct uvGradient	// Gradients for the X and Y texture coordinates can be used for fetching the textures when non-uniform flow control exists (https://www.khronos.org/opengl/wiki/Sampler_(GLSL)#Non-uniform_flow_control).  
{
	vec2 uv;		// xy texture coords
	vec2 dFdx;		// Gradient of x coords
	vec2 dFdy;		// Gradient of y coords
};

layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[32];		// sampler1D, sampler2D, sampler3D

layout(location = 0)  		in vec3 	inPos;
layout(location = 1)  flat	in vec3 	inCamPos;
layout(location = 2)  		in vec3 	inNormal;
layout(location = 3)  		in float	inSlope;
layout(location = 4)  		in float	inDist;
layout(location = 5)  flat	in float	inCamSqrHeight;
layout(location = 6)		in float	inGroundHeight;
layout(location = 7)  		in vec3	 	inTanX;
layout(location = 8)  		in vec3	 	inBTanX;
layout(location = 9)  		in vec3	 	inTanY;
layout(location = 10)  		in vec3	 	inBTanY;
layout(location = 11) 		in vec3	 	inTanZ;
layout(location = 12) 		in vec3	 	inBTanZ;
layout(location = 13) flat  in float    inTime;
layout(location = 14) flat	in LightPD	inLight[NUMLIGHTS];

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


// Declarations:

vec3  getFragColor	  (vec3 albedo, vec3 normal, vec3 specularity, float roughness);
void  getTex		  (inout vec3 result, int albedo, int normal, int specular, int roughness, float scale, vec2 UV);
vec4  triplanarTexture(sampler2D tex, float texFactor);
vec4  triplanarTexture(sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy);
vec3  triplanarNormal (sampler2D tex, float texFactor);		// https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
vec3  triplanarNormal (sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy);
vec3  toRGB			  (vec3 vec);							// Transforms non-linear sRGB color to linear RGB. Note: Usually, input is non-linear sRGB, but it's automatically converted to linear RGB in the shader, and output later in sRGB.
vec3  toSRGB		  (vec3 vec);							// Transforms linear RGB color to non-linear sRGB
void  precalculateValues();
vec2  unpackUV		  (vec2 UV, float texFactor);			// Invert Y axis and apply scale
vec3  unpackNormal    (vec3 normal);						// Pass to SRGB, put in range [-1,1], and normalize
uvGradient getGradients(vec2 uvs);
vec3  applyLinearFog  (vec3 fragColor, vec3 fogColor, float minDist, float maxDist);
float applyLinearFog  (float value, float fogValue, float minDist, float maxDist);
vec3  applyFog		  (vec3 fragColor, vec3 fogColor);
float applyFog		  (float value,   float fogValue);
float getTexScaling	  (float initialTexFactor, float baseDist, float mixRange, inout float texFactor1, inout float texFactor2);
vec3  getDryColor	  (vec3 color, float minHeight, float maxHeight);
float modulus		  (float dividend, float divider);		// modulus(%) = a - (b * floor(a/b))

vec3 getTex_Sea();
vec3 triplanarNormal_Sea(sampler2D tex, float texFactor, float speed);
float getTransparency(float minTransp, float minDist, float maxDist);
float getRatio(float value, float min, float max);

// Definitions:

void main()
{
	precalculateValues();
	vec3 color = getTex_Sea();
	outColor = vec4(color, getTransparency(0.1, 20, 40));
}

vec3 getTex_Sea()
{	
	// Colors: https://colorswall.com/palette/63192
	// Colors: https://www.color-hex.com/color-palette/101255

	return getFragColor( 
		vec3(36, 76, 92) / 255.f,
		triplanarNormal_Sea(texSampler[31], 10, 2),
		vec3(1, 1, 1),
		30 );
}

float ratio(float value, float min, float max)
{
	return clamp((value - min) / (max - min), 0, 1);
}

float scaleRatio(float ratio, float min, float max)
{
	return (max - min) * ratio + min;
}

vec3 direction(vec3 origin, vec3 end)
{
	return normalize(end - origin);
}

float angle(vec3 dir_1, vec3 dir_2)
{
	return acos(dot(dir_1, dir_2));
}

float getTransparency(float minTransp, float minDist, float maxDist) 
{
	return 1;
	float ratio_1;		// The closer, the more transparent
	float ratio_2;		// The lower the camera-normal angle is, the more transparent
	float ratio_3;		// The larger distance, the less transparent
	
	ratio_1 = ratio(inDist, minDist, maxDist);				// [0,1]
	ratio_1 = scaleRatio(ratio_1, minTransp, 1);			// [minTransp, 1]
	
	ratio_2 = angle(inNormal, direction(inPos, inCamPos));	// angle normal-vertex-camera
	ratio_2 = ratio(ratio_2, 0, PI/2);						// [0,1]
	ratio_2 = scaleRatio(ratio_2, minTransp, 1);			// [minTransp, 1]
	
	return ratio_2;
	
	// <<< GLSL headers
}


// Tools ---------------------------------------------------------------------------------------------

vec3 directionalLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
	// ----- Ambient lighting -----
	vec3 ambient = ubo.light[i].ambient.xyz * albedo;
	if(dot(inLight[i].direction.xyz, normal) > 0) return ambient;			// If light comes from below the tangent plane
	
	// ----- Diffuse lighting -----
	float diff   = max(dot(normal, -inLight[i].direction.xyz), 0.f);
	vec3 diffuse = ubo.light[i].diffuse.xyz * albedo * diff;		
	
	// ----- Specular lighting -----
	float spec		= pow(max(dot(normal, pre.halfwayDir[i]), 0.0), roughness * 4);
	vec3 specular	= ubo.light[i].specular.xyz * specularity * spec;
	
	// ----- Result -----
	return vec3(ambient + diffuse + specular);
}


vec3 PointLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    // ----- Ambient lighting -----
    vec3 ambient = ubo.light[i].ambient.xyz * albedo * pre.attenuation[i];
	if(dot(pre.lightDirFrag[i], normal) > 0) return ambient;				// If light comes from below the tangent plane

    // ----- Diffuse lighting -----
    float diff   = max(dot(normal, -pre.lightDirFrag[i]), 0.f);
    vec3 diffuse = ubo.light[i].diffuse.xyz * albedo * diff * pre.attenuation[i];
	
    // ----- Specular lighting -----
	float spec        = pow(max(dot(normal, pre.halfwayDir[i]), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec * pre.attenuation[i];
	
    // ----- Result -----
    return vec3(ambient + diffuse + specular);	
}


vec3 SpotLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    // ----- Ambient lighting -----
    vec3 ambient = ubo.light[i].ambient.xyz * albedo * pre.attenuation[i];
	if(dot(pre.lightDirFrag[i], normal) > 0) return ambient;				// If light comes from below the tangent plane

    // ----- Diffuse lighting -----
	float diff      = max(dot(normal, -pre.lightDirFrag[i]), 0.f);
    vec3 diffuse    = ubo.light[i].diffuse.xyz * albedo * diff * pre.attenuation[i] * pre.intensity[i];

    // ----- Specular lighting -----
	float spec        = pow(max(dot(normal, pre.halfwayDir[i]), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec * pre.attenuation[i] * pre.intensity[i];
	
    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}

// Apply the lighting type you want to a fragment
vec3 getFragColor(vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
	//albedo      = applyLinearFog(albedo, vec3(.1,.1,.1), 100, 500);
	//specularity = applyLinearFog(specularity, vec3(0,0,0), 100, 500);
	//roughness   = applyLinearFog(roughness, 0, 100, 500);

	vec3 result = vec3(0,0,0);

	for(int i = 0; i < NUMLIGHTS; i++)		// for each light source
	{
		if(ubo.light[i].type == 1)
			result += directionalLightColor	(i, albedo, normal, specularity, roughness);
		else if(ubo.light[i].type == 2)
			result += PointLightColor		(i, albedo, normal, specularity, roughness);
		else if(ubo.light[i].type == 3)
			result += SpotLightColor		(i, albedo, normal, specularity, roughness);
	}
	
	return result;
}

void getTex(inout vec3 result, int albedo, int normal, int specular, int roughness, float scale, vec2 UV)
{
	result = getFragColor(
				texture(texSampler[albedo], UV/scale).rgb,
				normalize(toSRGB(texture(texSampler[normal], UV/scale).rgb) * 2.f - 1.f).rgb,
				texture(texSampler[specular], UV/scale).rgb, 
				texture(texSampler[roughness], UV/scale).r * 255 );
}

void precalculateValues()
{
	vec3 viewDir = normalize(inCamPos - inPos);	// Camera view direction
	float distFragLight;						// Distance fragment-lightSource
	float theta;								// The closer to 1, the more direct the light gets to fragment.
	float epsilon;								// Cutoff range
	int type;									// Light type
	
	for(int i = 0; i < NUMLIGHTS; i++)
	{
		type = ubo.light[i].type;
		if(type == 1)
		{
			pre.halfwayDir[i]	= normalize(-inLight[i].direction.xyz + viewDir);
		}
		else if(type == 2)
		{
			distFragLight		= length(inLight[i].position.xyz - inPos);
			pre.lightDirFrag[i]	= normalize(inPos - inLight[i].position.xyz);
			pre.halfwayDir[i]   = normalize(-pre.lightDirFrag[i] + viewDir);
			pre.attenuation[i]  = 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distFragLight + ubo.light[i].degree[2] * distFragLight * distFragLight);
		}
		else if(type == 3)
		{
			distFragLight		= length(inLight[i].position.xyz - inPos);
			pre.lightDirFrag[i]	= normalize(inPos - inLight[i].position.xyz);
			pre.halfwayDir[i]   = normalize(-pre.lightDirFrag[i] + viewDir);
			pre.attenuation[i]	= 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distFragLight + ubo.light[i].degree[2] * distFragLight * distFragLight);
			theta				= dot(pre.lightDirFrag[i], inLight[i].direction.xyz);
			epsilon				= ubo.light[i].cutOff[0] - ubo.light[i].cutOff[1];
			pre.intensity[i]	= clamp((theta - ubo.light[i].cutOff[1]) / epsilon, 0.0, 1.0);
		}
	}
}

vec2 unpackUV(vec2 UV, float texFactor)
{
	return UV.xy * vec2(1, -1) / texFactor;
}

vec4 triplanarTexture(sampler2D tex, float texFactor)
{
	vec4 dx = texture(tex, unpackUV(inPos.zy, texFactor));
	vec4 dy = texture(tex, unpackUV(inPos.xz, texFactor));
	vec4 dz = texture(tex, unpackUV(inPos.xy, texFactor));
	
	vec3 weights = abs(normalize(inNormal));
	weights *= weights;
	weights = weights / (weights.x + weights.y + weights.z);

	return dx * weights.x + dy * weights.y + dz * weights.z;
}

vec4 triplanarTexture(sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy)
{
	vec4 dx = textureGrad(tex, dzy.uv, dzy.dFdx, dzy.dFdy);
	vec4 dy = textureGrad(tex, dxz.uv, dxz.dFdx, dxz.dFdy);
	vec4 dz = textureGrad(tex, dxy.uv, dxy.dFdx, dxy.dFdy);
	
	vec3 weights = abs(normalize(inNormal));
	weights *= weights;
	weights = weights / (weights.x + weights.y + weights.z);

	return dx * weights.x + dy * weights.y + dz * weights.z;
}

vec3 unpackNormal(vec3 normal)
{
	return normalize(toSRGB(normal) * 2.f - 1.f);
}

vec3 triplanarNormal(sampler2D tex, float texFactor)
{	
	// Tangent space normal maps (retrieved using triplanar UVs; i.e., 3 facing planes)
	vec3 tnormalX = unpackNormal(texture(tex, unpackUV(inPos.zy, texFactor)).xyz);
	vec3 tnormalY = unpackNormal(texture(tex, unpackUV(inPos.xz, texFactor)).xyz);
	vec3 tnormalZ = unpackNormal(texture(tex, unpackUV(inPos.xy, texFactor)).xyz);
	
	// Fix X plane projection over positive X axis
	vec3 axis = sign(inNormal);
	tnormalX.x *= -axis.x;	
	
	// World space normals
	tnormalX = mat3(inTanX, inBTanX, inNormal) * tnormalX;	// TBN_X * tnormalX
	tnormalY = mat3(inTanY, inBTanY, inNormal) * tnormalY;	// TBN_Y * tnormalY
	tnormalZ = mat3(inTanZ, inBTanZ, inNormal) * tnormalZ;	// TBN_Z * tnormalZ
	
	// Weighted average
	vec3 weights = abs(inNormal);
	weights *= weights;
	weights /= weights.x + weights.y + weights.z;
		
	return normalize(tnormalX * weights.x  +  tnormalY * weights.y  +  tnormalZ * weights.z);
}

vec3 triplanarNormal_Sea(sampler2D tex, float texFactor, float speed)
{	
	// Get normal map coordinates
	float time = inTime * speed;
	
	vec2 coordsXY[4] = { 
		vec2(         inPos.x, time + inPos.y) / 4,
		vec2(         inPos.x, time + inPos.y) / 6,
		vec2(time + inPos.x,          inPos.y) / 5,
		vec2(time + inPos.x,          inPos.y) / 7 };

	vec2 coordsZY[4] = {
		vec2(         inPos.z, time + inPos.y) / 4,
		vec2(         inPos.z, time + inPos.y) / 6,
		vec2(time + inPos.z,          inPos.y) / 5,
		vec2(time + inPos.z,          inPos.y) / 7 };
		
	vec2 coordsXZ[4] = {
		vec2(         inPos.x, time + inPos.z) / 4,
		vec2(         inPos.x, time + inPos.z) / 6,
		vec2(time + inPos.x,          inPos.z) / 5,
		vec2(time + inPos.x,          inPos.z) / 7 };
		
	// Tangent space normal maps (retrieved using triplanar UVs; i.e., 3 facing planes)
	vec3 tnormalX = {0,0,0};
	vec3 tnormalY = {0,0,0};
	vec3 tnormalZ = {0,0,0};
	int i = 0;
		
	for(i = 0; i < 4; i++) 
		tnormalX += unpackNormal(texture(tex, unpackUV(coordsZY[i], texFactor)).xyz);
	tnormalX = normalize(tnormalX);

	for(i = 0; i < 4; i++) 
		tnormalY += unpackNormal(texture(tex, unpackUV(coordsXZ[i], texFactor)).xyz);
	tnormalY = normalize(tnormalY);
		
	for(i = 0; i < 4; i++)
		tnormalZ += unpackNormal(texture(tex, unpackUV(coordsXY[i], texFactor)).xyz);
	tnormalZ = normalize(tnormalZ);
	
	// Tangent space normal maps (retrieved using triplanar UVs; i.e., 3 facing planes)
	//vec3 tnormalX = unpackNormal(texture(tex, unpackUV(inPos.zy, texFactor)).xyz);
	//vec3 tnormalY = unpackNormal(texture(tex, unpackUV(inPos.xz, texFactor)).xyz);
	//vec3 tnormalZ = unpackNormal(texture(tex, unpackUV(inPos.xy, texFactor)).xyz);
	
	// Fix X plane projection over positive X axis
	vec3 axis = sign(inNormal);
	tnormalX.x *= -axis.x;	
	
	// World space normals
	tnormalX = mat3(inTanX, inBTanX, inNormal) * tnormalX;	// TBN_X * tnormalX
	tnormalY = mat3(inTanY, inBTanY, inNormal) * tnormalY;	// TBN_Y * tnormalY
	tnormalZ = mat3(inTanZ, inBTanZ, inNormal) * tnormalZ;	// TBN_Z * tnormalZ
	
	// Weighted average
	vec3 weights = abs(inNormal);
	weights *= weights;
	weights /= weights.x + weights.y + weights.z;
		
	return normalize(tnormalX * weights.x  +  tnormalY * weights.y  +  tnormalZ * weights.z);
}

vec3 triplanarNormal(sampler2D tex, uvGradient dzy, uvGradient dxz, uvGradient dxy)
{	
	// Tangent space normal maps (retrieved using triplanar UVs; i.e., 3 facing planes)
	vec3 tnormalX = unpackNormal(texture(tex, dzy.uv).xyz);
	vec3 tnormalY = unpackNormal(texture(tex, dxz.uv).xyz);
	vec3 tnormalZ = unpackNormal(texture(tex, dxy.uv).xyz);
	
	// Fix X plane projection over positive X axis
	vec3 axis = sign(inNormal);
	tnormalX.x *= -axis.x;	
	
	// World space normals
	tnormalX = mat3(inTanX, inBTanX, inNormal) * tnormalX;	// TBN_X * tnormalX
	tnormalY = mat3(inTanY, inBTanY, inNormal) * tnormalY;	// TBN_Y * tnormalY
	tnormalZ = mat3(inTanZ, inBTanZ, inNormal) * tnormalZ;	// TBN_Z * tnormalZ
	
	// Weighted average
	vec3 weights = abs(inNormal);
	weights *= weights;
	weights /= weights.x + weights.y + weights.z;
		
	return normalize(tnormalX * weights.x  +  tnormalY * weights.y  +  tnormalZ * weights.z);
}

vec3 toRGB(vec3 vec)
{
	vec3 linear;
	
	if (vec.x <= 0.04045) linear.x = vec.x / 12.92;
	else linear.x = pow((vec.x + 0.055) / 1.055, 2.4);
	
	if (vec.y <= 0.04045) linear.y = vec.y / 12.92;
	else linear.y = pow((vec.y + 0.055) / 1.055, 2.4);
	
	if (vec.z <= 0.04045) linear.z = vec.z / 12.92;
	else linear.z = pow((vec.z + 0.055) / 1.055, 2.4);
	
	return linear;
}

vec3 toSRGB(vec3 vec)
{
	vec3 nonLinear;
	
	if (vec.x <= 0.0031308) nonLinear.x = vec.x * 12.92;
	else nonLinear.x = 1.055 * pow(vec.x, 1.0/2.4) - 0.055;
	
	if (vec.y <= 0.0031308) nonLinear.y = vec.y * 12.92;
	else nonLinear.y = 1.055 * pow(vec.y, 1.0/2.4) - 0.055;
	
	if (vec.z <= 0.0031308) nonLinear.z = vec.z * 12.92;
	else nonLinear.z = 1.055 * pow(vec.z, 1.0/2.4) - 0.055;
	
	return nonLinear;
}

uvGradient getGradients(vec2 uvs)
{
	uvGradient result;
	result.uv = uvs;
	result.dFdx = dFdx(uvs);
	result.dFdy = dFdy(uvs);
	return result;
}

vec3 applyLinearFog(vec3 fragColor, vec3 fogColor, float minDist, float maxDist)
{
	float minSqrRadius = minDist * minDist;
	float maxSqrRadius = maxDist * maxDist;
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    if(sqrDist > maxSqrRadius) return fogColor;
    else
    {
        float ratio  = (sqrDist - minSqrRadius) / (maxSqrRadius - minSqrRadius);
        return fragColor * (1-ratio) + fogColor * ratio;
    }
}

float applyLinearFog(float value, float fogValue, float minDist, float maxDist)
{
	float minSqrRadius = minDist * minDist;
	float maxSqrRadius = maxDist * maxDist;
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    if(sqrDist > maxSqrRadius) return fogValue;
    else
    {
        float ratio  = (sqrDist - minSqrRadius) / (maxSqrRadius - minSqrRadius);
        return value * (1-ratio) + fogValue * ratio;
    }
}

vec3 applyFog(vec3 fragColor, vec3 fogColor)
{
	float coeff[3] = { 1, 0.000000000001, 0.000000000001 };		// coefficients  ->  a + b*dist + c*dist^2
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	
	float attenuation = 1.0 / (coeff[0] + coeff[1] * sqrDist + coeff[2] * sqrDist * sqrDist);
	return fragColor * attenuation + fogColor * (1. - attenuation);
}

float applyFog(float value, float fogValue)
{
	float coeff[3] = { 1, 0.000000000001, 0.000000000001 };		// coefficients  ->  a + b*dist + c*dist^2
	vec3 diff = inPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	
	float attenuation = 1.0 / (coeff[0] + coeff[1] * sqrDist + coeff[2] * sqrDist * sqrDist);
	return value * attenuation + fogValue * (1. - attenuation);
}

float getTexScaling(float initialTexFactor, float baseDist, float mixRange, inout float texFactor1, inout float texFactor2)
{
	// Compute current and next step
	float linearStep = 1 + floor(inDist / baseDist);	// Linear step [1, inf)
	float quadraticStep = ceil(log(linearStep) / log(2));
	float step[2];
	step[0] = pow (2, quadraticStep);					// Exponential step [0, inf)
	step[1] = pow(2, quadraticStep + 1);				// Next exponential step
	
	// Get texture resolution for each section
	texFactor1 = step[0] * initialTexFactor;
	texFactor2 = step[1] * initialTexFactor;
	
	// Get mixing ratio
	float maxDist = baseDist * step[0];
	mixRange = mixRange * maxDist;						// mixRange is now an absolute value (not percentage)
	return clamp((maxDist - inDist) / mixRange, 0.f, 1.f);
}

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((inGroundHeight - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}

float modulus(float dividend, float divider) { return dividend - (divider * floor(dividend/divider)); }
