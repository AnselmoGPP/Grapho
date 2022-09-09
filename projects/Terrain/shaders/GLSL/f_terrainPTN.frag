#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    int lightType;		// int   0: no light   1: directional   2: point   3: spot
	
    vec4 position;		// vec3
    vec4 direction;		// vec3

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
    Light light;
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[41];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec2 inUVCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangCamPos;
layout(location = 4) in vec3 inTangLightPos;
layout(location = 5) in vec3 inTangLightDir;
layout(location = 6) in float inSlope;

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

vec3 applyFog		 (vec3 fragment);
void getTex			 (inout vec3 result, int albedo, int normal, int specular, int roughness);
void getTex_Grid     (inout vec3 result);
void getTex_Grass1   (inout vec3 result);
void getTex_Tech     (inout vec3 result);
void getTex_Sand     (inout vec3 result);
void getTex_GrassRock(inout vec3 result);

void main()
{
	//outColor = vec4(inColor, 1.0);
	//outColor = texture(texSampler[0], inUVCoord);
	//outColor = vec4(inColor * texture(texSampler, inUVCoord).rgb, 1.0);

	vec3 color;

	//getTex(color, 1, 2, 3, 4);
	//getTex_Grid(color);
	//getTex_Grass1(color);
	//getTex_Tech(color);
	getTex_Sand(color);
	//getTex_GrassRock(color);

    //color = applyFog(color);
	
	outColor = vec4(color, 1.0);
}


vec3 directionalLightColor(Light light, vec3 albedo, vec3 normal, vec3 specularity, float roughness)
{
    // ----- Ambient lighting -----
    vec3 ambient = light.ambient.xyz * albedo;

    // ----- Diffuse lighting -----
    float diff = max(dot(normal, -inTangLightDir), 0.0);	// <<<
    vec3 diffuse = light.diffuse.xyz * diff * albedo;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(inTangCamPos - inFragPos);
    vec3 reflectDir = reflect(inTangLightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), roughness);
    vec3 specular = light.specular.xyz * spec * specularity;
	
    // ----- Result -----
	return vec3(ambient + diffuse + specular);
}


vec3 PointLightColor( Light light, vec3 diffuseMap, vec3 normalMap, vec3 specularMap, float shininess)
{
    float distance = length(light.position.xyz - inFragPos);
    float attenuation = 1.0 / (light.degree[0] + light.degree[1] * distance + light.degree[2] * distance * distance);
	vec3 fragLightDir = normalize(light.position.xyz - inFragPos);
	vec3 norm = normalize(inNormal);

    // ----- Ambient lighting -----
    vec3 ambient = light.ambient.xyz * diffuseMap * attenuation;

    // ----- Diffuse lighting -----
    float diff = max(dot(norm, fragLightDir), 0.0);
    vec3 diffuse = light.diffuse.xyz * diff * diffuseMap * attenuation;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(inTangCamPos.xyz - inFragPos);
    vec3 reflectDir = reflect(-fragLightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular.xyz * spec * specularMap * attenuation;

    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}


vec3 SpotLightColor( Light light, vec3 diffuseMap, vec3 normalMap, vec3 specularMap, float shininess)
{
    float distance = length(light.position.xyz - inFragPos);
    float attenuation = 1.0 / (light.degree[0] + light.degree[1] * distance + light.degree[2] * distance * distance);
    vec3 fragLightDir = normalize(light.position.xyz - inFragPos);
    vec3 norm = normalize(inNormal);
	
    // ----- Ambient lighting -----
    vec3 ambient = light.ambient.xyz * diffuseMap * attenuation;

    // ----- Diffuse lighting -----
    float theta = dot(fragLightDir, normalize(light.direction.xyz));	// The closer to 1, the more direct the light gets to fragment.
    if(theta < light.cutOff[1]) return vec3(ambient);

    float epsilon = light.cutOff[0] - light.cutOff[1];
    float intensity = clamp((theta - light.cutOff[1]) / epsilon, 0.0, 1.0);
    float diff = max(dot(norm, fragLightDir), 0.0);
    vec3 diffuse = light.diffuse.xyz * diff * diffuseMap * attenuation * intensity;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(inTangCamPos.xyz - inFragPos);
    vec3 reflectDir = reflect(-fragLightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular.xyz * spec * specularMap * attenuation * intensity;

    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}

// Apply the lighting type you want to a fragment
vec3 getFragColor(vec3 diffuseMap, vec3 normalMap, vec3 specularMap, float shininess)
{
	if(ubo.light.lightType == 1)
		return directionalLightColor(ubo.light, diffuseMap, normalMap, specularMap, shininess);
	else if(ubo.light.lightType == 2)
		return PointLightColor(ubo.light, diffuseMap, normalMap, specularMap, shininess);
	else if(ubo.light.lightType == 3)
		return SpotLightColor(ubo.light, diffuseMap, normalMap, specularMap, shininess);
	else
		return diffuseMap;
}

void getTex (inout vec3 result, int albedo, int normal, int specular, int roughness)
{
		result   = getFragColor(
					texture(texSampler[albedo], inUVCoord/10).rgb,
					normalize(texture(texSampler[normal], inUVCoord/10).rgb * 2.f - 1.f).rgb,
					texture(texSampler[specular], inUVCoord/10).rgb, 
					texture(texSampler[roughness], inUVCoord/10).r * 255 );
}

void getTex_Grid(inout vec3 result)
{
	result = getFragColor(texture(texSampler[0], inUVCoord/10).rgb, inNormal, vec3(0.1, 0.1, 0.1), 0.4);
}

void getTex_Grass1(inout vec3 result)
{
	result   = getFragColor(
					texture(texSampler[9], inUVCoord/10).rgb,
					normalize(texture(texSampler[10], inUVCoord/10).rgb * 2.f - 1.f).rgb,
					texture(texSampler[11], inUVCoord/10).rgb, 
					texture(texSampler[12], inUVCoord/10).r * 255 );
}

void getTex_Tech(inout vec3 result)
{
		result   = getFragColor(
					texture(texSampler[13], inUVCoord/100).rgb,
					normalize(texture(texSampler[14], inUVCoord/100).rgb * 2.f - 1.f).rgb,
					texture(texSampler[15], inUVCoord/100).rgb, 
					texture(texSampler[16], inUVCoord/100).r * 255 );
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
						normalize(texture(texSampler[18], inUVCoord/tf).rgb * 2.f - 1.f).rgb,
						texture(texSampler[19], inUVCoord/tf).rgb, 
						texture(texSampler[20], inUVCoord/tf).r * 255 );
						
	vec3 plainFrag = getFragColor(
						texture(texSampler[21], inUVCoord/tf).rgb,
						normalize(texture(texSampler[22], inUVCoord/tf).rgb * 2.f - 1.f).rgb,
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
	
    float squareDistance = (inFragPos.x - inTangCamPos.x) * (inFragPos.x - inTangCamPos.x) +
                           (inFragPos.y - inTangCamPos.y) * (inFragPos.y - inTangCamPos.y) +
                           (inFragPos.z - inTangCamPos.z) * (inFragPos.z - inTangCamPos.z);

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


// modulus(%) = a - (b * floor(a/b))