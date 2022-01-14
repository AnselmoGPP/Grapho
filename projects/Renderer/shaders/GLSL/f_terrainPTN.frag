#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    int lightType;
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float cutOff;
    float outerCutOff;
};

struct Material
{
    sampler2D diffuseT;
    vec3 diffuse;
    sampler2D specularT;
    vec3 specular;
    float shininess;
};

vec3 directionalLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess);

layout(set = 0, binding  = 1) uniform sampler2D texSampler[2];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inPosition;

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

void main()
{
	//outColor = vec4(inColor, 1.0);
	outColor = texture(texSampler[0], inTexCoord);
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);
}

vec3 directionalLightColor(Light light, vec3 diffuseMap, vec3 specularMap, float shininess)
{
    vec3 camPos = vec3(30., -30., 30);

    // ----- Ambient lighting -----
    vec3 ambient = light.ambient * diffuseMap;

    // ----- Diffuse lighting -----
    vec3 lightDir = normalize(light.direction);
    vec3 norm = normalize(inNormal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseMap;

    // ----- Specular lighting -----
    vec3 viewDir = normalize(camPos - inPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * specularMap;

    // ----- Result -----
    return vec3(ambient + diffuse + specular);
}