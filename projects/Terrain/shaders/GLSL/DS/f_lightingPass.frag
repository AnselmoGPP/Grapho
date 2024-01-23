#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

//layout(early_fragment_tests) in;

layout(set = 0, binding = 1) uniform ubobject {
	vec4 camPos;
	//LightPD lightPosDir[NUMLIGHTS];	// n * (2 * vec4)
	LightProps lightProps[NUMLIGHTS];
} ubo;

//layout(set = 0, binding = 1) uniform sampler2D texSampler[2];		// Opt. depth, Density
layout(set = 0, binding = 2) uniform sampler2D inputAttachments[4];	// Position, Albedo, Normal, Specular_roughness (sampler2D for single-sample | sampler2DMS for multisampling)

layout(location = 0) in vec2 inUVs;
layout(location = 1) flat in LightPD inLight[NUMLIGHTS];

layout(location = 0) out vec4 outColor;

void main()
{
	//if(ubo.lightProps[0].type > 9.0) {outColor = vec4(1, 0, 0, 1); return; }
	//float range = 2000;
	//vec3 fragPos = (texture(inputAttachments[0], inUVs).xyz * 2 * range) - vec3(range);
	vec3 fragPos = texture(inputAttachments[0], inUVs).xyz;
	vec3 albedo = texture(inputAttachments[1], inUVs).xyz;
	vec3 normal = normalize(texture(inputAttachments[2], unpackUV(inUVs, 1)).xyz);//unpackNormal(texture(inputAttachments[2], unpackUV(inUVs, 1)).xyz);	//unpackNormal(texture(inputAttachments[2], unpackUV(inUVs, 1)).xyz);
	vec4 specRough = texture(inputAttachments[3], inUVs);
	
	//normal = normalize(ubo.camPos.xyz);
	//outColor = vec4(fragPos - vec3(1), 1.0); return;
	outColor = vec4(albedo, 1.0); return;
	//outColor = vec4(normal, 1.0); return;
	//outColor = vec4(specRough.xyz, specRough.xyz, specRough.xyz, 1.0); return;
	//outColor = vec4(specRough.w, specRough.w, specRough.w, 1.0); return;
	
	//float dist = length(ubo.camPos.xyz - fragPos) / 200;
	//outColor = vec4(dist, dist, dist, 1.0); return;
	//outColor = vec4(fragPos, 1.0); return;
	//outColor = vec4(vec3(specRough.w, specRough.w, specRough.w), 1.0); return;
	//outColor = vec4(vec3(1000, 1000, 0), 1.0); return;
	//outColor = vec4(fragPos, 1.0); return;
	
	savePrecalcLightValues(fragPos, ubo.camPos.xyz, ubo.lightProps, inLight);//ubo.lightPosDir);
	//TB3 empty;
	//savePNT(fragPos, normal, empty);
	
	//outColor = inputAttachments[2];
	//outColor = texture(inputAttachments[1], inUVs);
	//outColor = vec4(getFragColor(albedo, normal, specRough.xyz, specRough.w), 1.0);
	//outColor = vec4(getFragColor(albedo, normal, vec3(0,0,0), 0), 1.0);
	//outColor = texture(inputAttachments[2], ivec2(inUVs * textureSize(inputAttachments[2], 0)));
	
	//outColor = texelFetch(inputAttachments[0], ivec2(inUVs * textureSize(inputAttachments[0])), gl_SampleID);
	//outColor = vec4(texture(texSampler[0], inUVs).rgb, 1.0);
	//outColor = vec4(0,0,1,1);
	//outColor = vec4(gl_FragCoord.x, gl_FragCoord.x, gl_FragCoord.x, 1);
}
