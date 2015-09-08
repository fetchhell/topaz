#include "topaz.h"
#include <windows.h>

TopazSample::TopazSample(NvPlatformContext* platform) : NvSampleApp(platform, "Topaz Sample"), drawMode(DRAW_STANDARD)
{
	oit = std::unique_ptr<WeightedBlendedOIT>(new WeightedBlendedOIT);
	brushStyle = std::unique_ptr<BrushStyles>(new BrushStyles);

	isTokenInternalsInited = false;

	sceneBackgroundColor = nv::vec4f(0.2f, 0.2f, 0.2f, 0.0f);
	m_transformer->setTranslationVec(nv::vec3f(0.f, -0.1f, -2.5f));
	forceLinkHack();
}

TopazSample::~TopazSample()
{
}

void TopazSample::configurationCallback(NvEGLConfiguration& config)
{
	config.depthBits = 24;
	config.stencilBits = 0;
	config.apiVer = NvGfxAPIVersionGL4_4();
}

void TopazSample::initUI()
{
	if (mTweakBar) 
	{
		NvTweakEnum<uint32_t> enumVals[] = 
		{
			{ "standard", DRAW_STANDARD },
			{ "nvcmdlist list", DRAW_TOKEN_LIST },
			{ "weight blended standard", DRAW_WEIGHT_BLENDED_STANDARD },
			{ "weight blended token list", DRAW_WEIGHT_BLENDED_TOKEN_LIST }
		};

		mTweakBar->addPadding();

		/* draw mode : DRAW_STANDART, DRAW_TOKEN_LIST */
		mTweakBar->addEnum("Draw Mode:", drawMode, enumVals, TWEAKENUM_ARRAYSIZE(enumVals));

		/* weighted blended oit parameters */
		mTweakBar->addValue("Opacity:", oit->getOpacity(), 0.0f, 1.0f);
		mTweakBar->addValue("Weight Parameter:", oit->getWeightParameter(), 0.1f, 1.0f);

		mTweakBar->syncValues();
	}
}

void TopazSample::initRendering()
{
	if (!GLEW_ARB_bindless_texture)
	{
		LOGI("This sample requires ARB_bindless_texture");
		exit(EXIT_FAILURE);
	}

	bindlessVboUbo = GLEW_NV_vertex_buffer_unified_memory && requireExtension("GL_NV_uniform_buffer_unified_memory", false);

	NvAssetLoaderAddSearchPath("Topaz/Topaz");

	if(!requireMinAPIVersion(NvGfxAPIVersionGL4_4(), true))
	{
		exit(EXIT_FAILURE);
	}

	compileShaders("draw", "shaders/vertex.glsl", "shaders/fragment.glsl");
	/*
	If needed geometry shader:
		compileShaders("geometry", "shaders/vertex.glsl", "shaders/fragment.glsl", "shaders/geometry.glsl");
	*/
	compileShaders("weightBlended", "shaders/vertex.glsl", "shaders/fragmentBlendOIT.glsl");
	compileShaders("weightBlendedFinal", "shaders/vertexOIT.glsl", "shaders/fragmentFinalOIT.glsl");

	// like as glClearBufferfv for nv_command_list
	compileShaders("clear", "shaders/vertexOIT.glsl", "shaders/clear.glsl");

	loadModel("models/background.obj", shaderPrograms["draw"]->getProgram());

	loadModel("models/way3.obj", shaderPrograms["draw"]->getProgram(), true);
	loadModel("models/way4.obj", shaderPrograms["draw"]->getProgram(), true);
	loadModel("models/way5.obj", shaderPrograms["draw"]->getProgram());

	loadModel("models/formular.obj", shaderPrograms["draw"]->getProgram());

	textures.skybox = NvImage::UploadTextureFromDDSFile("textures/sky_cube.dds");

	cmdlist.state.programIncarnation++;

	CHECK_GL_ERROR();
}

void TopazSample::reshape(int32_t width, int32_t height)
{
	initFramebuffers(width, height);
	oit->InitAccumulationRenderTargets(width, height);

	initScene();
	initCommandList();
	initCommandListWeightBlended();

	CHECK_GL_ERROR();
}

void TopazSample::draw()
{
	nv::matrix4f projection = nv::perspective(projection, 45.f * NV_PI / 180.f, m_width / float(m_height), 0.1f, 10.f);
	sceneData.sceneDepthId64 = texturesAddress64.sceneDepth;
	sceneData.modelViewProjection = projection * m_transformer->getModelViewMat();
	sceneData.depthScale = oit->getWeightParameter();

	glNamedBufferSubDataEXT(ubos.sceneUbo, 0, sizeof(SceneData), &sceneData);

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

	glViewport(0, 0, m_width, m_height);

	glClearColor(sceneBackgroundColor.x, sceneBackgroundColor.y, sceneBackgroundColor.z, sceneBackgroundColor.w);
	glClearDepthf(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (cmdlist.state != cmdlist.captured)
	{
		updateCommandListState();
	}

	if (drawMode == DRAW_STANDARD)
	{
		drawStandard();
	}
	else if (drawMode == DRAW_TOKEN_LIST)
	{
		glCallCommandListNV(cmdlist.tokenCmdList);
	}
	else if (drawMode == DRAW_WEIGHT_BLENDED_STANDARD)
	{
		renderStandartWeightedBlendedOIT();
	}
	else if (drawMode == DRAW_WEIGHT_BLENDED_TOKEN_LIST)
	{
		for (auto model = models.begin() + 1; model != models.end(); model++)
		{
			objectData.objectColor = nv::vec4f(1.0f, 1.0f, 1.0f, oit->getOpacity());
			glNamedBufferSubDataEXT((*model)->getBufferID("ubo"), 0, sizeof(ObjectData), &objectData);

			objectData.objectColor = nv::vec4f(1.0f, 0.0f, 0.0f, oit->getOpacity());
			glNamedBufferSubDataEXT((*model)->getCornerBufferID("ubo"), 0, sizeof(ObjectData), &objectData);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
		glCallCommandListNV(cmdlist.tokenCmdListWeightBlended);
	}
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos.scene);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
}

void TopazSample::compileShaders(std::string name,
								 const char* vertexShaderFilename,
								 const char* fragmentShaderFilename,
								 const char* geometryShaderFilename)
{
	std::unique_ptr<NvGLSLProgram> program(new NvGLSLProgram);
	
	int32_t sourceLength;
	std::vector<NvGLSLProgram::ShaderSourceItem> sources;

	NvGLSLProgram::ShaderSourceItem vertexShader;
	vertexShader.type = GL_VERTEX_SHADER;
	vertexShader.src  = NvAssetLoaderRead(vertexShaderFilename, sourceLength);

	sources.push_back(vertexShader);

	if (geometryShaderFilename != nullptr)
	{
		NvGLSLProgram::ShaderSourceItem geometryShader;
		geometryShader.type = GL_GEOMETRY_SHADER;
		geometryShader.src = NvAssetLoaderRead(geometryShaderFilename, sourceLength);

		sources.push_back(geometryShader);
	}

	NvGLSLProgram::ShaderSourceItem fragmentShader;
	fragmentShader.type = GL_FRAGMENT_SHADER;
	fragmentShader.src  = NvAssetLoaderRead(fragmentShaderFilename, sourceLength);
	
	sources.push_back(fragmentShader);

	program->setSourceFromStrings(sources.data(), sources.size());

	shaderPrograms[name] = std::move(program);
}

void TopazSample::loadModel(std::string filename, GLuint program, bool calculateCornerPoints)
{
	int32_t length;
	char* data = NvAssetLoaderRead(filename.c_str(), length);

	std::unique_ptr<TopazGLModel> model(new TopazGLModel());
	model->loadModelFromObjData(data);

	if (calculateCornerPoints)
	{
		model->calculateCornerPoints(1.0f);
	}

	model->rescaleModel(1.0f);
	model->setProgram(program);

	models.push_back(std::move(model));

	NvAssetLoaderFree(data);
	CHECK_GL_ERROR();
}

void TopazSample::initBuffer(GLenum target, GLuint& buffer, GLuint64& buffer64, 
	GLsizeiptr size, const GLvoid* data, bool ismutable)
{
	if (buffer)
	{
		glDeleteBuffers(1, &buffer);
	}
	glGenBuffers(1, &buffer);

	if (!ismutable)
	{
		glNamedBufferStorageEXT(buffer, size, data, 0);
	}
	else
	{
		glBindBuffer(target, buffer);
		glBufferData(target, size, data, GL_DYNAMIC_DRAW);
		glBindBuffer(target, 0);
	}

	if (bindlessVboUbo)
	{
		glGetNamedBufferParameterui64vNV(buffer, GL_BUFFER_GPU_ADDRESS_NV, &buffer64);
		glMakeNamedBufferResidentNV(buffer, GL_READ_ONLY);
	}
}

void TopazSample::initScene()
{
	// initializate skybox
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, textures.skybox);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		texturesAddress64.skybox = glGetTextureHandleARB(textures.skybox);
		glMakeTextureHandleResidentARB(texturesAddress64.skybox);
	}

	// init fullscreen quad
	{
		const float position[] = 
		{
			-1.0f, -1.0f, 0.0,
			1.0f, -1.0f, 0.0,
			-1.0f, 1.0f, 0.0, 
			1.0f, 1.0f, 0.0
		};

		initBuffer(GL_ARRAY_BUFFER, fullScreenRectangle.vboFullScreen, fullScreenRectangle.vboFullScreen64,
			12 * sizeof(float),
			position); 
	}

	// initializate scene & weightBlended ubo
	{
		initBuffer(GL_UNIFORM_BUFFER, ubos.sceneUbo, ubos.sceneUbo64,
			sizeof(SceneData),
			nullptr,
			true); // mutable buffer

		initBuffer(GL_UNIFORM_BUFFER, ubos.identityUbo, ubos.identityUbo64,
			sizeof(IdentityData),
			identityData.identity._array,
			true); // mutable buffer

		WeightBlendedData weightBlendedData;
		weightBlendedData.background = texturesAddress64.sceneColor;
		weightBlendedData.colorTex0 = oit->getAccumulationTextureId64(0);
		weightBlendedData.colorTex1 = oit->getAccumulationTextureId64(1);

		initBuffer(GL_UNIFORM_BUFFER, ubos.weightBlendedUbo, ubos.weightBlendedUbo64,
			sizeof(WeightBlendedData),
			&weightBlendedData,
			true); // mutable buffer
	}

	brushStyle->brushPattern8to32(QtStyles::Dense1Pattern);

	objectData.objectID = nv::vec4f(0.0);
	objectData.objectColor = nv::vec4f(1.0f, 1.0f, 1.0f, oit->getOpacity());
	objectData.skybox = texturesAddress64.skybox;
	objectData.pattern = brushStyle->getTextureId64();

	for (auto & model : models)
	{
		model->getModel()->compileModel(NvModelPrimType::TRIANGLES);

		/* ibo */
		initBuffer(GL_ELEMENT_ARRAY_BUFFER, model->getBufferID("ibo"), model->getBufferID64("ibo"),
			model->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES) * sizeof(uint32_t),
			model->getModel()->getCompiledIndices(NvModelPrimType::TRIANGLES));

		/* vbo */
		initBuffer(GL_ARRAY_BUFFER, model->getBufferID("vbo"), model->getBufferID64("vbo"),
			model->getModel()->getCompiledVertexCount() * model->getModel()->getCompiledVertexSize() * sizeof(float), 
			model->getModel()->getCompiledVertices());
		
		if (model->cornerPointsExists())
		{
			/* ibo corner */
			initBuffer(GL_ELEMENT_ARRAY_BUFFER, model->getCornerBufferID("ibo"), model->getCornerBufferID64("ibo"),
				model->getCornerIndices().size() * sizeof(uint32_t), 
				model->getCornerIndices().data());

			/* vbo corner */
			initBuffer(GL_ARRAY_BUFFER, model->getCornerBufferID("vbo"), model->getCornerBufferID64("vbo"),
				model->getCorners().size() * sizeof(nv::vec3f),
				model->getCorners().data());

			/* ubo corner */
			ObjectData curObjectData;
			curObjectData.objectID = nv::vec4f(1.0f);
			curObjectData.objectColor = nv::vec4f(1.0f, 0.0f, 0.0f, oit->getOpacity());
			curObjectData.pattern = brushStyle->getTextureId64();

			initBuffer(GL_UNIFORM_BUFFER, model->getCornerBufferID("ubo"), model->getCornerBufferID64("ubo"), 
				sizeof(ObjectData),
				&curObjectData, 
				true); // mutable buffer
		}

		/* ubo */
		initBuffer(GL_UNIFORM_BUFFER, model->getBufferID("ubo"), model->getBufferID64("ubo"),
			sizeof(ObjectData),
			&objectData,
			true); // mutable buffer

		objectData.objectID = nv::vec4f(1.0);
	}
}

typedef void(*NVPproc)(void);
NVPproc sysGetProcAddress(const char* name) 
{
	return (NVPproc)wglGetProcAddress(name);
}

void TopazSample::setTokenBuffers(TopazGLModel* model, std::string& stream, bool cornerPoints)
{
	GLuint vboId = (!cornerPoints) ? model->getBufferID("vbo") : model->getCornerBufferID("vbo");
	GLuint vboId64 = (!cornerPoints) ? model->getBufferID64("vbo") : model->getCornerBufferID64("vbo");

	GLuint iboId = (!cornerPoints) ? model->getBufferID("ibo") : model->getCornerBufferID("ibo");
	GLuint iboId64 = (!cornerPoints) ? model->getBufferID64("ibo") : model->getCornerBufferID64("ibo");

	GLuint uboId = (!cornerPoints) ? model->getBufferID("ubo") : model->getCornerBufferID("ubo");
	GLuint uboId64 = (!cornerPoints) ? model->getBufferID64("ubo") : model->getCornerBufferID64("ubo");

	NVTokenVbo vbo;
	vbo.setBinding(0);
	vbo.setBuffer(vboId, vboId64, 0);
	nvtokenEnqueue(stream, vbo);

	NVTokenIbo ibo;
	ibo.setType(GL_UNSIGNED_INT);
	ibo.setBuffer(iboId, iboId64);
	nvtokenEnqueue(stream, ibo);

	NVTokenUbo ubo;
	ubo.setBuffer(uboId, uboId64, 0, sizeof(ObjectData));
	ubo.setBinding(UBO_OBJECT, NVTOKEN_STAGE_VERTEX);
	nvtokenEnqueue(stream, ubo);
	ubo.setBinding(UBO_OBJECT, NVTOKEN_STAGE_FRAGMENT);
	nvtokenEnqueue(stream, ubo);
}

void TopazSample::pushTokenParameters(NVTokenSequence& sequence, size_t& offset, std::string& stream, GLuint fbo, GLuint state)
{
	sequence.offsets.push_back(offset);
	sequence.sizes.push_back(GLsizei(stream.size() - offset));
	sequence.fbos.push_back(fbo);
	sequence.states.push_back(state);

	offset = stream.size();
}

void TopazSample::initCommandList()
{
	if (!isTokenInternalsInited)
	{
		hwsupport = init_NV_command_list(sysGetProcAddress) ? true : false;
		nvtokenInitInternals(hwsupport, bindlessVboUbo);

		isTokenInternalsInited = true;
	}

	enum States
	{
		STATE_DRAW,
		STATE_GEOMETRY_DRAW,
		STATE_LINES_DRAW,
		STATES_COUNT
	};
	
	if (hwsupport)
	{
		for (size_t i = 0; i < STATES_COUNT; i++)
		{
			glCreateStatesNV(1, &cmdlist.stateObjects[i]);
		}

		glGenBuffers(1, &cmdlist.tokenBuffer);
		glCreateCommandListsNV(1, &cmdlist.tokenCmdList);
	}

	NVTokenSequence& seq = cmdlist.tokenSequence;
	std::string& stream = cmdlist.tokenData;
	size_t offset = 0;

	{
		NVTokenUbo  ubo;
		ubo.setBuffer(ubos.sceneUbo, ubos.sceneUbo64, 0, sizeof(SceneData));
		ubo.setBinding(UBO_SCENE, NVTOKEN_STAGE_VERTEX);
		nvtokenEnqueue(stream, ubo);
		ubo.setBinding(UBO_SCENE, NVTOKEN_STAGE_FRAGMENT);
		nvtokenEnqueue(stream, ubo);
	}
	
	for (auto & model : models)
	{
		setTokenBuffers(model.get(), stream);

		NVTokenDrawElems  draw;
		draw.setParams(model->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES));
		draw.setMode(GL_TRIANGLES);
		nvtokenEnqueue(stream, draw);
	}
	pushTokenParameters(seq, offset, stream, fbos.scene, cmdlist.stateObjects[STATE_DRAW]);
	
	for (auto & model : models)
	{
		if (model->cornerPointsExists())
		{
			setTokenBuffers(model.get(), stream, true);

			NVTokenDrawElems drawCorner;
			drawCorner.setParams(model->getCornerIndices().size());
			drawCorner.setMode(GL_LINE_STRIP);
			nvtokenEnqueue(stream, drawCorner);
		}
	}
	pushTokenParameters(seq, offset, stream, fbos.scene, cmdlist.stateObjects[STATE_LINES_DRAW]);

	if (hwsupport)
	{
		glNamedBufferStorageEXT(cmdlist.tokenBuffer, cmdlist.tokenData.size(), &cmdlist.tokenData.at(0), 0);

		cmdlist.tokenSequenceList = cmdlist.tokenSequence;
		for (size_t i = 0; i < cmdlist.tokenSequenceList.offsets.size(); i++)
		{
			cmdlist.tokenSequenceList.offsets[i] += (GLintptr) &cmdlist.tokenData.at(0);
		}
	}

	updateCommandListState();
}

void TopazSample::initCommandListWeightBlended()
{
	if (!isTokenInternalsInited)
	{
		hwsupport = init_NV_command_list(sysGetProcAddress) ? true : false;
		nvtokenInitInternals(hwsupport, bindlessVboUbo);

		isTokenInternalsInited = true;
	}

	enum States
	{
		STATE_CLEAR,
		STATE_OPAQUE,
		STATE_TRANSPARENT,
		STATE_TRASPARENT_LINES,
		STATE_COMPOSITE,
		STATES_COUNT
	};

	if (hwsupport)
	{
		for (size_t i = 0; i < STATES_COUNT; i++)
		{
			glCreateStatesNV(1, &cmdlist.stateObjectsWeightBlended[i]);
		}

		glGenBuffers(1, &cmdlist.tokenBufferWeightBlended);
		glCreateCommandListsNV(1, &cmdlist.tokenCmdListWeightBlended);
	}

	NVTokenSequence& seq = cmdlist.tokenSequenceWeightBlended;
	std::string& stream = cmdlist.tokenDataWeightBlended;
	size_t offset = 0;

	{
		NVTokenUbo  ubo;
		ubo.setBuffer(ubos.sceneUbo, ubos.sceneUbo64, 0, sizeof(SceneData));
		ubo.setBinding(UBO_SCENE, NVTOKEN_STAGE_VERTEX);
		nvtokenEnqueue(stream, ubo);
		ubo.setBinding(UBO_SCENE, NVTOKEN_STAGE_FRAGMENT);
		nvtokenEnqueue(stream, ubo);
	}

	// 1. render 'background' into framebuffer 'fbos.scene' 
	{
		auto& model = models.at(0);
		setTokenBuffers(model.get(), stream);

		NVTokenDrawElems  draw;
		draw.setParams(model->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES));
		draw.setMode(GL_TRIANGLES);
		nvtokenEnqueue(stream, draw);

		pushTokenParameters(seq, offset, stream, fbos.scene, cmdlist.stateObjectsWeightBlended[STATE_OPAQUE]);
	}

	// 2. geometry pass OIT 
	for (auto model = models.begin() + 1; model != models.end(); model++)
	{
		// like call glClearBufferfv
		{
			NVTokenVbo vbo;
			vbo.setBinding(0);
			vbo.setBuffer(fullScreenRectangle.vboFullScreen, fullScreenRectangle.vboFullScreen64, 0);
			nvtokenEnqueue(stream, vbo);

			NVTokenUbo ubo;
			ubo.setBuffer(ubos.identityUbo, ubos.identityUbo64, 0, sizeof(IdentityData));
			ubo.setBinding(UBO_IDENTITY, NVTOKEN_STAGE_VERTEX);
			nvtokenEnqueue(stream, ubo);

			NVTokenDrawArrays  draw;
			draw.setParams(4, 0);
			draw.setMode(GL_TRIANGLE_STRIP);
			nvtokenEnqueue(stream, draw);
		}
		pushTokenParameters(seq, offset, stream, oit->getFramebufferID(), cmdlist.stateObjectsWeightBlended[STATE_CLEAR]);

		// 2. geometry pass
		{
			setTokenBuffers((*model).get(), stream);

			NVTokenDrawElems  draw;
			draw.setParams((*model)->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES));
			draw.setMode(GL_TRIANGLES);
			nvtokenEnqueue(stream, draw);
		}
		pushTokenParameters(seq, offset, stream, oit->getFramebufferID(), cmdlist.stateObjectsWeightBlended[STATE_TRANSPARENT]);

		{
			setTokenBuffers((*model).get(), stream, true);

			NVTokenDrawElems  draw;
			draw.setParams((*model)->getCornerIndices().size());
			draw.setMode(GL_LINE_STRIP);
			nvtokenEnqueue(stream, draw);
		}
		pushTokenParameters(seq, offset, stream, oit->getFramebufferID(), cmdlist.stateObjectsWeightBlended[STATE_TRASPARENT_LINES]);

		// 3. composite pass
		{
			NVTokenVbo vbo;
			vbo.setBinding(0);
			vbo.setBuffer(fullScreenRectangle.vboFullScreen, fullScreenRectangle.vboFullScreen64, 0);
			nvtokenEnqueue(stream, vbo);

			NVTokenUbo ubo;
			ubo.setBuffer(ubos.identityUbo, ubos.identityUbo64, 0, sizeof(IdentityData));
			ubo.setBinding(UBO_IDENTITY, NVTOKEN_STAGE_VERTEX);
			nvtokenEnqueue(stream, ubo);

			NVTokenUbo uboWeightBlended;
			uboWeightBlended.setBuffer(ubos.weightBlendedUbo, ubos.weightBlendedUbo64, 0, sizeof(WeightBlendedData));
			uboWeightBlended.setBinding(UBO_OIT, NVTOKEN_STAGE_FRAGMENT);
			nvtokenEnqueue(stream, uboWeightBlended);

			NVTokenDrawArrays  draw;
			draw.setParams(4, 0);
			draw.setMode(GL_TRIANGLE_STRIP);
			nvtokenEnqueue(stream, draw);
		}
		pushTokenParameters(seq, offset, stream, fbos.scene, cmdlist.stateObjectsWeightBlended[STATE_COMPOSITE]);
	}
	
	if (hwsupport)
	{
		glNamedBufferStorageEXT(cmdlist.tokenBufferWeightBlended, cmdlist.tokenDataWeightBlended.size(), &cmdlist.tokenDataWeightBlended.at(0), 0);

		cmdlist.tokenSequenceListWeightBlended = cmdlist.tokenSequenceWeightBlended;
		for (size_t i = 0; i < cmdlist.tokenSequenceListWeightBlended.offsets.size(); i++)
		{
			cmdlist.tokenSequenceListWeightBlended.offsets[i] += (GLintptr)&cmdlist.tokenDataWeightBlended.at(0);
		}
	}
	
	glEnableVertexAttribArray(VERTEX_POS);
	glVertexAttribFormat(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(VERTEX_POS, 0);

	glEnable(GL_DEPTH_TEST);

	// 1. opaque modes
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

		shaderPrograms["draw"]->enable();

		glBindVertexBuffer(0, 0, 0, 9 * sizeof(float));
		glStateCaptureNV(cmdlist.stateObjectsWeightBlended[STATE_OPAQUE], GL_TRIANGLES);
		
		shaderPrograms["draw"]->disable();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glDisable(GL_DEPTH_TEST);

	// like a glClearBufferfv
	{
		glBindFramebuffer(GL_FRAMEBUFFER, oit->getFramebufferID());

		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);

		shaderPrograms["clear"]->enable();

		glBindVertexBuffer(0, 0, 0, sizeof(nv::vec3f));
		glStateCaptureNV(cmdlist.stateObjectsWeightBlended[STATE_CLEAR], GL_TRIANGLES);

		shaderPrograms["clear"]->disable();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// 2. oit first step
	{
		glBindFramebuffer(GL_FRAMEBUFFER, oit->getFramebufferID());

		// ???
		//glEnable(GL_POLYGON_STIPPLE);
		//glPolygonStipple(brushStyle->brushPattern8to32(QtStyles::DiagCrossPattern).data());

		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

		shaderPrograms["weightBlended"]->enable();

		glBindVertexBuffer(0, 0, 0, 9 * sizeof(float));
		glStateCaptureNV(cmdlist.stateObjectsWeightBlended[STATE_TRANSPARENT], GL_TRIANGLES);

		glBindVertexBuffer(0, 0, 0, sizeof(nv::vec3f));
		glStateCaptureNV(cmdlist.stateObjectsWeightBlended[STATE_TRASPARENT_LINES], GL_LINES);

		shaderPrograms["weightBlended"]->disable();

		// ???
		//glDisable(GL_POLYGON_STIPPLE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// 3. oit second step
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
		glDisable(GL_BLEND);

		shaderPrograms["weightBlendedFinal"]->enable();

		glBindVertexBuffer(0, 0, 0, sizeof(nv::vec3f));
		glStateCaptureNV(cmdlist.stateObjectsWeightBlended[STATE_COMPOSITE], GL_TRIANGLES);

		shaderPrograms["weightBlendedFinal"]->disable();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// compile command list
	NVTokenSequence& sequenceList = cmdlist.tokenSequenceListWeightBlended;
	glCommandListSegmentsNV(cmdlist.tokenCmdListWeightBlended, 1);
	glListDrawCommandsStatesClientNV(cmdlist.tokenCmdListWeightBlended, 0, (const void**)&sequenceList.offsets[0], &sequenceList.sizes[0], &sequenceList.states[0], &sequenceList.fbos[0], int(sequenceList.states.size()));
	glCompileCommandListNV(cmdlist.tokenCmdListWeightBlended);
}

void TopazSample::updateCommandListState()
{
	enum StateObjects
	{
		STATE_DRAW,
		STATE_GEOMETRY_DRAW,
		STATE_LINES_DRAW,
		STATES_COUNT
	};

	if (cmdlist.state.programIncarnation != cmdlist.captured.programIncarnation)
	{
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

			glEnable(GL_DEPTH_TEST);

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(1, 1);

			glEnableVertexAttribArray(VERTEX_POS);
			glVertexAttribFormat(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, models.at(0)->getModel()->getCompiledPositionOffset());
			glVertexAttribBinding(VERTEX_POS, 0);

			 glBindVertexBuffer(0, 0, 0, models.at(0)->getModel()->getCompiledVertexSize() * sizeof(float));

			if (hwsupport)
			{
				glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, 0, 0);
				glBufferAddressRangeNV(GL_ELEMENT_ARRAY_ADDRESS_NV, 0, 0, 0);
				glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, UBO_OBJECT, 0, 0);
				glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, UBO_SCENE, 0, 0);
			}
			
			shaderPrograms["draw"]->enable();
			glStateCaptureNV(cmdlist.stateObjects[STATE_DRAW], GL_TRIANGLES);

			glBindVertexBuffer(0, 0, 0, sizeof(nv::vec3f));

			glStateCaptureNV(cmdlist.stateObjects[STATE_LINES_DRAW], GL_LINES);

			glDisableVertexAttribArray(VERTEX_POS);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glDisable(GL_DEPTH_TEST);
	}

	if (hwsupport && (
		cmdlist.state.programIncarnation != cmdlist.captured.programIncarnation ||
		cmdlist.state.fboIncarnation != cmdlist.captured.fboIncarnation))
	{
		NVTokenSequence &seq = cmdlist.tokenSequenceList;
		glCommandListSegmentsNV(cmdlist.tokenCmdList, 1);
		glListDrawCommandsStatesClientNV(cmdlist.tokenCmdList, 0, (const void**)&seq.offsets[0], &seq.sizes[0], &seq.states[0], &seq.fbos[0], int(seq.states.size()));
		glCompileCommandListNV(cmdlist.tokenCmdList);
	}
	
	cmdlist.captured = cmdlist.state;
}

void TopazSample::initFramebuffers(int32_t width, int32_t height)
{
	if (textures.sceneColor && GLEW_ARB_bindless_texture)
	{
		glMakeTextureHandleNonResidentARB(texturesAddress64.sceneColor);
		glMakeTextureHandleNonResidentARB(texturesAddress64.sceneDepth);
	}

	if (textures.sceneColor)
	{
		glDeleteTextures(1, &textures.sceneColor);
	}
	glGenTextures(1, &textures.sceneColor);

	glBindTexture(GL_TEXTURE_RECTANGLE, textures.sceneColor);
	glTexStorage2D(GL_TEXTURE_RECTANGLE, 1, GL_RGBA16F, width, height);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	if (textures.sceneDepth)
	{
		glDeleteTextures(1, &textures.sceneDepth);
	}
	glGenTextures(1, &textures.sceneDepth);

	glBindTexture(GL_TEXTURE_RECTANGLE, textures.sceneDepth);
	glTexStorage2D(GL_TEXTURE_RECTANGLE, 1, GL_DEPTH_COMPONENT24, width, height);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	if (fbos.scene)
	{
		glDeleteFramebuffers(1, &fbos.scene);
	}
	glGenFramebuffers(1, &fbos.scene);

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, textures.sceneColor, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, textures.sceneDepth, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (GLEW_ARB_bindless_texture)
	{
		texturesAddress64.sceneColor = glGetTextureHandleARB(textures.sceneColor);
		texturesAddress64.sceneDepth = glGetTextureHandleARB(textures.sceneDepth);
		glMakeTextureHandleResidentARB(texturesAddress64.sceneColor);
		glMakeTextureHandleResidentARB(texturesAddress64.sceneDepth);
	}

	cmdlist.state.fboIncarnation++;
}

void TopazSample::drawStandard()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1, 1);

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, ubos.sceneUbo);
	
	shaderPrograms["draw"]->enable();
	{
		const char* uniformNames[] = { "objectData.objectID", "objectData.objectColor", "objectData.skybox", "objectData.pattern" };

		std::unique_ptr<GLint>  parameters(new GLint[4]);
		std::unique_ptr<GLuint> uniformIndices(new GLuint[4]);

		glGetUniformIndices(shaderPrograms["draw"]->getProgram(), 4, uniformNames, uniformIndices.get());
		glGetActiveUniformsiv(shaderPrograms["draw"]->getProgram(), 4, uniformIndices.get(), GL_UNIFORM_OFFSET, parameters.get());

		GLint* offset = parameters.get();
		CHECK_GL_ERROR();
	}

	for (auto & model : models)
	{
		drawModel(GL_TRIANGLES, *shaderPrograms["draw"], *model);
	}

	CHECK_GL_ERROR();
}

void TopazSample::drawModel(GLenum mode, NvGLSLProgram& program, TopazGLModel& model)
{
	glVertexAttribFormat(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, model.getModel()->getCompiledPositionOffset());

	glVertexAttribBinding(VERTEX_POS, 0);
	glEnableVertexAttribArray(VERTEX_POS);

	program.enable();

	program.bindTextureRect("pattern", 0, brushStyle->getTextureId());

	glBindBufferRange(GL_UNIFORM_BUFFER, UBO_OBJECT, model.getBufferID("ubo"), 0, sizeof(ObjectData));

	glBindVertexBuffer(0, model.getBufferID("vbo"), 0, model.getModel()->getCompiledVertexSize() * sizeof(float));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.getBufferID("ibo"));
	glDrawElements(mode, model.getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES), GL_UNSIGNED_INT, nullptr);
	
	if (model.cornerPointsExists())
	{
		glBindBufferRange(GL_UNIFORM_BUFFER, UBO_OBJECT, model.getCornerBufferID("ubo"), 0, sizeof(ObjectData));

		glBindVertexBuffer(0, model.getCornerBufferID("vbo"), 0, sizeof(nv::vec3f));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.getCornerBufferID("ibo"));

		glDrawElements(GL_LINE_STRIP, model.getCornerIndices().size(), GL_UNSIGNED_INT, nullptr);
	}
	
	program.disable();

	glDisableVertexAttribArray(VERTEX_POS);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexBuffer(0, 0, 0, 0);
}

void TopazSample::renderStandartWeightedBlendedOIT()
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_OFFSET_FILL);

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, ubos.sceneUbo);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_IDENTITY, ubos.identityUbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
	drawModel(GL_TRIANGLES, *shaderPrograms["draw"], *models.at(0));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	/* first pass oit */
	glDisable(GL_DEPTH_TEST);

	// TODO : change on transparent list of models
	for (auto model = models.begin() + 1; model != models.end(); model++)
	{
		/* geometry pass */
		{
			glBindFramebuffer(GL_FRAMEBUFFER, oit->getFramebufferID());

			const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, drawBuffers);

			GLfloat clearColorZero[4] = { 0.0f };
			GLfloat clearColorOne[4] = { 1.0f };

			glClearBufferfv(GL_COLOR, 0, clearColorZero);
			glClearBufferfv(GL_COLOR, 1, clearColorOne);

			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunci(0, GL_ONE, GL_ONE);
			glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

			objectData.objectColor = nv::vec4f(1.0f, 1.0f, 1.0f, oit->getOpacity());
			glNamedBufferSubDataEXT((*model)->getBufferID("ubo"), 0, sizeof(ObjectData), &objectData);

			objectData.objectColor = nv::vec4f(1.0f, 0.0f, 0.0f, oit->getOpacity());
			glNamedBufferSubDataEXT((*model)->getCornerBufferID("ubo"), 0, sizeof(ObjectData), &objectData);

			drawModel(GL_TRIANGLES, *shaderPrograms["weightBlended"], **model);

			glDisable(GL_BLEND);
			CHECK_GL_ERROR();
		}

		/* composition pass */
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
			glBindBufferRange(GL_UNIFORM_BUFFER, UBO_OIT, ubos.weightBlendedUbo, 0, sizeof(WeightBlendedData));

			// check uniform buffer offset
			{
				const char* uniformNames[] = { "weightBlendedData.background", "weightBlendedData.colorTex0", "weightBlendedData.colorTex1" };

				std::unique_ptr<GLint>  parameters(new GLint[3]);
				std::unique_ptr<GLuint> uniformIndices(new GLuint[3]);

				glGetUniformIndices(shaderPrograms["weightBlendedFinal"]->getProgram(), 3, uniformNames, uniformIndices.get());
				glGetActiveUniformsiv(shaderPrograms["weightBlendedFinal"]->getProgram(), 3, uniformIndices.get(), GL_UNIFORM_OFFSET, parameters.get());

				GLint* offset = parameters.get();
			}

			{
				shaderPrograms["weightBlendedFinal"]->enable();

				glVertexAttribFormat(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, 0);

				glVertexAttribBinding(VERTEX_POS, 0);
				glEnableVertexAttribArray(VERTEX_POS);

				glBindVertexBuffer(0, fullScreenRectangle.vboFullScreen, 0, sizeof(nv::vec3f));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				shaderPrograms["weightBlendedFinal"]->disable();
			}
			CHECK_GL_ERROR();
		}
	}
}

void TopazSample::renderTokenListWeightedBlendedOIT()
{

}

NvAppBase* NvAppFactory(NvPlatformContext* platform)
{
	return new TopazSample(platform);
}

