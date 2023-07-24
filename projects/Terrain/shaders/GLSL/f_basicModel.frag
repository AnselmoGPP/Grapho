#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

//#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

layout(set = 0, binding  = 1) uniform sampler2D texSampler;		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

void main()
{
	//outColor = vec4(vec3(0.5, 1, 0.5) * texture(texSampler, inUV).rgb, 1.0);
	outColor = vec4(texture(texSampler, inUV).rgb, 1.0);
}