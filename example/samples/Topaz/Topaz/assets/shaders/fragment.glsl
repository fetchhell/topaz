#extension GL_NV_command_list : enable
#extension GL_NV_shadow_samplers_cube : enable
#extension GL_ARB_bindless_texture : require

#define UBO_OBJECT    1

struct ObjectData
{
	vec4 objectID;
	vec4 objectColor;
	samplerCube skybox;
};

layout(commandBindableNV) uniform;

layout(std140, binding = UBO_OBJECT) uniform objectBuffer
{
	ObjectData  objectData;
};

in Varyings
{
	vec3 skyboxCoord;
} varyings;

layout(location = 0, index = 0) out vec4 outColor;

void main()
{
	vec4 color = vec4(0.0);
	if (objectData.objectID == 0)
	{
		color = textureCube(objectData.skybox, varyings.skyboxCoord);
	}
	else
	{
		color = objectData.objectColor;
	}

	outColor = color;
}