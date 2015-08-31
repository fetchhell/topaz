#pragma once

#include "includeAll.h"
#include "WeightedBlendedOIT.h"

using namespace nvtoken;

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

	void loadModel(std::string filename, GLuint program, bool calculateCornerPoints = false);

	void compileShaders(std::string name,
						const char* vertexShaderFilename, 
						const char* fragmentShaderFilename,
						const char* geometryShaderFilename = nullptr);

private:

	void initScene();
	void initCommandList();
	void initFramebuffers(int32_t width, int32_t height);

	void updateCommandListState();

	void initBuffer(GLenum target, GLuint& buffer, GLuint64& buffer64, 
		GLsizeiptr size, const GLvoid* data, bool ismutable = false);

	void drawModel(GLenum mode, NvGLSLProgram& program, TopazGLModel& model);

	void renderTokenListWeightedBlendedOIT();

	/* checking result of standart draw ( without command list ) */ 
	void TopazSample::drawStandard();
	void TopazSample::renderStandartWeightedBlendedOIT();

	/* ARB_bindless_texture support */
	bool bindlessVboUbo;

	/* NV_uniform_buffer_unified_memory support */
	bool hwsupport;

	enum DrawMode 
	{
		DRAW_STANDARD,
		DRAW_TOKEN_LIST,
		DRAW_WEIGHT_BLENDED_STANDARD,
		DRAW_WEIGHT_BLENDED_TOKEN_LIST
	};

	uint32_t drawMode;

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
		CmdList() : stateObjectDraw(0), stateObjectGeometryDraw(0), stateObjectLinesDraw(0),
			tokenBuffer(0), tokenCmdList(0)
		{
		}

		StateIncarnation  state;
		StateIncarnation  captured;

		GLuint          stateObjectDraw;
		GLuint			stateObjectGeometryDraw;
		GLuint			stateObjectLinesDraw;

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
		uniformBuffer() : sceneUbo(0), sceneUbo64(0), weightBlendedUbo(0), weightBlendedUbo64(0)
		{
		}

		GLuint   sceneUbo;
		GLuint64 sceneUbo64;

		GLuint weightBlendedUbo;
		GLuint64 weightBlendedUbo64;

	} ubos;

	struct SceneData
	{
		nv::matrix4f modelViewProjection;
		float depthScale;
	} sceneData;

	struct ObjectData
	{
		nv::vec4f objectID;
		nv::vec4f objectColor;
		GLuint64  skybox;
	} objectData;

	struct WeightBlendedData
	{
		GLuint64 background;
		GLuint64 colorTex0;
		GLuint64 colorTex1;
	} weightBlendedData;

	struct DepthPeelingData
	{
	};

	std::map<std::string, std::unique_ptr<NvGLSLProgram> > shaderPrograms;
	std::vector<std::unique_ptr<TopazGLModel> >  models;

	std::vector<std::unique_ptr<TopazGLModel> > opaqueModels;
	std::vector<std::unique_ptr<TopazGLModel> > transparentModels;

	std::unique_ptr<WeightedBlendedOIT> oit;

	nv::vec4f sceneBackgroundColor;
};