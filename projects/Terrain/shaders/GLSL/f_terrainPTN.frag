#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    int type;			// int   0: no light   1: directional   2: point   3: spot
	
    vec4 position;		// vec3
    vec4 direction;		// vec3

    vec4 ambient;		// vec3
    vec4 diffuse;		// vec3
    vec4 specular;		// vec3

    vec4 degree;		// vec3	(constant, linear, quadratic)
    vec4 cutOff;		// vec2 (cuttOff, outerCutOff)
};

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

struct Material
{
    //sampler2D diffuseT;	// object color
    vec4 diffuse;			// vec3
    //sampler2D specularT;	// specular map
    vec4 specular;			// vec3
    vec4 shininess;			// float
};

// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
layout(set = 0, binding = 1) uniform ubobject
{
    vec4 time;				// float
	ivec4 numLights;		// int
	LightProps light[2];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[41];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3  inFragPos;
layout(location = 1) in vec2  inUVCoord;
layout(location = 2) in vec3  inNormal;
layout(location = 3) in vec3  inCamPos;
layout(location = 4) in float inSlope;
layout(location = 5) in LightPD inLight[2];

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

vec3 applyFog		 (vec3 fragment);
void getTex			 (inout vec3 result, int albedo, int normal, int specular, int roughness);
void getTex_Grid     (inout vec3 result);
void getTex_Grass1   (inout vec3 result);
void getTex_Tech     (inout vec3 result);
void getTex_Sand     (inout vec3 result);
void getTex_GrassRock(inout vec3 result);
vec3 toRGB			 (vec3 vec);		// Transforms non-linear sRGB color to linear RGB. Usually, input is non-linear sRGB, but it's automatically converted to linear RGB in the shader, and output later in sRGB.
vec3 toSRGB			 (vec3 vec);		// Transforms linear RGB color to non-linear sRGB

void main()
{
	//outColor = vec4(inColor, 1.0);
	//outColor = texture(texSampler[0], inUVCoord);
	//outColor = vec4(inColor * texture(texSampler, inUVCoord).rgb, 1.0);

	vec3 color;

	//getTex(color, 6, 7, 8, 9);
	//getTex_Grid(color);
	//getTex_Grass1(color);
	getTex_Tech(color);
	//getTex_Sand(color);
	//getTex_GrassRock(color);

    //color = applyFog(color);
	
	outColor = vec4(color, 1.0);
}

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
	vec3 result = vec3(0,0,0);

	for(int i = 0; i < ubo.numLights[0]; i++)		// for each light source
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

void getTex (inout vec3 result, int albedo, int normal, int specular, int roughness)
{
	result   = getFragColor(
				texture(texSampler[albedo], inUVCoord/10).rgb,
				normalize(toSRGB(texture(texSampler[normal], inUVCoord/10).rgb) * 2.f - 1.f).rgb,
				texture(texSampler[specular], inUVCoord/10).rgb, 
				texture(texSampler[roughness], inUVCoord/10).r * 255 );
}

void getTex_Grid(inout vec3 result)
{
	result = getFragColor(
				texture(texSampler[0], inUVCoord/10).rgb, 
				inNormal, 
				vec3(0.1, 0.1, 0.1), 
				1);
}

void getTex_Grass1(inout vec3 result)
{
	result   = getFragColor(
					texture(texSampler[5], inUVCoord/10).rgb,
					normalize(toSRGB(texture(texSampler[6], inUVCoord/10).rgb) * 2.f - 1.f).rgb,
					texture(texSampler[7], inUVCoord/10).rgb, 
					texture(texSampler[8], inUVCoord/10).r * 255 );
}

void getTex_Tech(inout vec3 result)
{
		result   = getFragColor(
					texture(texSampler[37], inUVCoord/100).rgb,
					normalize(toSRGB(texture(texSampler[38], inUVCoord/100).rgb) * 2.f - 1.f).rgb,
					texture(texSampler[39], inUVCoord/100).rgb, 
					texture(texSampler[40], inUVCoord/100).r * 255 );
}

void getTex_Sand(inout vec3 result)
{
    float slopeThreshold = 0.3;           // sand-plainSand slope threshold
    float mixRange       = 0.1;           // threshold mixing range (slope range)
    float tf             = 50;            // texture factor
    //float slope = dot( normalize(inNormal), normalize(vec3(inNormal.x, inNormal.y, 0.0)) );

	float ratio;
	if (inSlope < slopeThreshold - mixRange) ratio = 0;
	else if(inSlope > slopeThreshold + mixRange) ratio = 1;
	else ratio = (inSlope - (slopeThreshold - mixRange)) / (2 * mixRange);
		
	vec3 sandFrag = getFragColor(
						texture(texSampler[17], inUVCoord/tf).rgb,
						normalize(toSRGB(texture(texSampler[18], inUVCoord/tf).rgb) * 2.f - 1.f).rgb,
						texture(texSampler[19], inUVCoord/tf).rgb, 
						texture(texSampler[20], inUVCoord/tf).r * 255 );
						
	vec3 plainFrag = getFragColor(
						texture(texSampler[21], inUVCoord/tf).rgb,
						normalize(toSRGB(texture(texSampler[22], inUVCoord/tf).rgb) * 2.f - 1.f).rgb,
						texture(texSampler[23], inUVCoord/tf).rgb, 
						texture(texSampler[24], inUVCoord/tf).r * 255 );

	result = (ratio) * plainFrag + (1-ratio) * sandFrag;
}

void getTex_GrassRock(inout vec3 result)
{
	float slopeThreshold = 0.5;           // grass-rock slope threshold
    float mixRange       = 0.05;          // threshold mixing range (slope range)
    float rtf            = 30;            // rock texture factor
    float gtf            = 20;            // grass texture factor

    float maxSnowLevel   = 80;            // maximum snow height (up from here, there's only snow within the maxSnowSlopw)
    float minSnowLevel   = 50;            // minimum snow height (down from here, there's zero snow)
    float maxSnowSlope   = 0.90;          // maximum slope where snow can rest
    float snowSlope      = maxSnowSlope * ( (inFragPos.z - minSnowLevel) / (maxSnowLevel - minSnowLevel) );
    float mixSnowRange   = 0.1;           // threshold mixing range (slope range)

    if(snowSlope > maxSnowSlope) snowSlope = maxSnowSlope;
    float slope = dot( normalize(inNormal), normalize(vec3(inNormal.x, inNormal.y, 0.0)) );
/*
    // >>> SNOW
    if(slope < snowSlope)
    {
        vec4 snowFrag = getFragColor( sun, snow.diffuse, snow.specular, snow.shininess, 1.0 );

        // >>> MIXTURE (SNOW + GRASS + ROCK)
        if(slope > (snowSlope - mixSnowRange) && slope < snowSlope)
        {
            vec4 rockFrag  = getFragColor( sun, vec3(texture(rock.diffuseT, TexCoord/rtf)), vec3(texture(rock.specularT, TexCoord/rtf)), rock.shininess, 1.0 );
            vec4 grassFrag = getFragColor( sun, vec3(texture(grass.diffuseT, TexCoord/gtf)), vec3(texture(grass.specularT, TexCoord/gtf)), grass.shininess, 1.0 );
            float ratio    = (slope - (slopeThreshold - mixRange)) / (2 * mixRange);
            if(ratio < 0) ratio = 0;
            else if(ratio > 1) ratio = 1;
            vec3 mixGround = rockFrag.xyz * ratio + grassFrag.xyz * (1-ratio);

            ratio      = (snowSlope - slope) / mixSnowRange;
            if(ratio < 0) ratio = 0;
            else if (ratio > 1) ratio = 1;
            vec3 mix   = mixGround.xyz * (1-ratio) + snowFrag.xyz * ratio;
            snowFrag   = vec4( mix, 1.0 );
        }

        result = snowFrag;
    }

    // >>> GRASS
    else if (slope < slopeThreshold - mixRange)
        result = getFragColor( sun, vec3(texture(grass.diffuseT, TexCoord/gtf)), vec3(texture(grass.specularT, TexCoord/gtf)), grass.shininess, 1.0 );

    // >>> ROCK
    else if(slope > slopeThreshold + mixRange)
        result = getFragColor( sun, vec3(texture(rock.diffuseT, TexCoord/rtf)), vec3(texture(rock.specularT, TexCoord/rtf)), rock.shininess, 1.0 );

    // >>> MIXTURE (GRASS + ROCK)
    else if(slope >= slopeThreshold - mixRange && slope <= slopeThreshold + mixRange)
    {
        vec4 rockFrag  = getFragColor( sun, vec3(texture(rock.diffuseT, TexCoord/rtf)), vec3(texture(rock.specularT, TexCoord/rtf)), rock.shininess, 1.0 );
        vec4 grassFrag = getFragColor( sun, vec3(texture(grass.diffuseT, TexCoord/gtf)), vec3(texture(grass.specularT, TexCoord/gtf)), grass.shininess, 1.0 );

        float ratio    = (slope - (slopeThreshold - mixRange)) / (2 * mixRange);
        vec3 mixGround = rockFrag.xyz * ratio + grassFrag.xyz * (1-ratio);

        result = vec4(mixGround, 1.0);
    }
*/
}

vec3 applyFog(vec3 fragment)
{
	float fogMinSquareRadius = 1000;
	float fogMaxSquareRadius = 5000;
	vec3 skyColor = {0, 0, 0};
	//float distance = length(inTangCamPos - inFragPos);
	
    float squareDistance = (inFragPos.x - inCamPos.x) * (inFragPos.x - inCamPos.x) +
                           (inFragPos.y - inCamPos.y) * (inFragPos.y - inCamPos.y) +
                           (inFragPos.z - inCamPos.z) * (inFragPos.z - inCamPos.z);

    if(squareDistance > fogMinSquareRadius)
        if(squareDistance > fogMaxSquareRadius)
            fragment = skyColor;
        else
    {
        float ratio  = (squareDistance - fogMinSquareRadius) / (fogMaxSquareRadius - fogMinSquareRadius);
        fragment = vec3(fragment * (1-ratio) + skyColor * ratio);
    }

    return fragment;
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


// modulus(%) = a - (b * floor(a/b))