#version 440

#extension GL_ARB_bindless_texture : require
#extension GL_NV_command_list : enable

#define UBO_OBJECT    1
#define UBO_OIT       2

struct ObjectData
{
	vec4 objectID;
	vec4 objectColor;
	samplerCube skybox;
};

struct WeightBlendedData
{
	sampler2DRect background;
	sampler2DRect colorTex0;
	sampler2DRect colorTex1;
};

layout(commandBindableNV) uniform;

layout(std140, binding = UBO_OBJECT) uniform objectBuffer
{
	ObjectData objectData;
};

layout(std140, binding = UBO_OIT) uniform weightBlendedBuffer
{
	WeightBlendedData weightBlendedData;
};


uniform sampler2DRect ColorTex0;
uniform sampler2DRect ColorTex1;
/*
uniform sampler2DRect background;
*/

layout(location = 0) out vec4 outColor;

void main(void)
{
    vec4 sumColor = texture(weightBlendedData.colorTex0, gl_FragCoord.xy); // texture(ColorTex0, gl_FragCoord.xy); // texture(weightBlendedData.colorTex0, gl_FragCoord.xy);
    float transmittance = texture(weightBlendedData.colorTex1, gl_FragCoord.xy).r; // texture(ColorTex1, gl_FragCoord.xy).r; // texture(weightBlendedData.colorTex1, gl_FragCoord.xy).r;
    vec3 backgroundColor = texture(weightBlendedData.background, gl_FragCoord.xy).rgb;  // texture(background, gl_FragCoord.xy).rgb;
	
    vec3 averageColor = sumColor.rgb / max(sumColor.a, 0.00001);

    outColor.rgb = averageColor * (1 - transmittance) + backgroundColor * transmittance;
}

