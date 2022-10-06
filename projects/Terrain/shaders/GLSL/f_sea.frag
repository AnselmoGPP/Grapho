#version 450
#extension GL_ARB_separate_shader_objects : enable

#define numLights 2

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

// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
layout(set = 0, binding = 1) uniform ubobject
{
	vec4 time;				// float
	LightProps light[2];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[1];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3  inFragPos;
layout(location = 1) in vec3  inCamPos;
layout(location = 2) in LightPD inLight[numLights];

layout(location = 0) out vec4 outColor;								// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

// Declarations:

vec3  getFragColor	(vec3 albedo, vec3 normal, vec3 specularity, float roughness);
void  getTex		(inout vec3 result, int albedo, int normal, int specular, int roughness, float scale);
vec3  toRGB			(vec3 vec);							// Transforms non-linear sRGB color to linear RGB. Note: Usually, input is non-linear sRGB, but it's automatically converted to linear RGB in the shader, and output later in sRGB.
vec3  toSRGB		(vec3 vec);							// Transforms linear RGB color to non-linear sRGB
vec3  applyLinearFog(vec3 fragColor, vec3 fogColor, float minDist, float maxDist);
float applyLinearFog(float value, float fogValue, float minDist, float maxDist);
vec3  applyFog		(vec3 fragColor, vec3 fogColor);
float applyFog		(float value,   float fogValue);
float modulus		(float dividend, float divider);	// modulus(%) = a - (b * floor(a/b))
vec3 directionalLightColor(int i, vec3 albedo, vec3 normal, vec3 specularity, float roughness);

// Definitions:

void main()
{
	//outColor.xyz = getFragColor(vec3(0, 0, 1), vec3(0, 0, 1), vec3(0.3, 0.3, 0.3), 2.f);

	outColor.w   = 0.7;
	outColor.xyz = getFragColor(
						vec3(1./255., 137./255., 249./255.),
						normalize(
							toSRGB(texture(texSampler[0], vec2(               inFragPos.x,  ubo.time[0] + inFragPos.y)/40).rgb) * 2.f - 1.f +  
							toSRGB(texture(texSampler[0], vec2(               inFragPos.x, -ubo.time[0] + inFragPos.y)/60).rgb) * 2.f - 1.f +
							toSRGB(texture(texSampler[0], vec2( ubo.time[0] + inFragPos.x,                inFragPos.y)/50).rgb) * 2.f - 1.f +
							toSRGB(texture(texSampler[0], vec2(-ubo.time[0] + inFragPos.x,                inFragPos.y)/70).rgb) * 2.f - 1.f ).rgb,	
						vec3(0.3, 0.3, 0.3),
						2.f );
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
    float attenuation = 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distance + ubo.light[i].degree[2] * distance * distance);
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
    float attenuation = 1.0 / (ubo.light[i].degree[0] + ubo.light[i].degree[1] * distance + ubo.light[i].degree[2] * distance * distance);
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

	for(int i = 0; i < numLights; i++)		// for each light source
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
				texture(texSampler[albedo], inFragPos.xy/scale).rgb,
				normalize(toSRGB(texture(texSampler[normal], inFragPos.xy/scale).rgb) * 2.f - 1.f).rgb,
				texture(texSampler[specular], inFragPos.xy/scale).rgb, 
				texture(texSampler[roughness], inFragPos.xy/scale).r * 255 );
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