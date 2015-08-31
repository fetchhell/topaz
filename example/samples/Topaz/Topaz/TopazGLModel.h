
#ifndef TOPAZGLMODEL_H_
#define TOPAZGLMODEL_H_

#include <NvFoundation.h>
#include "NV/NvPlatformGL.h"
#include "NV/NvMath.h"

#include <vector>
#include <map>

class NvModel;

class TopazGLModel
{
public:
    TopazGLModel();
    ~TopazGLModel();

	TopazGLModel(NvModel *pModel);

    void loadModelFromObjData(char *fileData);
    void rescaleModel(float radius);

	void setProgram(GLuint program);

    void initBuffers(bool computeTangents = false, bool computeNormals = true);

    void drawElements(GLint positionHandle);
    void drawElements(GLint positionHandle, GLint normalHandle);
    void drawElements(GLint positionHandle, GLint normalHandle, GLint texcoordHandle);
    void drawElements(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle);

    NvModel *getModel();

    void computeCenter();
	void calculateCornerPoints(float radius);

    nv::vec3f m_center; 

    nv::vec3f GetMinExt()
    {
        return m_minExtent;
    }

    nv::vec3f GetMaxExt()
    {
        return m_maxExtent;
    }

	GLuint getProgram()
	{
		return model_program;
	}

	GLuint & getBufferID(std::string name)
	{
		return modelBuffers[name];
	}

	GLuint64 & getBufferID64(std::string name)
	{
		return modelBuffers64[name];
	}

	GLuint & getCornerBufferID(std::string name)
	{
		return modelCornerBuffers[name];
	}

	GLuint64 & getCornerBufferID64(std::string name)
	{
		return modelCornerBuffers64[name];
	}
	
	bool cornerPointsExists()
	{
		return cornerPointsExisting;
	}

	std::vector<nv::vec3f> & getCorners()
	{
		return corners;
	}

	std::vector<GLuint> & getCornerIndices()
	{
		return cornerIndices;
	}

    void bindBuffers();
    void unbindBuffers();

private:

	NvModel* model;

	std::map<std::string, GLuint> modelBuffers;
	std::map<std::string, GLuint64> modelBuffers64;

	std::map<std::string, GLuint> modelCornerBuffers;
	std::map<std::string, GLuint64> modelCornerBuffers64;

	GLuint model_program;

    nv::vec3f m_minExtent, m_maxExtent, m_radius;
	
	std::vector<nv::vec3f> corners;
	std::vector<GLuint> cornerIndices;

	bool cornerPointsExisting;
};

#endif 
