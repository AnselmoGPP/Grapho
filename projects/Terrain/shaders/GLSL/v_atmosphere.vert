#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform ubobject {
	vec4 aspRatio;		// float
	vec4 camPos;		// vec3
	vec4 camDir;		// vec3
} ubo;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUVcoords;

layout(location = 0) out vec2 outUVcoords;
layout(location = 1) out float outAspRatio;
layout(location = 2) out vec3 outCamPos;
layout(location = 3) out vec3 outCamDir;

void main()
{
    //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
    gl_Position = vec4(inPos, 1.0f);
	//gl_Position.x = gl_Position.x * ubo.aspRatio.x;
    outUVcoords = inUVcoords;
	outAspRatio = ubo.aspRatio.x;
	outCamPos = ubo.camPos.xyz;
	outCamDir = ubo.camDir.xyz;
}