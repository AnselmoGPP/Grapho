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

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;		// mat3
	vec4 camPos;			// vec3
	Light light;			// vec4 * 32
} ubo;

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outVertPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outCampPos;
layout(location = 3) out Light light;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inVertPos, 1.0);	
	outNormal   = mat3(ubo.normalMatrix) * inNormal;
	light       = ubo.light;
	
	// TBN matrix
	vec3 tangent   = normalize(vec3(ubo.model * vec4(cross(vec3(0,1,0), inNormal), 0.f)));	// x
	vec3 bitangent = normalize(vec3(ubo.model * vec4(cross(inNormal,    tangent ), 0.f)));	// y
	mat3 TBN       = transpose(mat3(tangent, bitangent, inNormal));							// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)

	// Values transformed to tangent space:
	outVertPos          = TBN * vec3(ubo.model * vec4(inVertPos, 1.f));	// inverted TBN transforms vectors to tangent space
	outCampPos          = TBN * ubo.camPos.xyz;
	light.position.xyz  = TBN * light.position.xyz;						// for point & spot light
	light.direction.xyz = TBN * normalize(light.direction.xyz);			// for directional light
}