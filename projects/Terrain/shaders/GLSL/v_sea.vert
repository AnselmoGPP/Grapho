#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

//#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

#define numLights 3

struct LightPD2
{
    vec4 position;		// vec3
    vec4 direction;		// vec3
};

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;	// mat3
	vec4 camPos;		// vec3
	LightPD2 light[2];	// n * (2 * vec4)
} ubo;

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outVertPos;
layout(location = 1) out vec3 outCamPos;
layout(location = 2) out LightPD2 outLight[numLights];

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inVertPos, 1.0);
	vec3 normal = mat3(ubo.normalMatrix) * inNormal;
/*
	// TBN matrix:
	vec3 tangent   = normalize(vec3(ubo.model * vec4(cross(vec3(0,1,0), normal), 0.f)));	// X
	vec3 bitangent = normalize(vec3(ubo.model * vec4(cross(normal,    tangent ), 0.f)));	// Y
	mat3 TBN       = transpose(mat3(tangent, bitangent, normal));							// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)

	// Values transformed to tangent space:
	outVertPos             = TBN * vec3(ubo.model * vec4(inVertPos, 1.f));					// inverted TBN transforms vectors to tangent space
	outCamPos              = TBN * ubo.camPos.xyz;
	for(int i = 0; i < numLights; i++) {
		outLight[i].position.xyz  = TBN * ubo.light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = TBN * normalize(ubo.light[i].direction.xyz);			// for directional light
	}
*/
}