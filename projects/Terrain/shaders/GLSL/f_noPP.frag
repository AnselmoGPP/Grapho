#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

layout(set = 0, binding = 1) uniform sampler2D texSampler[2];			// Opt. depth, Density
layout(set = 0, binding = 2) uniform sampler2DMS inputAttachments[2];	// Color, Depth (sampler2D for single-sample | sampler2DMS for multisampling)

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texelFetch(inputAttachments[0], ivec2(inUVs * textureSize(inputAttachments[0])), gl_SampleID);
	//outColor = vec4(texture(texSampler[0], inUVs).rgb, 1.0);
	//outColor = vec4(0,0,1,1);
	//outColor = vec4(gl_FragCoord.x, gl_FragCoord.x, gl_FragCoord.x, 1);
}
