#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

layout(set = 0, binding  = 1) uniform sampler2D texSampler[6];		// sampler1D, sampler2D, sampler3D

//layout(location = 0) in vec2 inUVs;
layout(location = 0) in vec3 inPos;

layout(location = 0) out vec4 outColor;				// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

void main()
{
	outColor.xyz = skyboxTex(inPos, texSampler[0], texSampler[1], texSampler[2], texSampler[3], texSampler[4], texSampler[5]);
	outColor.w = 1;
	//outColor = texture(texSampler, inUVs);
	
	//outColor = vec4(0, 0.5, 0.5, 1);
	//outColor = vec4(fragColor, 1.0);
	//outColor = vec4(texture(texSampler, fragTexCoord).rgb, 1.0);
	//outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}