#version 460

#extension GL_EXT_descriptor_heap : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(descriptor_heap) uniform texture2D textures[];
layout(descriptor_heap) uniform sampler samplers[];

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(push_constant, std430) uniform PushConstants {
    int texture_idx;
    int sampler_idx;
} pc;

#define TEXTURE textures[pc.texture_idx]
#define SAMPLER samplers[pc.sampler_idx]

void main() {
    vec2 gridUV = inUV * 10.0;
	
    outFragColor = texture(sampler2D(TEXTURE, SAMPLER), inUV);

}    
