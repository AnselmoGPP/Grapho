#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    int lightType;		// int
	
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
layout(set = 0, binding = 1) uniform dataBlock 
{
    Light light;
	vec4 camPos;		// vec3
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[2];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

vec3 directionalLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess);
vec3 PointLightColor	  (Light light, vec3 diffuseMap, vec3 specularMap, float shininess);
vec3 SpotLightColor		  (Light light, vec3 diffuseMap, vec3 specularMap, float shininess);
vec3 applyFog			  (vec3 fragment);

void main()
{
	//outColor = vec4(inColor, 1.0);
	//outColor = texture(texSampler[0], inTexCoord);
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);

	if(ubo.light.lightType == 1)
		outColor = vec4( directionalLightColor(ubo.light, texture(texSampler[0], inTexCoord).rgb, vec3(0.1, 0.1, 0.1), 0.4),  1.0 );
	else if(ubo.light.lightType == 2)
		outColor = vec4( PointLightColor(ubo.light, texture(texSampler[0], inTexCoord).rgb, vec3(0.1, 0.1, 0.1), 0.4),  1.0 );
	else if(ubo.light.lightType == 3)
		outColor = vec4( SpotLightColor(ubo.light, texture(texSampler[0], inTexCoord).rgb, vec3(0.1, 0.1, 0.1), 0.4),  1.0 );
	else
		outColor = outColor = texture(texSampler[0], inTexCoord);
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


// mod(%) = a - (b * floor(a/b))