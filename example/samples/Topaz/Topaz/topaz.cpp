#include "topaz.h"
#include <windows.h>

TopazSample::TopazSample(NvPlatformContext* platform) : NvSampleApp(platform, "Topaz Sample")
{
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

	compileShaders();

	loadModel("models/background.obj");
	
	loadModel("models/way1.obj");
	loadModel("models/way2.obj");

	textures.skybox = NvImage::UploadTextureFromDDSFile("textures/sky_cube.dds");

	cmdlist.state.programIncarnation++;

	CHECK_GL_ERROR();
}

void TopazSample::reshape(int32_t width, int32_t height)
{
	initFramebuffers(width, height);

	initScene();
	initCommandList();

	CHECK_GL_ERROR();
}

void TopazSample::draw()
{
	nv::matrix4f projection = nv::perspective(projection, 45.f * NV_PI / 180.f, m_width / float(m_height), 0.1f, 10.f);
	sceneData.modelViewProjection = projection * m_transformer->getModelViewMat();

	glNamedBufferSubDataEXT(ubos.sceneUbo, 0, sizeof(SceneData), &sceneData);

	// TODO : change this
	objectData.objectID = nv::vec4f(0.0);
	objectData.objectColor = nv::vec4f(1.0f);
	objectData.skybox = texturesAddress64.skybox;

	for (auto & model : models)
	{
		glNamedBufferSubDataEXT(model->getUBO(), 0, sizeof(ObjectData), &objectData);
		objectData.objectID = nv::vec4f(1.0);
	}
	//

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

	glViewport(0, 0, m_width, m_height);

	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glClearDepthf(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (cmdlist.state != cmdlist.captured)
	{
		updateCommandListState();
	}

	glCallCommandListNV(cmdlist.tokenCmdList);
	// drawStandard();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos.scene);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void TopazSample::compileShaders()
{
	NvGLSLProgram::setGlobalShaderHeader("#version 440\n");

	shaderPrograms.push_back(std::unique_ptr<NvGLSLProgram>(
		NvGLSLProgram::createFromFiles("shaders/vertex.glsl", "shaders/fragment.glsl")));

	NvGLSLProgram::setGlobalShaderHeader(nullptr);
}

void TopazSample::loadModel(std::string filename)
{
	int32_t length;
	char* data = NvAssetLoaderRead(filename.c_str(), length);

	std::unique_ptr<TopazGLModel> model(new TopazGLModel());
	model->loadModelFromObjData(data);
	model->rescaleModel(1.0f);

	models.push_back(std::move(model));

	NvAssetLoaderFree(data);
	CHECK_GL_ERROR();
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

	// initializate scene ubo
	{
		if (ubos.sceneUbo)
		{
			glDeleteBuffers(1, &ubos.sceneUbo);
		}
		glGenBuffers(1, &ubos.sceneUbo);

		glBindBuffer(GL_UNIFORM_BUFFER, ubos.sceneUbo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneData), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		if (bindlessVboUbo)
		{
			glGetNamedBufferParameterui64vNV(ubos.sceneUbo, GL_BUFFER_GPU_ADDRESS_NV, &ubos.sceneUbo64);
			glMakeNamedBufferResidentNV(ubos.sceneUbo, GL_READ_ONLY);
		}
	}

	for (auto & model : models)
	{
		model->getModel()->compileModel(NvModelPrimType::TRIANGLES);

		/* ibo */
		if (model->getIBO())
		{
			glDeleteBuffers(1, &model->getIBO());
		}
		glGenBuffers(1, &model->getIBO());
		glNamedBufferStorageEXT(model->getIBO(), model->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES) * sizeof(uint32_t),
			model->getModel()->getCompiledIndices(NvModelPrimType::TRIANGLES), 0);

		/* vbo */
		if (model->getVBO())
		{
			glDeleteBuffers(1, &model->getVBO());
		}
		glGenBuffers(1, &model->getVBO());
		glNamedBufferStorageEXT(model->getVBO(), model->getModel()->getCompiledVertexCount() * model->getModel()->getCompiledVertexSize() * sizeof(float), model->getModel()->getCompiledVertices(), 0);

		if (bindlessVboUbo)
		{
			glGetNamedBufferParameterui64vNV(model->getIBO(), GL_BUFFER_GPU_ADDRESS_NV, &model->getIBO64());
			glGetNamedBufferParameterui64vNV(model->getVBO(), GL_BUFFER_GPU_ADDRESS_NV, &model->getVBO64());
			glMakeNamedBufferResidentNV(model->getIBO(), GL_READ_ONLY);
			glMakeNamedBufferResidentNV(model->getVBO(), GL_READ_ONLY);
		}

		/* ubo */
		if (model->getUBO())
		{
			glDeleteBuffers(1, &model->getUBO());
		}
		glGenBuffers(1, &model->getUBO());

		glBindBuffer(GL_UNIFORM_BUFFER, model->getUBO());
		glBufferData(GL_UNIFORM_BUFFER, sizeof(ObjectData), nullptr, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		if (bindlessVboUbo)
		{
			glGetNamedBufferParameterui64vNV(model->getUBO(), GL_BUFFER_GPU_ADDRESS_NV, &model->getUBO64());
			glMakeNamedBufferResidentNV(model->getUBO(), GL_READ_ONLY);
		}
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

		glGenBuffers(1, &cmdlist.tokenBuffer);
		glCreateCommandListsNV(1, &cmdlist.tokenCmdList);
	}

	NVTokenSequence& seq = cmdlist.tokenSequence;
	std::string& stream = cmdlist.tokenData;
	{
		NVTokenUbo  ubo;
		ubo.setBuffer(ubos.sceneUbo, ubos.sceneUbo64, 0, sizeof(SceneData));
		ubo.setBinding(UBO_SCENE, NVTOKEN_STAGE_VERTEX);
		nvtokenEnqueue(stream, ubo);
		ubo.setBinding(UBO_SCENE, NVTOKEN_STAGE_FRAGMENT);
		nvtokenEnqueue(stream, ubo);
	}

	size_t offset = 0;
	for (auto & model : models)
	{
		NVTokenVbo vbo;
		vbo.setBinding(0);
		vbo.setBuffer(model->getVBO(), model->getVBO64(), 0);
		nvtokenEnqueue(stream, vbo);

		NVTokenIbo ibo;
		ibo.setType(GL_UNSIGNED_INT);
		ibo.setBuffer(model->getIBO(), model->getIBO64());
		nvtokenEnqueue(stream, ibo);

		NVTokenUbo ubo;
		ubo.setBuffer(model->getUBO(), model->getUBO64(), 0, sizeof(ObjectData));
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

			auto & currentProgram = shaderPrograms.at(0);

			glUseProgram(currentProgram->getProgram());

			if (hwsupport)
			{
				glStateCaptureNV(cmdlist.stateObjectDraw, GL_TRIANGLES);
			}

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

	glBindTexture(GL_TEXTURE_2D, textures.sceneColor);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);

	if (textures.sceneDepthStencil)
	{
		glDeleteTextures(1, &textures.sceneDepthStencil);
	}
	glGenTextures(1, &textures.sceneDepthStencil);

	glBindTexture(GL_TEXTURE_2D, textures.sceneDepthStencil);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, width, height);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (fbos.scene)
	{
		glDeleteFramebuffers(1, &fbos.scene);
	}
	glGenFramebuffers(1, &fbos.scene);

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures.sceneColor, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, textures.sceneDepthStencil, 0);
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
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, ubos.sceneUbo);

	for (auto & model : models)
	{
		glVertexAttribFormat(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, model->getModel()->getCompiledPositionOffset());

		glVertexAttribBinding(VERTEX_POS, 0);
		glEnableVertexAttribArray(VERTEX_POS);

		glUseProgram(shaderPrograms.at(0)->getProgram());

		glBindBufferRange(GL_UNIFORM_BUFFER, UBO_OBJECT, model->getUBO(), 0, sizeof(ObjectData));

		glBindVertexBuffer(0, model->getVBO(), 0, model->getModel()->getCompiledVertexSize() * sizeof(float));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->getIBO());
		glDrawElements(GL_TRIANGLES, model->getModel()->getCompiledIndexCount(NvModelPrimType::TRIANGLES), GL_UNSIGNED_INT, nullptr);

		glDisableVertexAttribArray(VERTEX_POS);
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_OBJECT, 0);
	glBindVertexBuffer(0, 0, 0, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

NvAppBase* NvAppFactory(NvPlatformContext* platform)
{
	return new TopazSample(platform);
}

