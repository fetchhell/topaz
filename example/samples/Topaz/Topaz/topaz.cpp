#include "topaz.h"
#include <windows.h>

TopazSample::TopazSample(NvPlatformContext* platform) : NvSampleApp(platform, "Topaz Sample"), drawMode(DRAW_TOKEN_LIST)
{
	oit = std::unique_ptr<WeightedBlendedOIT>(new WeightedBlendedOIT);

	sceneBackgroundColor = nv::vec4f(0.2f, 0.2f, 0.2f, 0.0f);
	m_transformer->setTranslationVec(nv::vec3f(0.f, 0.f, -2.5f));
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
			{ "weight blended standard", DRAW_WEIGHT_BLENDED_STANDARD }
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
	compileShaders("weightBlendedFinal", "shaders/vertex.glsl", "shaders/fragmentFinalOIT.glsl");

	loadModel("models/background.obj", shaderPrograms["draw"]->getProgram());
	loadModel("models/way3.obj", shaderPrograms["draw"]->getProgram(), true);
	loadModel("models/way4.obj", shaderPrograms["draw"]->getProgram(), true);

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

	CHECK_GL_ERROR();
}

void TopazSample::draw()
{
	nv::matrix4f projection = nv::perspective(projection, 45.f * NV_PI / 180.f, m_width / float(m_height), 0.1f, 10.f);
	sceneData.modelViewProjection = projection * m_transformer->getModelViewMat();
	sceneData.depthScale = oit->getWeightParameter();

	glNamedBufferSubDataEXT(ubos.sceneUbo, 0, sizeof(SceneData), &sceneData);

	glLineWidth(10.0f);

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

	// initializate scene & weightBlended ubo
	{
		initBuffer(GL_UNIFORM_BUFFER, ubos.sceneUbo, ubos.sceneUbo64, 
			sizeof(SceneData), 
			nullptr, 
			true); // mutable buffer

		WeightBlendedData weightBlendedData;
		weightBlendedData.background = texturesAddress64.sceneColor; // oit->getOpaqueId64(); 
		weightBlendedData.colorTex0 = oit->getAccumulationTextureId64(0);
		weightBlendedData.colorTex1 = oit->getAccumulationTextureId64(1);

		initBuffer(GL_UNIFORM_BUFFER, ubos.weightBlendedUbo, ubos.weightBlendedUbo64,
			sizeof(WeightBlendedData),
			&weightBlendedData,
			true); // mutable buffer
	}

	objectData.objectID = nv::vec4f(0.0);
	objectData.objectColor = nv::vec4f(1.0f, 1.0f, 1.0f, oit->getOpacity());
	objectData.skybox = texturesAddress64.skybox;

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

void TopazSample::initCommandList()
{
	hwsupport = init_NV_command_list(sysGetProcAddress) ? true : false;

	nvtokenInitInternals(hwsupport, bindlessVboUbo);
	
	if (hwsupport)
	{
		glCreateStatesNV(1, &cmdlist.stateObjectDraw);
	    glCreateStatesNV(1, &cmdlist.stateObjectLinesDraw);

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
		NVTokenVbo vbo;
		vbo.setBinding(0);
		vbo.setBuffer(model->getBufferID("vbo"), model->getBufferID64("vbo"), 0);
		nvtokenEnqueue(stream, vbo);

		NVTokenIbo ibo;
		ibo.setType(GL_UNSIGNED_INT);
		ibo.setBuffer(model->getBufferID("ibo"), model->getBufferID64("ibo"));
		nvtokenEnqueue(stream, ibo);

		NVTokenUbo ubo;
		ubo.setBuffer(model->getBufferID("ubo"), model->getBufferID64("ubo"), 0, sizeof(ObjectData));
		ubo.setBinding(UBO_OBJECT, NVTOKEN_STAGE_VERTEX);
		nvtokenEnqueue(stream, ubo);
		ubo.setBinding(UBO_OBJECT, NVTOKEN_STAGE_FRAGMENT);
		nvtokenEnqueue(stream, ubo);

		NVTokenDrawElems  draw;
		draw.setParams(model->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES));
		draw.setMode(GL_TRIANGLES);
		nvtokenEnqueue(stream, draw);
	}
	
	seq.offsets.push_back(offset);
	seq.sizes.push_back(GLsizei(stream.size() - offset));
	seq.fbos.push_back(fbos.scene);
	seq.states.push_back(cmdlist.stateObjectDraw); 
	
 	offset = stream.size();
	
	for (auto & model : models)
	{
		if (model->cornerPointsExists())
		{
			NVTokenVbo vboCorner;
			vboCorner.setBinding(0);
			vboCorner.setBuffer(model->getCornerBufferID("vbo"), model->getCornerBufferID64("vbo"), 0);
			nvtokenEnqueue(stream, vboCorner);

			NVTokenIbo iboCorner;
			iboCorner.setType(GL_UNSIGNED_INT);
			iboCorner.setBuffer(model->getCornerBufferID("ibo"), model->getCornerBufferID64("ibo"));
			nvtokenEnqueue(stream, iboCorner);

			NVTokenUbo uboCorner;
			uboCorner.setBuffer(model->getCornerBufferID("ubo"), model->getCornerBufferID64("ubo"), 0, sizeof(ObjectData));
			uboCorner.setBinding(UBO_OBJECT, NVTOKEN_STAGE_VERTEX);
			nvtokenEnqueue(stream, uboCorner);
			uboCorner.setBinding(UBO_OBJECT, NVTOKEN_STAGE_FRAGMENT);
			nvtokenEnqueue(stream, uboCorner);

			NVTokenDrawElems drawCorner;
			drawCorner.setParams(model->getCornerIndices().size());
			drawCorner.setMode(GL_LINE_STRIP);
			nvtokenEnqueue(stream, drawCorner);
		}
	}
	
	seq.offsets.push_back(offset);
	seq.sizes.push_back(GLsizei(stream.size() - offset));
	seq.fbos.push_back(fbos.scene);
	seq.states.push_back(cmdlist.stateObjectLinesDraw);

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

void TopazSample::updateCommandListState()
{
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
			glStateCaptureNV(cmdlist.stateObjectDraw, GL_TRIANGLES);

			glBindVertexBuffer(0, 0, 0, sizeof(nv::vec3f));

			glStateCaptureNV(cmdlist.stateObjectLinesDraw, GL_LINES);

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
		glMakeTextureHandleNonResidentARB(texturesAddress64.sceneDepthStencil);
	}

	if (textures.sceneColor)
	{
		glDeleteTextures(1, &textures.sceneColor);
	}
	glGenTextures(1, &textures.sceneColor);

	glBindTexture(GL_TEXTURE_RECTANGLE, textures.sceneColor);
	glTexStorage2D(GL_TEXTURE_RECTANGLE, 1, GL_RGBA8, width, height);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	if (textures.sceneDepthStencil)
	{
		glDeleteTextures(1, &textures.sceneDepthStencil);
	}
	glGenTextures(1, &textures.sceneDepthStencil);

	glBindTexture(GL_TEXTURE_RECTANGLE, textures.sceneDepthStencil);
	glTexStorage2D(GL_TEXTURE_RECTANGLE, 1, GL_DEPTH24_STENCIL8, width, height);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	if (fbos.scene)
	{
		glDeleteFramebuffers(1, &fbos.scene);
	}
	glGenFramebuffers(1, &fbos.scene);

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, textures.sceneColor, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_RECTANGLE, textures.sceneDepthStencil, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (GLEW_ARB_bindless_texture)
	{
		texturesAddress64.sceneColor = glGetTextureHandleARB(textures.sceneColor);
		texturesAddress64.sceneDepthStencil = glGetTextureHandleARB(textures.sceneDepthStencil);
		glMakeTextureHandleResidentARB(texturesAddress64.sceneColor);
		glMakeTextureHandleResidentARB(texturesAddress64.sceneDepthStencil);
	}

	cmdlist.state.fboIncarnation++;
}

void TopazSample::drawStandard()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1, 1);

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, ubos.sceneUbo);

	for (auto & model : models)
	{
		drawModel(GL_TRIANGLES, *shaderPrograms["draw"], *model);
	}
}

void TopazSample::drawModel(GLenum mode, NvGLSLProgram& program, TopazGLModel& model)
{
	glVertexAttribFormat(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, model.getModel()->getCompiledPositionOffset());

	glVertexAttribBinding(VERTEX_POS, 0);
	glEnableVertexAttribArray(VERTEX_POS);

	program.enable();

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

	glLineWidth(2.0f);

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, ubos.sceneUbo);

	/* draw opaque models (GL_TEXTURE_2D) -> change to (GL_TEXTURE_RECTANGLE) */
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

			objectData.objectColor = nv::vec4f(1.0f, 1.0f, 1.0f, oit->getOpacity());
			glNamedBufferSubDataEXT((*model)->getBufferID("ubo"), 0, sizeof(ObjectData), &objectData);

			drawModel(GL_TRIANGLES, *shaderPrograms["weightBlendedFinal"], **model);

			CHECK_GL_ERROR();
		}
	}
}

NvAppBase* NvAppFactory(NvPlatformContext* platform)
{
	return new TopazSample(platform);
}

