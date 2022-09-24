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
	Light light;			// vec4 * 8
} ubo;

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec2 inUVCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outVertPos;
layout(location = 1) out vec2 outUVCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outCampPos;
layout(location = 4) out Light outLight;	// This occupies 8 locations (16 bytes each)
layout(location = 12) out float outSlope;
layout(location = 13) out vec3 lightDir;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inVertPos, 1.0);
	outLight    = ubo.light;
	outUVCoord  = inUVCoord;
	vec3 normal = mat3(ubo.normalMatrix) * inNormal;
	outSlope    = dot( normalize(normal), normalize(vec3(normal.xy, 0.0)) );
	
	// TBN matrix:
	vec3 tangent   = normalize(vec3(ubo.model * vec4(cross(vec3(0,1,0), normal), 0.f)));	// x
	vec3 bitangent = normalize(vec3(ubo.model * vec4(cross(normal,    tangent ), 0.f)));	// y
	mat3 TBN       = transpose(mat3(tangent, bitangent, normal));							// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)
	
	// Values transformed to tangent space:
	outVertPos 			   = TBN * vec3(ubo.model * vec4(inVertPos, 1.f));	// inverted TBN transforms vectors to tangent space
	outCampPos             = TBN * ubo.camPos.xyz;
	outLight.position.xyz  = TBN * ubo.light.position.xyz;						// for point & spot light
	outLight.direction.xyz = TBN * normalize(ubo.light.direction.xyz);			// for directional light
	lightDir 			   = TBN * normalize(ubo.light.direction.xyz);
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