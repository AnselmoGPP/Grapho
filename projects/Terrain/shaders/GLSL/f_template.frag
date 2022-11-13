#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUMLIGHTS 2

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

layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
    vec4 time;				// float
	LightProps light[2];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[41];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3    inFragPos;				// Vertex position transformed with TBN matrix
layout(location = 1) in vec3    inPos;					// Vertex position not transformed with TBN matrix
layout(location = 2) in vec2    inUVCoord;
layout(location = 3) in vec3    inCamPos;
layout(location = 4) in vec3    inNormal;
layout(location = 5) in float   inSlope;
layout(location = 6) in float   inDist;
layout(location = 7) in float   inHeight;
layout(location = 8) in LightPD inLight[NUMLIGHTS];

layout(location = 0) out vec4   outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

// Declarations:

vec3  getFragColor	  (vec3 albedo, vec3 normal, vec3 specularity, float roughness);
void  getTex		  (inout vec3 result, int albedo, int normal, int specular, int roughness, float scale);
vec4 triplanarTexture (sampler2D tex, float texFactor);
vec4  triplanarNormal (sampler2D tex, sampler2D diffuse, sampler2D specularMap, float shininess);
vec3  toRGB			  (vec3 vec);							// Transforms non-linear sRGB color to linear RGB. Note: Usually, input is non-linear sRGB, but it's automatically converted to linear RGB in the shader, and output later in sRGB.
vec3  toSRGB		  (vec3 vec);							// Transforms linear RGB color to non-linear sRGB
vec3  applyLinearFog  (vec3 fragColor, vec3 fogColor, float minDist, float maxDist);
float applyLinearFog  (float value, float fogValue, float minDist, float maxDist);
vec3  applyFog		  (vec3 fragColor, vec3 fogColor);
float applyFog		  (float value,   float fogValue);
float modulus		  (float dividend, float divider);		// modulus(%) = a - (b * floor(a/b))

// Definitions:

void main()
{
	vec3 color;

	//outColor = vec4(inColor * texture(texSampler, inUVCoord).rgb, 1.0);
	//color = getFragColor(texture(texSampler[0], inUVCoord/10).rgb, vec3(0,0,1), vec3(0.1, 0.1, 0.1), 1);	// Grid
	//getTex(color,  1,  2,  3,  4, 10);																	// Green
	
	outColor = vec4(color, 1.0);
}





// Tools ---------------------------------------------------------------------------------------------


vec3 directionalLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
	// ----- Ambient lighting -----
	vec3 ambient = ubo.light[i].ambient.xyz * albedo;
	if(dot(inLight[i].direction.xyz, normal) > 0) return ambient;		// If light comes from below the tangent plane
	
	// ----- Diffuse lighting -----
	float diff   = max(dot(normal, -inLight[i].direction.xyz), 0.f);
	vec3 diffuse = ubo.light[i].diffuse.xyz * albedo * diff;		
	
	// ----- Specular lighting -----
	vec3 viewDir      = normalize(inCamPos - inFragPos);
	//vec3 reflectDir = normalize(reflect(inLight[i].direction.xyz, normal));
	//float spec	  = pow(max(dot(viewDir, reflectDir), 0.f), roughness);
	vec3 halfwayDir   = normalize(-inLight[i].direction.xyz + viewDir);
	float spec        = pow(max(dot(normal, halfwayDir), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec;
	
	// ----- Result -----
	return vec3(ambient + diffuse + specular);
}


vec3 PointLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    float distance    = length(inLight[i].position.xyz - inFragPos);
    float attenuation = 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distance + ubo.light[i].degree[2] * distance * distance);	// How light attenuates with distance
	vec3 lightDir = normalize(inFragPos - inLight[i].position.xyz);			// Direction from light source to fragment

    // ----- Ambient lighting -----
    vec3 ambient = ubo.light[i].ambient.xyz * albedo * attenuation;
	if(dot(lightDir, normal) > 0) return ambient;							// If light comes from below the tangent plane

    // ----- Diffuse lighting -----
    float diff   = max(dot(normal, -lightDir), 0.f);
    vec3 diffuse = ubo.light[i].diffuse.xyz * albedo * diff * attenuation;
	
    // ----- Specular lighting -----
	vec3 viewDir      = normalize(inCamPos - inFragPos);
	//vec3 reflectDir = normalize(reflect(lightDir, normal));
	//float spec      = pow(max(dot(viewDir, reflectDir), 0.f), roughness);
	vec3 halfwayDir   = normalize(-lightDir + viewDir);
	float spec        = pow(max(dot(normal, halfwayDir), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec * attenuation;
	
    // ----- Result -----
    return vec3(ambient + diffuse + specular);	
}


vec3 SpotLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    float distance = length(inLight[i].position.xyz - inFragPos);
    float attenuation = 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distance + ubo.light[i].degree[2] * distance * distance);	// How light attenuates with distance
    vec3 lightDir = normalize(inFragPos - inLight[i].position.xyz);			// Direction from light source to fragment

    // ----- Ambient lighting -----
    vec3 ambient = ubo.light[i].ambient.xyz * albedo * attenuation;
	if(dot(lightDir, normal) > 0) return ambient;							// If light comes from below the tangent plane

    // ----- Diffuse lighting -----
	float theta		= dot(lightDir, inLight[i].direction.xyz);	// The closer to 1, the more direct the light gets to fragment.
	float epsilon   = ubo.light[i].cutOff[0] - ubo.light[i].cutOff[1];
    float intensity = clamp((theta - ubo.light[i].cutOff[1]) / epsilon, 0.0, 1.0);
	float diff      = max(dot(normal, -lightDir), 0.f);
    vec3 diffuse    = ubo.light[i].diffuse.xyz * albedo * diff * attenuation * intensity;

    // ----- Specular lighting -----
	vec3 viewDir      = normalize(inCamPos - inFragPos);
	//vec3 reflectDir = normalize(reflect(lightDir, normal));
	//float spec      = pow(max(dot(viewDir, reflectDir), 0.f), roughness);
	vec3 halfwayDir   = normalize(-lightDir + viewDir);
	//float spec      = pow(max(dot(viewDir, reflectDir), 0.f), roughness * 4);
	float spec        = pow(max(dot(normal, halfwayDir), 0.0), roughness * 4);
	vec3 specular     = ubo.light[i].specular.xyz * specularity * spec * attenuation * intensity;
	
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

void getTex(inout vec3 result, int albedo, int normal, int specular, int roughness, float scale)
{
	result   = getFragColor(
				texture(texSampler[albedo], inUVCoord/scale).rgb,
				normalize(toSRGB(texture(texSampler[normal], inUVCoord/scale).rgb) * 2.f - 1.f).rgb,
				texture(texSampler[specular], inUVCoord/scale).rgb, 
				texture(texSampler[roughness], inUVCoord/scale).r * 255 );
}

vec4 triplanarTexture(sampler2D tex, float texFactor)
{
	vec4 dx = texture(tex, inPos.zy / texFactor);
	vec4 dy = texture(tex, inPos.xz / texFactor);
	vec4 dz = texture(tex, inPos.xy / texFactor);
	
	vec3 weights = abs(normalize(inNormal));
	weights /= weights.x + weights.y + weights.z;

	return dx * weights.x + dy * weights.y + dz * weights.z;
}

vec4 triplanarNormal(sampler2D tex, sampler2D diffuse, sampler2D specularMap, float shininess)
{
/*
	float tf = 50;            // texture factor
	
	vec3 tx = texture(tex, inPosition.zy / tf);
	vec3 ty = texture(tex, inPosition.xz / tf);
	vec3 tz = texture(tex, inPosition.xy / tf);

	vec3 weights = abs(inNormal);
	weights *= weights;
	weights /= weights.x + weights.y + weights.z;
	
	vec3 axis = sign(normal);
	vec3 tangentX = normalize(cross(inNormal, vec3(0., axis.x, 0.)));
	vec3 bitangentX = normalize(cross(tangentX, inNormal)) * axis.x;
	mat3 tbnX = mat3(tangentY, bitangentY, inNormal);

	vec3 tangentY = normalize(cross(inNormal, vec3(0., 0., axis.y)));
	vec3 bitangentY = normalize(cross(tangentY, inNormal)) * axis.y;
	mat3 tbnY = mat3(tangentY, bitangentY, inNormal);

	vec3 tangentZ = normalize(cross(inNormal, vec3(0., -axis.z, 0.)));
	vec3 bitangentZ = normalize(-cross(tangentZ, inNormal)) * axis.z;
	mat3 tbnZ = mat3(tangentZ, bitangentZ, inNormal);
	
	vec3 worldNormal = normalize (
		clamp(tbnX * tx, -1., 1.) * weights.x +
		clamp(tbny * ty, -1., 1.) * weights.y +
		clamp(tbnZ * tz, -1., 1.) * weights.z);
	
	return vec4(worldNormal, 0.);
*/
	return vec4(0.);
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

vec3 applyLinearFog(vec3 fragColor, vec3 fogColor, float minDist, float maxDist)
{
	float minSqrRadius = minDist * minDist;
	float maxSqrRadius = maxDist * maxDist;
	vec3 diff = inFragPos - inCamPos;
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
	vec3 diff = inFragPos - inCamPos;
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
	vec3 diff = inFragPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	
	float attenuation = 1.0 / (coeff[0] + coeff[1] * sqrDist + coeff[2] * sqrDist * sqrDist);
	return fragColor * attenuation + fogColor * (1. - attenuation);
}

float applyFog(float value, float fogValue)
{
	float coeff[3] = { 1, 0.000000000001, 0.000000000001 };		// coefficients  ->  a + b*dist + c*dist^2
	vec3 diff = inFragPos - inCamPos;
	float sqrDist  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	
	float attenuation = 1.0 / (coeff[0] + coeff[1] * sqrDist + coeff[2] * sqrDist * sqrDist);
	return value * attenuation + fogValue * (1. - attenuation);
}

float modulus(float dividend, float divider) { return dividend - (divider * floor(dividend/divider)); }
