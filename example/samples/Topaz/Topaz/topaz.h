#pragma once

#include "NvAppBase/NvSampleApp.h"
#include "NvAppBase/NvInputTransformer.h"

#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvTimers.h"
#include "NvGLUtils/NvImage.h"
#include "TopazGLModel.h"
#include "NvModel/NvModel.h"
#include "NV/NvLogs.h"
#include "NV/NvMath.h"

#include <vector>
#include <string>

#include <memory>

#include "nvtoken.hpp"

using namespace nvtoken;

#define VERTEX_POS    0
#define VERTEX_NORMAL 1
#define VERTEX_UV     2

#define UBO_SCENE     0
#define UBO_OBJECT    1

class TopazSample : public NvSampleApp
{
public:

	TopazSample(NvPlatformContext* platform);
	~TopazSample();

	/* virtual functions */
	void initUI();
	void initRendering();
	void draw();
	void reshape(int32_t width, int32_t height);

	void configurationCallback(NvEGLConfiguration& config);

	void loadModel(std::string filename);

	void compileShaders();

private:

	void initScene();
	void initCommandList();
	void initFramebuffers(int32_t width, int32_t height);

	void updateCommandListState();

	bool bindlessVboUbo;
	bool hwsupport;

	struct StateIncarnation 
	{
		StateIncarnation() : programIncarnation(0), fboIncarnation(0)
		{
		}

		bool operator ==(const StateIncarnation& other) const
		{
			return memcmp(this, &other, sizeof(StateIncarnation)) == 0;
		}

		bool operator !=(const StateIncarnation& other) const
		{
			return memcmp(this, &other, sizeof(StateIncarnation)) != 0;
		}

		GLuint  programIncarnation;
		GLuint  fboIncarnation;
	};

	struct CmdList 
	{
		CmdList() : stateObjectDraw(0), tokenBuffer(0), tokenCmdList(0)
		{
		}

		StateIncarnation  state;
		StateIncarnation  captured;

		GLuint          stateObjectDraw;

		GLuint          tokenBuffer;
		GLuint          tokenCmdList;

		NVTokenSequence tokenSequence;
		NVTokenSequence tokenSequenceList;

		std::string     tokenData;
	} cmdlist;

	struct Textures
	{
		Textures() : sceneColor(0), sceneDepthStencil(0), skybox(0)
		{
		}

		GLuint sceneColor,
			   sceneDepthStencil,
			   skybox;
	} textures;

	struct TexturesAddress64
	{
		TexturesAddress64() : sceneColor(0), sceneDepthStencil(0), skybox(0)
		{
		}

		GLuint64 sceneColor,
				 sceneDepthStencil,
				 skybox;
	} texturesAddress64;

	struct framebuffer
	{
		framebuffer() : scene(0)
		{
		}

		GLuint scene;
	} fbos;

	struct uniformBuffer
	{
		uniformBuffer() : sceneUbo(0), sceneUbo64(0)
		{
		}

		GLuint   sceneUbo;
		GLuint64 sceneUbo64;
	} ubos;

	struct SceneData
	{
		nv::matrix4f modelViewProjection;
	} sceneData;

	struct ObjectData
	{
		nv::vec4f objectID;
		nv::vec4f objectColor;
		GLuint64  skybox;
	} objectData;

	std::vector<std::unique_ptr<NvGLSLProgram> > shaderPrograms;
	std::vector<std::unique_ptr<TopazGLModel> >	 models;

	// check
	void TopazSample::drawStandard();
};