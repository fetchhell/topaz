#pragma once

#include "includeAll.h"
#include "WeightedBlendedOIT.h"
#include "Brush.h"

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

	void pushTokenParameters(NVTokenSequence& sequence, size_t& offset, std::string& stream, GLuint fbo, GLuint state);
	void setTokenBuffers(TopazGLModel* model, std::string& stream, bool cornerPoints = false);

	// change
	void initCommandListWeightBlended();

	void updateCommandListState();

	void initBuffer(GLenum target, GLuint& buffer, GLuint64& buffer64, 
		GLsizeiptr size, const GLvoid* data, bool ismutable = false);

	void drawModel(GLenum mode, NvGLSLProgram& program, TopazGLModel& model);

	void renderTokenListWeightedBlendedOIT();

	/* checking result of standart draw ( without command list ) */ 
	void TopazSample::drawStandard();
	void TopazSample::renderStandartWeightedBlendedOIT();

	/* command list inited */
	bool isTokenInternalsInited;

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
		CmdList()
		{
		}

		StateIncarnation  state;
		StateIncarnation  captured;

		/* state objects of base opaque draw */
		std::map<GLenum, GLuint> stateObjects;

		/* state objects of weight blended algorithm */
		std::map<GLenum, GLuint> stateObjectsWeightBlended;

		GLuint          tokenBuffer;
		GLuint          tokenCmdList;

		/* tokens weight blended */
		GLuint			tokenBufferWeightBlended;
		GLuint			tokenCmdListWeightBlended;

		NVTokenSequence tokenSequence;
		NVTokenSequence tokenSequenceList;

		/* token sequence weight blended */
		NVTokenSequence tokenSequenceWeightBlended;
		NVTokenSequence tokenSequenceListWeightBlended;

		std::string     tokenData;

		/* token data weight blended */
		std::string		tokenDataWeightBlended;

	} cmdlist;

	struct Textures
	{
		Textures() : sceneColor(0), sceneDepth(0), skybox(0)
		{
		}

		GLuint sceneColor,
			   sceneDepth,
			   skybox;
	} textures;

	struct TexturesAddress64
	{
		TexturesAddress64() : sceneColor(0), sceneDepth(0), skybox(0)
		{
		}

		GLuint64 sceneColor,
				 sceneDepth,
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
		uniformBuffer() : sceneUbo(0), sceneUbo64(0), identityUbo(0), 
						identityUbo64(0), weightBlendedUbo(0), weightBlendedUbo64(0)
		{
		}

		GLuint   sceneUbo;
		GLuint64 sceneUbo64;

		GLuint	 identityUbo;
		GLuint64 identityUbo64;

		GLuint weightBlendedUbo;
		GLuint64 weightBlendedUbo64;

	} ubos;

	struct SceneData
	{
		nv::matrix4f modelViewProjection;
		GLuint64 sceneDepthId64;
		float depthScale;
	} sceneData;

	struct IdentityData
	{
		nv::matrix4f identity;
	} identityData;

	struct ObjectData
	{
		nv::vec4f objectID;
		nv::vec4f objectColor;
		GLuint64  skybox;
		GLuint64  pattern;
	} objectData;

	struct WeightBlendedData
	{
		GLuint64 background;
		GLuint64 colorTex0;
		GLuint64 colorTex1;
	} weightBlendedData;

	struct BrushData
	{
		GLubyte pattern[128];
	} brushData;

	struct FullScreenRectangle
	{
		FullScreenRectangle() : vboFullScreen(0), vboFullScreen64(0)
		{
		}

		GLuint vboFullScreen;
		GLuint64 vboFullScreen64;
	} fullScreenRectangle;

	std::map<std::string, std::unique_ptr<NvGLSLProgram> > shaderPrograms;
	std::vector<std::unique_ptr<TopazGLModel> >  models;

	std::unique_ptr<WeightedBlendedOIT> oit;
	std::unique_ptr<BrushStyles> brushStyle;

	nv::vec4f sceneBackgroundColor;
};