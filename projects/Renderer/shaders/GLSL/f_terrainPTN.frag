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
    sampler2D diffuseT;		// object color
    vec3 diffuse;
    sampler2D specularT;	// specular map
    vec3 specular;
    float shininess;		// shininess
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

void main()
{
	//outColor = vec4(inColor, 1.0);
	//outColor = texture(texSampler[0], inTexCoord);
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);
	outColor = vec4( directionalLightColor(sun.light, texture(texSampler[0], inTexCoord).rgb, vec3(0.1, 0.1, 0.1), 0.4),  1.0 );

	//outColor = vec4(sun.camPos[0], sun.camPos[1], sun.camPos[2], 1.f);
	//outColor = vec4(sun.constant, sun.linear, sun.quadratic, 1.f);
	//outColor = vec4(sun.light.degree[0], sun.light.degree[1], sun.light.degree[2], 1.f);
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


// mod(%) = a - (b * floor(a/b))