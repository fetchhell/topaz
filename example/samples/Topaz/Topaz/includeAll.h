#pragma once

#include "NvAppBase/NvSampleApp.h"
#include "NvAppBase/NvInputTransformer.h"

#include "NvAssetLoader/NvAssetLoader.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "NvGLUtils/NvTimers.h"
#include "NvGLUtils/NvImage.h"
#include "TopazGLModel.h"
#include "NvModel/NvModel.h"
#include "NvModel/NvShapes.h"
#include "NV/NvLogs.h"
#include "NV/NvMath.h"

#include "NvUI/NvTweakBar.h"

#include <vector>
#include <string>
#include <map> 

#include <memory>

#include "nvtoken.hpp"

#define VERTEX_POS    0
#define VERTEX_NORMAL 1
#define VERTEX_UV     2

#define UBO_SCENE     0
#define UBO_OBJECT    1
#define UBO_OIT		  2
#define UBO_IDENTITY  3