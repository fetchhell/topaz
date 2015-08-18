#include "NV/NvLogs.h"
#include "TopazGLModel.h"
#include "NvModel/NvModel.h"

#define OFFSET(n) ((char *)NULL + (n))

TopazGLModel::TopazGLModel()
:model_vboID(0), model_iboID(0), model_uboID(0), model_vboID64(0), model_iboID64(0), model_uboID64(0)
{
    // glGenBuffers(1, &model_vboID);
    // glGenBuffers(1, &model_iboID);
    model = NvModel::Create();
}

TopazGLModel::TopazGLModel(NvModel *pModel) : model_vboID(0), model_iboID(0),
model_uboID(0), model_vboID64(0), model_iboID64(0), model_uboID64(0), model(pModel)
{
	// glGenBuffers(1, &model_vboID);
	// glGenBuffers(1, &model_iboID);
	model = pModel;
}

TopazGLModel::~TopazGLModel()
{
    // glDeleteBuffers(1, &model_vboID);
    // glDeleteBuffers(1, &model_iboID);
    delete model;
}

void TopazGLModel::loadModelFromObjData(char *fileData)
{
    bool res = model->loadModelFromFileDataObj(fileData);
    if (!res) {
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

    glBindBuffer(GL_ARRAY_BUFFER, model_vboID);
    glBufferData(GL_ARRAY_BUFFER, model->getCompiledVertexCount() * model->getCompiledVertexSize() * sizeof(float), model->getCompiledVertices(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model_iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->getCompiledIndexCount(NvModelPrimType::TRIANGLES) * sizeof(uint32_t), model->getCompiledIndices(NvModelPrimType::TRIANGLES), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void TopazGLModel::bindBuffers()
{
    glBindBuffer(GL_ARRAY_BUFFER, model_vboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model_iboID);
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
