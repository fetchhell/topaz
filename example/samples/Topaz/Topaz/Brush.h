#pragma once

#include "includeAll.h"

namespace QtStyles
{
	enum BrushStyle
	{
		NoBrush,
		SolidPattern,
		Dense1Pattern,
		Dense2Pattern,
		Dense3Pattern,
		Dense4Pattern,
		Dense5Pattern,
		Dense6Pattern,
		Dense7Pattern,
		HorPattern,
		VerPattern,
		CrossPattern,
		BDiagPattern,
		FDiagPattern,
		DiagCrossPattern,
		LinearGradientPattern,
		RadialGradientPattern,
		ConicalGradientPattern,
		TexturePattern = 24
	};
}

class BrushStyles
{
public:
	BrushStyles();

	std::vector<unsigned char> brushPattern8to32(int style);

	GLuint& getTextureId()
	{
		return textureBrushStyleId;
	}

	GLuint64& getTextureId64()
	{
		return textureBrushStyleId64;
	}

private:
	const unsigned char* brushPattern8x8(int style);

	GLuint textureBrushStyleId;
	GLuint64 textureBrushStyleId64;
};