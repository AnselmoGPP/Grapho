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
    vec4 diffuse;			// vec3
    vec4 specular;			// vec3
    vec4 shininess;			// float
	
	//sampler2D diffuse;
	//sampler2D normal;
	//sampler2D specular;
	//sampler2D shininess;
};

layout(set = 0, binding = 1) uniform dataBlock		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
    Light light;
	vec4 camPos;		// vec3
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[9];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

vec3 applyFog			  (vec3 fragment);
void getTexture_Grid      (inout vec3 result);
void getTexture_Sand      (inout vec3 result);
void getTexture_GrassRock (inout vec3 result);

void main()
{
	//outColor = vec4(inColor, 1.0);
	//outColor = texture(texSampler[0], inTexCoord);
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);

	vec3 color;
	
	//getTexture_Grid(color);
	getTexture_Sand(color);
	//getTerrainTexture_GrassRock(color);

    //color = applyFog(color);
	
	outColor = vec4(color, 1.0);
}


vec3 directionalLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess)
{
	vec3 fragLightDir = normalize(light.direction.xyz);
	vec3 norm = normalize(inNormal);
	
    // ----- Ambient lighting -----
    vec3 ambient = light.ambient.xyz * diffuseMap;

    // ----- Diffuse lighting -----
    float diff = max(dot(norm, fragLightDir), 0.0);
    vec3 diffuse = light.diffuse.xyz * diff * diffuseMap;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(ubo.camPos.xyz - inPosition);
    vec3 reflectDir = reflect(-fragLightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular.xyz * spec * specularMap;

    // ----- Result -----
	return vec3(ambient + diffuse + specular);
}


vec3 PointLightColor( Light light, vec3 diffuseMap, vec3 specularMap, float shininess)
{
    float distance = length(light.position.xyz - inPosition);
    float attenuation = 1.0 / (light.degree[0] + light.degree[1] * distance + light.degree[2] * distance * distance);
	vec3 fragLightDir = normalize(light.position.xyz - inPosition);
	vec3 norm = normalize(inNormal);

    // ----- Ambient lighting -----
    vec3 ambient = light.ambient.xyz * diffuseMap * attenuation;

    // ----- Diffuse lighting -----
    float diff = max(dot(norm, fragLightDir), 0.0);
    vec3 diffuse = light.diffuse.xyz * diff * diffuseMap * attenuation;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(ubo.camPos.xyz - inPosition);
    vec3 reflectDir = reflect(-fragLightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular.xyz * spec * specularMap * attenuation;

    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}


vec3 SpotLightColor( Light light, vec3 diffuseMap, vec3 specularMap, float shininess)
{
    float distance = length(light.position.xyz - inPosition);
    float attenuation = 1.0 / (light.degree[0] + light.degree[1] * distance + light.degree[2] * distance * distance);
    vec3 fragLightDir = normalize(light.position.xyz - inPosition);
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
    vec3 viewDir = normalize(ubo.camPos.xyz - inPosition);
    vec3 reflectDir = reflect(-fragLightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular.xyz * spec * specularMap * attenuation * intensity;

    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}

// Apply the lighting type you want to a fragment
vec3 getFragColor(vec3 diffuseMap, vec3 specularMap, float shininess)
{
	if(ubo.light.lightType == 1)
		return directionalLightColor(ubo.light, diffuseMap, specularMap, shininess);
	else if(ubo.light.lightType == 2)
		return PointLightColor(ubo.light, diffuseMap, specularMap, shininess);
	else if(ubo.light.lightType == 3)
		return SpotLightColor(ubo.light, diffuseMap, specularMap, shininess);
	else
		return diffuseMap;
}

vec4 triplanarTexture(sampler2D tex)
{
	float tf = 50;            // texture factor

	vec4 dx = texture(tex, (inPosition.zy + vec2(1., 7.)) / tf);
	vec4 dy = texture(tex, (inPosition.xz + vec2(.2, .6)) / tf);
	vec4 dz = texture(tex, (inPosition.xy + vec2(.8, .3)) / tf);
	
	vec3 weights = abs(inNormal);
	weights /= (weights.x + weights.y + weights.z);
	
	return dx * weights.x + dy * weights.y + dz * weights.z;
}

vec4 triplanarNormal(sampler2D diffuse, sampler2D specularMap, float shininess)
{
	float tf = 50;            // texture factor
	
	vec2 uvX = inPosition.zy;	// x facing plane
	vec2 uvY = inPosition.xz;	// y facing plane
	vec2 uvZ = inPosition.xy;	// z facing plane
	
	// ...
	return vec4(0);
}

void getTexture_Grid(inout vec3 result)
{
	result = getFragColor(texture(texSampler[0], inTexCoord).rgb, vec3(0.1, 0.1, 0.1), 0.4);
}

void getTexture_Sand(inout vec3 result)
{
    float slopeThreshold = 0.3;           // sand-plainSand slope threshold
    float mixRange       = 0.1;           // threshold mixing range (slope range)
    float tf             = 50;            // texture factor

	//precision highp float;
	//vec3 normal = normalize(normalize(inNormal));
	//float slope = dot( inNormal, normalize(vec3(inNormal.x, inNormal.y, 0)) );
	vec3 normalXradius = cross(inNormal, inPosition);
	vec3 radiusXprevious = cross(inPosition, normalXradius);
	float slope = dot( inNormal, normalize(radiusXprevious) );

    // >>> DESERT
    if (slope < slopeThreshold - mixRange)
        //result = getFragColor(texture(texSampler[5], inPosition.xy/tf).rgb, texture(texSampler[6], inPosition.xy/tf).rgb, 1.0);
        result = getFragColor(triplanarTexture(texSampler[5]).rgb, triplanarTexture(texSampler[6]).rgb, 1.0);

    // >>> PLAIN
    else if(slope > slopeThreshold + mixRange)
        //result = getFragColor(texture(texSampler[7], inPosition.xy/tf).rgb, texture(texSampler[8], inPosition.xy/tf).rgb, 1.0);
		result = getFragColor(triplanarTexture(texSampler[7]).rgb, triplanarTexture(texSampler[8]).rgb, 1.0);

    // >>> MIXTURE
    else if(slope >= slopeThreshold - mixRange && slope <= slopeThreshold + mixRange)
    {
	    //vec3 sandFrag  = getFragColor(texture(texSampler[5], inPosition.xy/tf).rgb, texture(texSampler[6], inPosition.xy/tf).rgb, 1.0);
        //vec3 plainFrag = getFragColor(texture(texSampler[7], inPosition.xy/tf).rgb, texture(texSampler[8], inPosition.xy/tf).rgb, 1.0);
		vec3 sandFrag = getFragColor(triplanarTexture(texSampler[5]).rgb, triplanarTexture(texSampler[6]).rgb, 1.0);
		vec3 plainFrag = getFragColor(triplanarTexture(texSampler[7]).rgb, triplanarTexture(texSampler[8]).rgb, 1.0);

        float ratio    = (slope - (slopeThreshold - mixRange)) / (2 * mixRange);
        result = plainFrag.xyz * ratio + sandFrag.xyz * (1-ratio);
    }
	else result = vec3(0.5, 0.5, 0.5);
}

void getTexture_GrassRock(inout vec3 result)
{
	float slopeThreshold = 0.5;           // grass-rock slope threshold
    float mixRange       = 0.05;          // threshold mixing range (slope range)
    float rtf            = 30;            // rock texture factor
    float gtf            = 20;            // grass texture factor

    float maxSnowLevel   = 80;            // maximum snow height (up from here, there's only snow within the maxSnowSlopw)
    float minSnowLevel   = 50;            // minimum snow height (down from here, there's zero snow)
    float maxSnowSlope   = 0.90;          // maximum slope where snow can rest
    float snowSlope      = maxSnowSlope * ( (inPosition.z - minSnowLevel) / (maxSnowLevel - minSnowLevel) );
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
	//float distance = length(ubo.camPos - inPosition);
	
    float squareDistance = (inPosition.x - ubo.camPos.x) * (inPosition.x - ubo.camPos.x) +
                           (inPosition.y - ubo.camPos.y) * (inPosition.y - ubo.camPos.y) +
                           (inPosition.z - ubo.camPos.z) * (inPosition.z - ubo.camPos.z);

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