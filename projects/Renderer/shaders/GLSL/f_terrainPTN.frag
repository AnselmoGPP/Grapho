#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    vec4 lightType;		// int
	
    vec4 position;		// vec3
    vec4 direction;		// vec3

    vec4 ambient;		// vec3
    vec4 diffuse;		// vec3
    vec4 specular;		// vec3

    vec4 degree;		// vec3
    vec4 cutOff;		// vec2
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
} sun;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[2];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

vec3 directionalLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess);
//vec4 PointLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess, float alpha);
//vec4 SpotLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess, float alpha);
//vec4 applyFog(vec4 fragment);

void main()
{
	//outColor = vec4(inColor, 1.0);
	//outColor = texture(texSampler[0], inTexCoord);
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);
	outColor = vec4( directionalLightColor(sun.light, texture(texSampler[0], inTexCoord).rgb, vec3(0.1, 0.1, 0.1), 0.4),  1.0 );
}


vec3 directionalLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess)
{
    vec3 norm = normalize(inNormal);
	vec3 lightDir = normalize(light.direction.xyz);
	
    // ----- Ambient lighting -----
    vec3 ambient = light.ambient.xyz * diffuseMap;

    // ----- Diffuse lighting -----
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse.xyz * diff * diffuseMap;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(sun.camPos.xyz - inPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular.xyz * spec * specularMap;

    // ----- Result -----
	return vec3(ambient + diffuse + specular);
}

/*
vec4 PointLightColor( Light light, vec3 diffuseMap, vec3 specularMap, float shininess, float alpha )
{
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // ----- Ambient lighting -----
    vec3 ambient = light.ambient * diffuseMap * attenuation;

    // ----- Diffuse lighting -----
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseMap *attenuation;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(camPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * specularMap * attenuation;

    // ----- Result -----
    return vec4(vec3(ambient + diffuse + specular), alpha);
}


vec4 SpotLightColor( Light light, vec3 diffuseMap, vec3 specularMap, float shininess, float alpha )
{
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // ----- Ambient lighting -----
    vec3 ambient = light.ambient * diffuseMap * attenuation;

    // ----- Diffuse lighting -----
    vec3 lightDir = normalize(light.position - FragPos);
    float theta = dot(lightDir, normalize(light.direction));

    if(theta < light.outerCutOff) return vec4(ambient, 1.0);

    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseMap * attenuation * intensity;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(camPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * specularMap * attenuation * intensity;

    // ----- Result -----
    return vec4(vec3(ambient + diffuse + specular), alpha);
}


vec4 applyFog(vec4 fragment)
{
    float squareDistance = (FragPos.x - camPos.x) * (FragPos.x - camPos.x) +
                           (FragPos.y - camPos.y) * (FragPos.y - camPos.y) +
                           (FragPos.z - camPos.z) * (FragPos.z - camPos.z);

    if(squareDistance > fogMinSquareRadius)
        if(squareDistance > fogMaxSquareRadius)
            fragment = skyColor;
        else
    {
        float ratio  = (squareDistance - fogMinSquareRadius) / (fogMaxSquareRadius - fogMinSquareRadius);
        fragment = vec4(fragment.xyz * (1-ratio) + skyColor.xyz * ratio, fragment.a);
    }

    return fragment;
}
*/

// mod(%) = a - (b * floor(a/b))