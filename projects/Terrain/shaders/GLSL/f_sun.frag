#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

layout(set = 0, binding  = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 inUVs;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(texSampler, inUVs);
}
