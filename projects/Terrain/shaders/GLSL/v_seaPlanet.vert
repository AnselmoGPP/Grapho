#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

#define RADIUS 2020
#define SPEED     1
#define AMPLITUDE 1
#define FREQUENCY 0.1
#define STEEPNESS 0.2		// [0,1]
#define MIN_RANGE 200
#define MAX_RANGE 400

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;			// mat3
	vec4 camPos;				// vec3
	vec4 time;					// float
	vec4 sideDepthsDiff;
	LightPD light[NUMLIGHTS];	// n * (2 * vec4)
} ubo;

layout(location = 0) in vec3 inPos;					// Each location has 16 bytes
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inGapFix;

layout(location = 0)  		out vec3	outPos;			// Vertex position.
layout(location = 1)  flat 	out vec3 	outCamPos;		// Camera position
layout(location = 2)  		out vec3	outNormal;		// Ground normal
layout(location = 3)  		out float	outSlope;		// Ground slope
layout(location = 4)  		out float	outDist;		// Distace vertex-camera
layout(location = 5)  flat	out float	outCamSqrHeight;// Camera square height over nucleus
layout(location = 6)		out float	outGroundHeight;// Ground height over nucleus
layout(location = 7) flat   out float   outTime;
layout(location = 8)  		out TB3		outTB3;			// Tangents & Bitangents
layout(location = 14) flat	out LightPD outLight[NUMLIGHTS];

vec3 getSeaOptimized(inout vec3 normal);
vec3 GerstnerWaves(vec3 pos, inout vec3 normal);			// Gerstner waves standard (https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models)
vec3 GerstnerWaves_sphere(vec3 pos, inout vec3 normal);		// Gerstner waves projected along the sphere surface.
vec3 GerstnerWaves_sphere2(vec3 pos, inout vec3 normal);	// Gerstner waves projected perpendicularly from the direction line to the sphere

void main()
{
	vec3 normal     = inNormal;
	vec3 pos        = getSeaOptimized(normal);//GerstnerWaves_sphere(inPos, normal);
	gl_Position		= ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
				    
	outPos          = pos;
	outNormal       = mat3(ubo.normalMatrix) * normal;
	outDist         = getDist(pos, ubo.camPos.xyz);		// Dist. to wavy geoid
	outCamSqrHeight = ubo.camPos.x * ubo.camPos.x + ubo.camPos.y * ubo.camPos.y + ubo.camPos.z * ubo.camPos.z;	// Assuming vec3(0,0,0) == planetCenter
	outGroundHeight = sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
	outSlope        = 1. - dot(outNormal, normalize(pos - vec3(0,0,0)));				// Assuming vec3(0,0,0) == planetCenter
	outCamPos       = ubo.camPos.xyz;
	outTime         = ubo.time[0];
	
	for(int i = 0; i < NUMLIGHTS; i++) 
	{
		outLight[i].position.xyz  = ubo.light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = normalize(ubo.light[i].direction.xyz);			// for directional light
	}
	
	outTB3 = getTB3(normal);
}


vec3 getSeaOptimized(inout vec3 normal)
{
	float dist = getDist(inPos, ubo.camPos.xyz);		// Dist. to the sphere, not the wavy geoid.
	
	if(dist > MAX_RANGE) 
		return fixedPos(inPos, inGapFix, ubo.sideDepthsDiff);
	else if(dist > MIN_RANGE)
	{
		float ratio = 1 - (dist - MIN_RANGE) / (MAX_RANGE - MIN_RANGE);
		vec3 pos_0 = fixedPos(inPos, inGapFix, ubo.sideDepthsDiff);
		vec3 pos_1 = GerstnerWaves_sphere(pos_0, normal);
		vec3 diff = pos_1 - pos_0;
		
		return pos_0 + ratio * diff;
	}
	else 
		return GerstnerWaves_sphere(fixedPos(inPos, inGapFix, ubo.sideDepthsDiff), normal);
}

vec3 getSphereDir(vec3 planeDir, vec3 normal)
{
	vec3 right = cross(planeDir, normal);
	return cross(normal, right);				// front
}

// Gerstner waves applied over a sphere, perpendicular to normal
vec3 GerstnerWaves_sphere(vec3 pos, inout vec3 normal)
{
	float speed      = SPEED;
	float w          = FREQUENCY;				// Frequency (number of cycles in 2π)
	float A          = AMPLITUDE;				// Amplitude
	float steepness  = STEEPNESS;				// [0,1]
	float Q          = steepness * 1 / (w * A);	// Steepness [0, 1/(w·A)] (bigger values produce loops)
	float time       = ubo.time[0];
	const int count  = 6;	
	vec3 dir[count]  = { 	// Set of unit vectors
		vec3(1,0,0), vec3(SR05, SR05, 0), vec3(0,1,0), vec3(0, SR05, -SR05), vec3(0,0,1), vec3(-SR05, 0, SR05) };
		
	vec3 newPos      = pos;
	vec3 newNormal   = normal;
	float multiplier = 0.9;
	float horDisp, verDisp;			// Horizontal/Vertical displacement
	float arcDist;					// Arc distance (-direction-origin-vertex)
	float rotAng;
	vec3 rotAxis;
	vec3 right, front, up = normalize(pos);

	
	for(int i = 0; i < count; i++)
	{
		arcDist = getAngle(-dir[i], up) * RADIUS;
		rotAxis = normalize(cross(dir[i], up));
		
		horDisp = cos(w * arcDist + speed * time);
		verDisp = sin(w * arcDist + speed * time);
		
		// Vertex
		rotAng  = (Q * A) * horDisp / RADIUS;					
		newPos  = rotationMatrix(rotAxis, rotAng) * newPos;		// <<< what if rotation axis is == 0?
		newPos += normalize(newPos) * A * verDisp;
		
		// Normal
		right  = cross(dir[i], up);
		front  = normalize(cross(up, right));
		
		rotAng = asin(w * A * horDisp);
		newNormal = rotationMatrix(rotAxis, -rotAng) * newNormal;
		
		speed *= multiplier;
		w /= multiplier;
		//A *= multiplier;
		//Q *= multiplier;
	}

	normal = newNormal;
	return newPos;
}

// Gerstner waves applied over a sphere, perpendicular to direction
vec3 GerstnerWaves_sphere2(vec3 pos, inout vec3 normal)
{
	float speed      = SPEED;
	float w          = FREQUENCY;				// Frequency (number of cycles in 2π)
	float A          = AMPLITUDE;				// Amplitude
	float steepness  = STEEPNESS;				// [0,1]
	float Q          = steepness * 1 / (w * A);	// Steepness [0, 1/(w·A)] (bigger values produce loops)
	float time       = ubo.time[0];
	const int count  = 6;
		
	vec3 dir[count]  = { vec3(1,0,0), vec3(SR05, SR05, 0), vec3(0,1,0), vec3(0, SR05, -SR05), vec3(0,0,1), vec3(-SR05, 0, SR05) };
	vec3 newPos      = pos;
	vec3 newNormal   = normal;
	vec3 unitPos     = normalize(pos);
	float multiplier = 0.95;
	float cosVal, sinVal;
	
	for(int i = 0; i < count; i++)
	{
		cosVal = cos(w * dot(dir[i], pos) + speed * time);
		sinVal = sin(w * dot(dir[i], pos) + speed * time);
		
		newPos.x += Q * A * dir[i].x * cosVal;
		newPos.y += Q * A * dir[i].y * cosVal;
		newPos.z += Q * A * dir[i].z * cosVal;
		newPos   += unitPos * A * sinVal;
		
		dir[i] = getSphereDir(dir[i], normal);
		
		newNormal.x -= w * A * dir[i].x * cosVal;
		newNormal.y -= w * A * dir[i].y * cosVal;
		newNormal.z -= w * A * dir[i].z * cosVal;
		//newNormal   -= w * A * Q * sinVal;		//<<< this is subtracting a float, but I use a vec3 (normal)
		
		speed *= multiplier;
		w /= multiplier;
		A *= multiplier;
		Q *= multiplier;
	}
	
	normal = normalize(newNormal);
	return newPos;
}

// Gerstner waves applied over a 2D surface
vec3 GerstnerWaves(vec3 pos, inout vec3 normal)
{
	float time      = ubo.time[0];
	float speed     = 3;
	float w         = 0.010;			// Frequency (number of cycles in 2π)
	float A         = 100;				// Amplitude
	float Q         = 0.5;				// Steepness [0, 1/(w·A)] (bigger values produce loops)
	const int count = 6;
	vec3 dir[count] = { vec3(1,0,0), vec3(SR05, SR05, 0), vec3(0,1,0), vec3(0, SR05, -SR05), vec3(0,0,1), vec3(-SR05, 0, SR05) };
	vec3 newPos     = { pos.x, pos.y, 0 };
	//normal        = { 0, 0, 1 };
	float cosVal, sinVal;	
	
	for(int i = 0; i < count; i++)
	{
		cosVal = cos(w * dot(dir[i], pos) + speed * time);
		sinVal = sin(w * dot(dir[i], pos) + speed * time);
		
		newPos.x += A * Q * dir[i].x * cosVal;
		newPos.y += A * Q * dir[i].y * cosVal;
		newPos.z += A * sinVal;
		
		normal.x -= w * A * dir[i].x * cosVal;
		normal.y -= w * A * dir[i].y * cosVal;
		normal.z -= w * A * Q * sinVal;
	}
	
	normalize(normal);
	return newPos;
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