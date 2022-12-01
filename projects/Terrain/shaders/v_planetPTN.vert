#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NUMLIGHTS 2

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

layout(location = 0) in vec3     inPos;
layout(location = 1) in vec2     inUV;
layout(location = 2) in vec3     inNormal;

layout(location = 0) out vec3    outVertPos;	// Each location has 16 bytes
layout(location = 1) out vec3    outPos;		// Vertex position not transformed with TBN matrix
layout(location = 2) out vec2    outUV;
layout(location = 3) out vec3    outCamPos;
layout(location = 4) out float   outSlope;
layout(location = 5) out vec3    outNormal;
layout(location = 6) out float   outDist;
layout(location = 7) out float   outHeight;
layout(location = 8) out LightPD outLight[NUMLIGHTS];

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
	
	outPos      = inPos;
	outUV  		= inUV;
	outNormal   = mat3(ubo.normalMatrix) * inNormal;
	vec3 diff   = inPos - ubo.camPos.xyz;
	outDist     = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
	outHeight   = sqrt(inPos.x * inPos.x + inPos.y * inPos.y + inPos.z * inPos.z);
	outSlope    = 1. - dot(outNormal, normalize(inPos - vec3(0,0,0)));					// vec3(0,0,0) == planetCenter
	
	// TBN matrix:
	vec3 tangent;
	vec3 bitangent;
	mat3 TBN;
	
	tangent   = normalize(vec3(ubo.model * vec4(cross(vec3(0, 1, 0), outNormal), 0.f)));	// X
	bitangent = normalize(vec3(ubo.model * vec4(cross(outNormal,     tangent  ), 0.f)));	// Y
	TBN       = transpose(mat3(tangent, bitangent, outNormal));								// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)

	// Values transformed to tangent space:
	outVertPos = TBN * vec3(ubo.model * vec4(inPos, 1.f));								// inverted TBN transforms vectors to tangent space
	outCamPos  = TBN * ubo.camPos.xyz;
	for(int i = 0; i < NUMLIGHTS; i++) {
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