#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

layout(set = 0, binding  = 1) uniform sampler2D texSampler;		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;				// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

void main()
{
	//outColor = vec4(fragColor, 1.0);
	outColor = texture(texSampler, fragTexCoord);
	//outColor = vec4(texture(texSampler, fragTexCoord).rgb, 1.0);
	//outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}