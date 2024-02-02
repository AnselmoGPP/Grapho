#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

#define MIN_HEIGHT 2014
#define MAX_HEIGHT 2090
#define MAX_SLOPE 0.22

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;			// mat3
	vec4 camPos_time;			// camPos + time
	vec4 modelPos_gSlope;		// vec3 + float
	LightPD light[NUMLIGHTS];	// n * (2 * vec4)
} ubo[500];

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUVs;
layout(location = 3) flat out vec3 outCamPos;
layout(location = 4) flat out vec3 outModelPos;
layout(location = 5) out float outSqrDist;
layout(location = 6) flat out LightPD outLight[NUMLIGHTS];	// light positions & directions

void main()
{
	
	vec3 pos      = inPos;												// position without MVP matrix applied yet
	vec3 modelPos = ubo[gl_InstanceID].modelPos_gSlope.xyz;
	float gSlope  = ubo[gl_InstanceID].modelPos_gSlope.a;
	float sqrDist = getSqrDist(modelPos, ubo[gl_InstanceID].camPos_time.xyz);			// dist modelPos-camPos
	float height  = getLength(modelPos);
	
	// Translation
	//pos += vec3(-0.1, 0, 0);
	
	// Scale
	pos *= getRatio(sqrDist, 90*90, 0);			// Re-scaling (later, the MVP scales the mesh)
	
	float xScale = 1;
	//float xScale = min(min(
	//				getRatio(height, MIN_HEIGHT, MIN_HEIGHT + 4, 0.2, 1.0),			// The closer to MIN_HEIGHT, the shorter the grass
	//				getRatio(height, MAX_HEIGHT, MAX_HEIGHT - 4, 0.2, 1.0) ),		// The closer to MAX_HEIGHT, the shorter the grass
	//				getRatio(gSlope, MAX_SLOPE - 0.1, MAX_SLOPE, 1.0, 0.2) );		// The higher the slope, the shorter the grass

	//pos.x *= xScale;	// vert. scale
	//pos.y *= 1.5;		// hor. scale
	
	// Wind
	float time = ubo[gl_InstanceID].camPos_time.a + (modelPos.x + modelPos.y + modelPos.z);				// add some randomness to the time
	pos += getRatio(inPos.x, 0, xScale) * vec3(0,0,1) * sin(2 * time) * (0.02 * xScale);	// speed (2), amplitude (0.02), move axis (0,0,1)
	
	// Final position
	if(sqrDist < 4)		//  <<< Displace grass when too close
	{		
		float ratio = 1.f - getRatio(sqrDist, 0, 2*2);	// 2*X: max distance from where cam moves grass
		ratio *= pos.x; 								// don't move roots
		
		vec3 displacementDir = normalize(modelPos - ubo[gl_InstanceID].camPos_time.xyz);
		vec3 sphereNormal = normalize(modelPos);
		vec3 right = normalize(cross(displacementDir, sphereNormal));
		displacementDir = normalize(cross(sphereNormal, right));
		
		vec4 vertexPos = ubo[gl_InstanceID].model * vec4(pos, 1.0);	// apply MVP to position
		vertexPos.xyz += displacementDir * ratio * 2;	// 2: max grass displacement
		gl_Position = ubo[gl_InstanceID].proj * ubo[gl_InstanceID].view * vertexPos;
	}
	else 
		gl_Position = ubo[gl_InstanceID].proj * ubo[gl_InstanceID].view * ubo[gl_InstanceID].model * vec4(pos, 1.0);
	
	// Others
	//gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
	outPos      = (ubo[gl_InstanceID].model * vec4(pos, 1.0)).xyz;
	outNormal   = mat3(ubo[gl_InstanceID].normalMatrix) * inNormal;
	outUVs      = inUVs;
	outCamPos   = ubo[gl_InstanceID].camPos_time.xyz;
	outModelPos = modelPos;
	outSqrDist  = getSqrDist(ubo[gl_InstanceID].camPos_time.xyz, (ubo[gl_InstanceID].model * vec4(pos, 1.0)).xyz);
	
	for(int i = 0; i < NUMLIGHTS; i++) 
	{
		outLight[i].position.xyz  = ubo[gl_InstanceID].light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = normalize(ubo[gl_InstanceID].light[i].direction.xyz);			// for directional & spot light
	}
}