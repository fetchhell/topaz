#pragma once

#include "includeAll.h"

class WeightedBlendedOIT
{
public:
	WeightedBlendedOIT();

	void InitAccumulationRenderTargets(int32_t width, int32_t height);
	void DeleteAccumulationRenderTargets();

	GLuint getFramebufferID()
	{
		return accumulationFramebufferId;
	}

	GLfloat & getOpacity()
	{
		return opacity;
	}

	GLfloat & getWeightParameter()
	{
		return weightParameter;
	}

	GLuint getAccumulationTextureId(size_t id)
	{
		return accumulationTextureId[id];
	}

	GLuint getOpaqueId()
	{
		return sceneOpaqueId;
	}

	GLuint64 getAccumulationTextureId64(size_t id)
	{
		return accumulationTextureId64[id];
	}

	GLuint64 getOpaqueId64()
	{
		return sceneOpaqueId64;
	}

private:

	GLuint sceneOpaqueId;
	GLuint64 sceneOpaqueId64;

	GLuint accumulationTextureId[2];
	GLuint64 accumulationTextureId64[2];

	GLuint accumulationFramebufferId;

	GLfloat opacity, weightParameter;

	int imageWidth, imageHeight;
};