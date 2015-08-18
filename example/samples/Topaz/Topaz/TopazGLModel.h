
#ifndef TOPAZGLMODEL_H_
#define TOPAZGLMODEL_H_

#include <NvFoundation.h>
#include "NV/NvPlatformGL.h"
#include "NV/NvMath.h"

class NvModel;

class TopazGLModel
{
public:
    TopazGLModel();
    ~TopazGLModel();

	TopazGLModel(NvModel *pModel);

    void loadModelFromObjData(char *fileData);
    void rescaleModel(float radius);

    void initBuffers(bool computeTangents = false, bool computeNormals = true);

    void drawElements(GLint positionHandle);
    void drawElements(GLint positionHandle, GLint normalHandle);
    void drawElements(GLint positionHandle, GLint normalHandle, GLint texcoordHandle);
    void drawElements(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle);

    NvModel *getModel();

    void computeCenter();

    nv::vec3f m_center; 

    nv::vec3f GetMinExt()
    {
        return m_minExtent;
    }

    nv::vec3f GetMaxExt()
    {
        return m_maxExtent;
    }

	GLuint & getVBO()
	{
		return model_vboID;
	}

	GLuint & getIBO()
	{
		return model_iboID;
	}

	GLuint & getUBO()
	{
		return model_uboID;
	}

	GLuint64 & getVBO64()
	{
		return model_vboID64;
	}

	GLuint64 & getIBO64()
	{
		return model_iboID64;
	}

	GLuint64 & getUBO64()
	{
		return model_uboID64;
	}

    void bindBuffers();
    void unbindBuffers();

private:

	NvModel *model;

    GLuint   model_vboID, model_iboID, model_uboID;
	GLuint64 model_vboID64, model_iboID64, model_uboID64;

    nv::vec3f m_minExtent, m_maxExtent, m_radius;
};

#endif 
