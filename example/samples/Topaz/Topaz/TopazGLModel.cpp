#include "NV/NvLogs.h"
#include "TopazGLModel.h"
#include "NvModel/NvModel.h"

#define OFFSET(n) ((char *)NULL + (n))

TopazGLModel::TopazGLModel() : cornerPointsExisting(false)
{
    model = NvModel::Create();
}

TopazGLModel::TopazGLModel(NvModel *pModel) :  model(pModel), cornerPointsExisting(false)
{
	model = pModel;
}

TopazGLModel::~TopazGLModel()
{
    delete model;
}

void TopazGLModel::setProgram(GLuint program)
{
	this->model_program = program;
}

void TopazGLModel::calculateCornerPoints(float radius)
{
	nv::vec3f minValue = nv::vec3f(1e10f, 1e10f, 1e10f);
	nv::vec3f maxValue = -minValue;

	model->computeBoundingBox(m_minExtent, m_maxExtent);

	nv::vec3f leftTop, rightTop, leftBottom, rightBottom;
	
	leftTop     = nv::vec3f(m_minExtent.x, m_minExtent.y, m_maxExtent.z);
	rightTop    = nv::vec3f(m_maxExtent.x, m_minExtent.y, m_maxExtent.z);
	leftBottom  = nv::vec3f(m_minExtent.x, m_minExtent.y, m_minExtent.z);
	rightBottom = nv::vec3f(m_maxExtent.x, m_minExtent.y, m_minExtent.z);

	nv::vec3f diff =  0.5f * (m_maxExtent - m_minExtent);
	nv::vec3f center =  m_minExtent + diff;
	center = nv::vec3f(center.x, 0.1, center.z);

	float oldRadius = std::max(diff.x, std::max(diff.y, diff.z));
	float scale = 5.0f; // radius / oldRadius;

	this->corners.push_back(nv::vec3f(scale * (rightBottom - center)));
	this->corners.push_back(nv::vec3f(scale * (rightTop - center)));

	this->corners.push_back(nv::vec3f(scale * (leftTop - center)));
	this->corners.push_back(nv::vec3f(scale * (leftBottom - center)));

	for (size_t i = 0; i < 4; i++)
	{
		cornerIndices.push_back(i);
	}
	cornerIndices.push_back(0);

	this->cornerPointsExisting = true;
}

void TopazGLModel::loadModelFromObjData(char *fileData)
{
    bool res = model->loadModelFromFileDataObj(fileData);
    if (!res) 
	{
        LOGI("Model Loading Failed !");
        return;
    }
    computeCenter();
}

void TopazGLModel::computeCenter()
{
    model->computeBoundingBox(m_minExtent, m_maxExtent);
    m_radius = (m_maxExtent - m_minExtent) / 2.0f;
    m_center = m_minExtent + m_radius;
}

void TopazGLModel::rescaleModel(float radius)
{
    //model->rescale(radius);
    model->rescaleToOrigin(radius);
}

void TopazGLModel::initBuffers(bool computeTangents, bool computeNormals)
{
	if(computeNormals)
	{
		model->computeNormals();
	}
    
    if (computeNormals || computeTangents)
    {
        model->computeTangents();
    }
    
    model->compileModel(NvModelPrimType::TRIANGLES);

    //print the number of vertices...
    //LOGI("Model Loaded - %d vertices\n", model->getCompiledVertexCount());

    glBindBuffer(GL_ARRAY_BUFFER, getBufferID("vbo"));
    glBufferData(GL_ARRAY_BUFFER, model->getCompiledVertexCount() * model->getCompiledVertexSize() * sizeof(float), model->getCompiledVertices(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getBufferID("ibo"));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->getCompiledIndexCount(NvModelPrimType::TRIANGLES) * sizeof(uint32_t), model->getCompiledIndices(NvModelPrimType::TRIANGLES), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void TopazGLModel::bindBuffers()
{
    glBindBuffer(GL_ARRAY_BUFFER, getBufferID("vbo"));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getBufferID("ibo"));
}

void TopazGLModel::unbindBuffers()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TopazGLModel::drawElements(GLint positionHandle)
{
    bindBuffers();
    glVertexAttribPointer(positionHandle, model->getPositionSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), 0);
    glEnableVertexAttribArray(positionHandle);
        glDrawElements(GL_TRIANGLES, model->getCompiledIndexCount(NvModelPrimType::TRIANGLES), GL_UNSIGNED_INT, 0);
    glDisableVertexAttribArray(positionHandle);
    unbindBuffers();
}

void TopazGLModel::drawElements(GLint positionHandle, GLint normalHandle)
{
    bindBuffers();
    glVertexAttribPointer(positionHandle, model->getPositionSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), 0);
    glEnableVertexAttribArray(positionHandle);
    
    if (normalHandle >= 0) {
        glVertexAttribPointer(normalHandle, model->getNormalSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), OFFSET(model->getCompiledNormalOffset()*sizeof(float)));
        glEnableVertexAttribArray(normalHandle);
    }
    
    glDrawElements(GL_TRIANGLES, model->getCompiledIndexCount(NvModelPrimType::TRIANGLES), GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(positionHandle);
    if (normalHandle >= 0)
        glDisableVertexAttribArray(normalHandle);
    unbindBuffers();
}

void TopazGLModel::drawElements(GLint positionHandle, GLint normalHandle, GLint texcoordHandle)
{
    bindBuffers();
    glVertexAttribPointer(positionHandle, model->getPositionSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), 0);
    glEnableVertexAttribArray(positionHandle);

    if (normalHandle >= 0) {
        glVertexAttribPointer(normalHandle, model->getNormalSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), OFFSET(model->getCompiledNormalOffset()*sizeof(float)));
        glEnableVertexAttribArray(normalHandle);
    }

    if (texcoordHandle >= 0) {
        glVertexAttribPointer(texcoordHandle, model->getTexCoordSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), OFFSET(model->getCompiledTexCoordOffset()*sizeof(float)));
        glEnableVertexAttribArray(texcoordHandle);
    }
    
    glDrawElements(GL_TRIANGLES, model->getCompiledIndexCount(NvModelPrimType::TRIANGLES), GL_UNSIGNED_INT, 0);
    
    glDisableVertexAttribArray(positionHandle);
    if (normalHandle >= 0)
        glDisableVertexAttribArray(normalHandle);
    if (texcoordHandle >= 0)
        glDisableVertexAttribArray(texcoordHandle);
    unbindBuffers();
}

void TopazGLModel::drawElements(GLint positionHandle, GLint normalHandle, GLint texcoordHandle, GLint tangentHandle)
{
    bindBuffers();
    bindBuffers();
    glVertexAttribPointer(positionHandle, model->getPositionSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), 0);
    glEnableVertexAttribArray(positionHandle);

    if (normalHandle >= 0) {
        glVertexAttribPointer(normalHandle, model->getNormalSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), OFFSET(model->getCompiledNormalOffset()*sizeof(float)));
        glEnableVertexAttribArray(normalHandle);
    }

    if (texcoordHandle >= 0) {
        glVertexAttribPointer(texcoordHandle, model->getTexCoordSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), OFFSET(model->getCompiledTexCoordOffset()*sizeof(float)));
        glEnableVertexAttribArray(texcoordHandle);
    }

    if (tangentHandle >= 0) {
        glVertexAttribPointer(tangentHandle,  model->getTangentSize(), GL_FLOAT, GL_FALSE, model->getCompiledVertexSize() * sizeof(float), OFFSET(model->getCompiledTangentOffset()*sizeof(float)));
        glEnableVertexAttribArray(tangentHandle);
    }
    glDrawElements(GL_TRIANGLES, model->getCompiledIndexCount(NvModelPrimType::TRIANGLES), GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(positionHandle);
    if (normalHandle >= 0)
        glDisableVertexAttribArray(normalHandle);
    if (texcoordHandle >= 0)
        glDisableVertexAttribArray(texcoordHandle);
    if (tangentHandle >= 0)
        glDisableVertexAttribArray(tangentHandle);
    unbindBuffers();
}

NvModel * TopazGLModel::getModel()
{
    return model;
}
