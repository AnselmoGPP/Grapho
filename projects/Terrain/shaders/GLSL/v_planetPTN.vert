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
    mat4 normalMatrix;			// mat3
	vec4 camPos;				// vec3
	vec4 sideDepthsDiff;
	LightPD light[NUMLIGHTS];	// n * (2 * vec4)
} ubo;

layout(location = 0) in vec3     inPos;					// Each location has 16 bytes
layout(location = 1) in vec3     inNormal;
layout(location = 2) in vec3    inGapFix;

layout(location = 0)  		out vec3	outPos;			// Vertex position.
layout(location = 1)  flat 	out vec3 	outCamPos;		// Camera position
layout(location = 2)  		out vec3	outNormal;		// Ground normal
layout(location = 3)  		out float	outSlope;		// Ground slope
layout(location = 4)  		out float	outDist;		// Distace vertex-camera
layout(location = 5)  flat	out float	outCamSqrHeight;// Camera square height over nucleus
layout(location = 6)		out float	outGroundHeight;// Ground height over nucleus
layout(location = 7)  		out vec3	outTanX;		// Tangents & Bitangents
layout(location = 8)  		out vec3	outBTanX;
layout(location = 9)  		out vec3	outTanY;
layout(location = 10)  		out vec3	outBTanY;
layout(location = 11) 		out vec3	outTanZ;
layout(location = 12) 		out vec3	outBTanZ;
layout(location = 13) flat	out LightPD outLight[NUMLIGHTS];

vec3 fixedPos();

void main()
{
	gl_Position		= ubo.proj * ubo.view * ubo.model * vec4(fixedPos(), 1.0);
				    
	outPos          = inPos;
	outNormal       = mat3(ubo.normalMatrix) * inNormal;
	vec3 diff       = inPos - ubo.camPos.xyz;
	outDist         = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
	outCamSqrHeight = ubo.camPos.x * ubo.camPos.x + ubo.camPos.y * ubo.camPos.y + ubo.camPos.z * ubo.camPos.z;	// Assuming vec3(0,0,0) == planetCenter
	outGroundHeight = sqrt(inPos.x * inPos.x + inPos.y * inPos.y + inPos.z * inPos.z);
	outSlope     = 1. - dot(outNormal, normalize(inPos - vec3(0,0,0)));				// Assuming vec3(0,0,0) == planetCenter
	outCamPos    = ubo.camPos.xyz;
	for(int i = 0; i < NUMLIGHTS; i++) 
	{
		outLight[i].position.xyz  = ubo.light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = normalize(ubo.light[i].direction.xyz);			// for directional light
	}
	
	// TBN matrices for triplanar shader:	<<< Maybe I can reduce X & Z plane projections into a single one
	vec3 axis = sign(inNormal);

	outTanX = normalize(cross(inNormal, vec3(0., axis.x, 0.)));			// z,y
	outBTanX = normalize(cross(outTanX, inNormal)) * axis.x;
	
	outTanY = normalize(cross(inNormal, vec3(0., 0., axis.y)));			// x,z
	outBTanY = normalize(cross(outTanY, inNormal)) * axis.y;
	
	outTanZ = normalize(cross(inNormal, vec3(0., -axis.z, 0.)));		// x,y
	outBTanZ = normalize(-cross(outTanZ, inNormal)) * axis.z;
}

vec3 fixedPos2(float sideDepthDiff)
{
	if(sideDepthDiff < 0.1)	return inPos;
	if(sideDepthDiff < 1.1)	return inPos + normalize(inPos) * inGapFix[1];
	else					return inPos + normalize(inPos) * inGapFix[2];
}

vec3 fixedPos()
{
	float vertexType = inGapFix[0];
	
	if     (vertexType < 0.1f) return inPos;
	else if(vertexType < 1.1f) return fixedPos2(ubo.sideDepthsDiff[0]);	// right
	else if(vertexType < 2.1f) return fixedPos2(ubo.sideDepthsDiff[1]);	// left
	else if(vertexType < 3.1f) return fixedPos2(ubo.sideDepthsDiff[2]);	// up
	else					   return fixedPos2(ubo.sideDepthsDiff[3]);	// down
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