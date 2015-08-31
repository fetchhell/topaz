#version 440

#extension GL_NV_command_list : enable
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(line_strip, max_vertices = 4) out;

#define UBO_SCENE     0

struct SceneData
{
	mat4 modelViewProjection;
};

layout(commandBindableNV) uniform;

layout(std140, binding = UBO_SCENE) uniform sceneBuffer
{
	SceneData  sceneData;
};

in Varyings 
{
  vec3 pos;
} in_varyings[];

void main()
{
  for (int i = 0; i < gl_in.length(); i++)
  {
	gl_Position = gl_in[i].gl_Position;
    EmitVertex();
  }

  gl_Position = gl_in[0].gl_Position;
  EmitVertex();
  
  EndPrimitive();  
}
