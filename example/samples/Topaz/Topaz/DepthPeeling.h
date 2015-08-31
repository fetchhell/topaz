#pragma once

#include "includeAll.h"

class DepthPeeling
{
public:

	DepthPeeling();

	void InitDepthPeelingRenderTargets();


private:
	
	GLuint frontFramebufferId[2];

	GLuint frontDepthTextureId[2];
	GLuint frontColorTextureId[2];

	GLuint frontColorBlenderTextureId;
	GLuint frontColorBlenderFramebufferId;

};