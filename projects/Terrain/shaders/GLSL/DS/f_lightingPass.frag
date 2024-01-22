#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)


//layout(set = 0, binding = 1) uniform ubobject {
//	vec4 camPos;
//	LightProps lightProps[NUMLIGHTS];
//} ubo;

//layout(set = 0, binding = 1) uniform sampler2D texSampler[2];		// Opt. depth, Density
layout(set = 0, binding = 1) uniform sampler2D inputAttachments[4];	// Position, Albedo, Normal, Specular_roughness (sampler2D for single-sample | sampler2DMS for multisampling)

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 outColor;

void main()
{
	//LightProps uboLight[NUMLIGHTS], LightPD inLight[NUMLIGHTS];
	//savePrecalcLightValues(inputAttachments[0], ubo.camPos.xyz, ubo.lightProps, inLight);
	//savePNT(inPos, normalize(inNormal), inTB3);
	
	//outColor = inputAttachments[2];
	outColor = texture(inputAttachments[1], inUVs);
	//outColor = texture(inputAttachments[2], ivec2(inUVs * textureSize(inputAttachments[2], 0)));
	
	//outColor = texelFetch(inputAttachments[0], ivec2(inUVs * textureSize(inputAttachments[0])), gl_SampleID);
	//outColor = vec4(texture(texSampler[0], inUVs).rgb, 1.0);
	//outColor = vec4(0,0,1,1);
	//outColor = vec4(gl_FragCoord.x, gl_FragCoord.x, gl_FragCoord.x, 1);
}
