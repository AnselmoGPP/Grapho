#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;		// mat3
	vec4 camPos;			// vec3
	vec4 lightPos;			// vec3
	vec4 lightDir;			// vec3
} ubo;

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec2 inUVCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outTangVertPos;
layout(location = 1) out vec2 outUVCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangCampPos;
layout(location = 4) out vec3 outTangLightPos;
layout(location = 5) out vec3 outTangLightDir;
layout(location = 6) out float outSlope;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inVertPos, 1.0);	
	outUVCoord  = inUVCoord;
	outNormal   = mat3(ubo.normalMatrix) * inNormal;
	outSlope = dot( normalize(inNormal), normalize(vec3(inNormal.x, inNormal.y, 0.0)) );
	
	vec3 tangent   = normalize(vec3(ubo.model * vec4(cross(vec3(0,1,0), inNormal), 0.f)));	// x
	vec3 bitangent = normalize(vec3(ubo.model * vec4(cross(inNormal,    tangent ), 0.f)));	// y
	mat3 TBN       = transpose(mat3(tangent, bitangent, inNormal));									// Transpose of an orthogonal matrix == its inverse (transpose is cheaper than inverse)

	outTangVertPos  = TBN * vec3(ubo.model * vec4(inVertPos, 1.f));
	outTangCampPos  = TBN * ubo.camPos.xyz;
	outTangLightPos = TBN * ubo.lightPos.xyz;		// for point & spot light
	outTangLightDir = TBN * ubo.lightDir.xyz;		// for directional light
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