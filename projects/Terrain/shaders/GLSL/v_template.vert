#version 450
#extension GL_ARB_separate_shader_objects : enable

#define numLights 2

struct LightPD
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
	LightPD light[2];	// n * (2 * vec4)
} ubo;

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec2 inUVCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outVertPos;	// Each location has 16 bytes
layout(location = 1) out vec2 outUVCoord;
layout(location = 2) out vec3 outCamPos;
layout(location = 3) out float outSlope;
layout(location = 4) out LightPD outLight[numLights];

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inVertPos, 1.0);
	outUVCoord  = inUVCoord;
	vec3 normal = mat3(ubo.normalMatrix) * inNormal;
	outSlope    = dot( normalize(normal), normalize(vec3(normal.xy, 0.0)) );
	
	// TBN matrix:
	
	vec3 tangent   = normalize(vec3(ubo.model * vec4(cross(vec3(0,1,0), normal), 0.f)));	// X
	vec3 bitangent = normalize(vec3(ubo.model * vec4(cross(normal, tangent), 0.f)));		// Y
	mat3 TBN       = transpose(mat3(tangent, bitangent, normal));							// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)
	
	// Values transformed to tangent space:
	
	outVertPos = TBN * vec3(ubo.model * vec4(inVertPos, 1.f));								// inverted TBN transforms vectors to tangent space
	outCamPos  = TBN * ubo.camPos.xyz;
	for(int i = 0; i < numLights; i++) {
		outLight[i].position.xyz  = TBN * ubo.light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = TBN * normalize(ubo.light[i].direction.xyz);			// for directional light
	}
}


/*
	Notes:
		- gl_Position:    Contains the position of the current vertex (you have to pass the vertex value)
		- gl_VertexIndex: Index of the current vertex, usually from the vertex buffer.
		- (location = X): Indices for the inputs that we can use later to reference them. Note that some types, like dvec3 64 bit vectors, use multiple slots:
							layout(location = 0) in dvec3 inPosition;
							layout(location = 2) in vec3 inColor;
		- MVP transformations: They compute the final position in clip coordinates. Unlike 2D triangles, the last component of the clip coordinates may not be 1, which will result 
		in a division when converted to the final normalized device coordinates on the screen. This is used in perspective projection for making closer objects look larger than 
		objects that are further away.
		- Multiple descriptor sets: You can bind multiple descriptor sets simultaneously by specifying a descriptor layout for each descriptor set when creating the pipeline layout. 
		Shaders can then reference specific descriptor sets like this:  "layout(set = 0, binding = 0) uniform UniformBufferObject { ... }". This way you can put descriptors that 
		vary per-object and descriptors that are shared into separate descriptor sets, avoiding rebinding most of the descriptors across draw calls
*/