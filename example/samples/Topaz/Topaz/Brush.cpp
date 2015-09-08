#include "Brush.h"

BrushStyles::BrushStyles()
{
}

const unsigned char* BrushStyles::brushPattern8x8(int style)
{
	assert(style > QtStyles::SolidPattern && style < QtStyles::LinearGradientPattern);

	static const unsigned char dense1[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
	static const unsigned char dense2[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
	static const unsigned char dense3[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
	static const unsigned char dense4[] = { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 };
	static const unsigned char dense5[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
	static const unsigned char dense6[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
	static const unsigned char dense7[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
	static const unsigned char hor[] = { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 };
	static const unsigned char ver[] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
	static const unsigned char cross[] = { 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10, 0x10 };
	static const unsigned char bdiag[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
	static const unsigned char fdiag[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
	static const unsigned char dcross[] = { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 };

	static const unsigned char* const pattern[] =
	{
		dense1, dense2, dense3, dense4, dense5, dense6, dense7, 
		hor, ver, cross, bdiag, fdiag, dcross
	};

	return pattern[style - QtStyles::Dense1Pattern];
}

std::vector<unsigned char> BrushStyles::brushPattern8to32(int style)
{
	const unsigned char * pattern8 = brushPattern8x8(style);
	std::vector<unsigned char> pattern32(128, 0);
	unsigned char* data = reinterpret_cast< unsigned char* > (pattern32.data());

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			for (int k = 0; k < 4; ++k)
			{
				data[k * 32 + i * 4 + j] = pattern8[i];
			}
		}
	}

	glGenTextures(1, &textureBrushStyleId);

	glBindTexture(GL_TEXTURE_RECTANGLE, textureBrushStyleId);

	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8, pattern32.size() / 8, pattern32.size() / 8, 0, GL_RED, GL_UNSIGNED_BYTE, pattern32.data());

	if (GLEW_ARB_bindless_texture)
	{
		textureBrushStyleId64 = glGetTextureHandleARB(textureBrushStyleId);
		glMakeTextureHandleResidentARB(textureBrushStyleId64);
	}

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	CHECK_GL_ERROR();
	return pattern32;
}