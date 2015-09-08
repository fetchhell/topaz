#include "WeightedBlendedOIT.h"

WeightedBlendedOIT::WeightedBlendedOIT() : opacity(0.5f), weightParameter(0.5f), imageWidth(0), imageHeight(0)
{
}

void WeightedBlendedOIT::InitAccumulationRenderTargets(int32_t width, int32_t height)
{
	this->imageWidth = width;
	this->imageHeight = height;

	glGenTextures(1, &this->sceneOpaqueId);

	/* scene non transparent */

	glBindTexture(GL_TEXTURE_RECTANGLE, this->sceneOpaqueId);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, this->imageWidth, this->imageHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	glGenTextures(2, this->accumulationTextureId);

	/* rgba texture */

	glBindTexture(GL_TEXTURE_RECTANGLE, this->accumulationTextureId[0]);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, this->imageWidth, this->imageHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	/* alpha texture */

	glBindTexture(GL_TEXTURE_RECTANGLE, this->accumulationTextureId[1]);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8, this->imageWidth, this->imageHeight, 0, GL_RED, GL_FLOAT, nullptr);

	glGenFramebuffers(1, &this->accumulationFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, this->accumulationFramebufferId);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, this->accumulationTextureId[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, this->accumulationTextureId[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, this->sceneOpaqueId, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (GLEW_ARB_bindless_texture)
	{
		this->accumulationTextureId64[0] = glGetTextureHandleARB(this->accumulationTextureId[0]);
		this->accumulationTextureId64[1] = glGetTextureHandleARB(this->accumulationTextureId[1]);
		this->sceneOpaqueId64 = glGetTextureHandleARB(this->sceneOpaqueId);
		glMakeTextureHandleResidentARB(this->accumulationTextureId64[0]);
		glMakeTextureHandleResidentARB(this->accumulationTextureId64[1]);
		glMakeTextureHandleResidentARB(this->sceneOpaqueId64);
	}
	
	CHECK_GL_ERROR();
}

void WeightedBlendedOIT::DeleteAccumulationRenderTargets()
{
	glDeleteFramebuffers(1, &this->accumulationFramebufferId);
	glDeleteTextures(2, this->accumulationTextureId);
}
