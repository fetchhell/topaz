#define VERTEX_POS    0
#define VERTEX_NORMAL 1
#define VERTEX_UV     2

#define UBO_SCENE     0
#define UBO_OBJECT    1

#extension GL_ARB_bindless_texture : require
#extension GL_NV_command_list : enable

struct ObjectData
{
	vec4 objectID;
	vec4 objectColor;
	samplerCube skybox;
};

struct SceneData
{
	mat4 modelViewProjection;
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

in layout(location = VERTEX_POS)    vec3 pos;
in layout(location = VERTEX_NORMAL) vec3 normal;
in layout(location = VERTEX_UV)     vec2 uv;

out Varyings
{
	vec3 skyboxCoord;
} varyings;

void main()
{
  gl_Position = sceneData.modelViewProjection * vec4(pos, 1);
  varyings.skyboxCoord = pos;
}







 