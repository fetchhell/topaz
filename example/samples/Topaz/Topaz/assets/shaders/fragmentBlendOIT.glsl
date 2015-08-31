#version 440

#extension GL_ARB_bindless_texture : require
#extension GL_NV_command_list : enable

#define UBO_SCENE     0
#define UBO_OBJECT    1

struct ObjectData
{
	vec4 objectID;
	vec4 objectColor;
	samplerCube skybox;
};

struct SceneData
{
	mat4 modelViewProjection;
	float depthScale;
};

layout(commandBindableNV) uniform;

layout(std140, binding = UBO_SCENE) uniform sceneBuffer
{
	SceneData  sceneData;
};

layout(std140, binding = UBO_OBJECT) uniform objectBuffer
{
	ObjectData  objectData;
};

layout(location = 0) out vec4 oSumColor;
layout(location = 1) out vec4 oSumWeight;

void main(void)
{
    vec4 color = objectData.objectColor;

    float viewDepth = abs(1.0 / gl_FragCoord.w);

    float linearDepth = viewDepth * sceneData.depthScale;
    float weight = clamp(0.03 / (1e-5 + pow(linearDepth, 4.0)), 1e-2, 3e3);

    oSumColor = vec4(color.rgb * color.a, color.a) * weight;
    oSumWeight = vec4(color.a);
}
