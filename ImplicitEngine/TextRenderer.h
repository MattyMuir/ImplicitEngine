#pragma once
#include "glall.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <unordered_map>

#include "Shader.h"
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "VertexBufferLayout.h"

struct Character
{
	unsigned int texID, w, h, bx, by, advance;
};

struct FontMeta
{
	bool operator==(const FontMeta& other) const;
	std::string name;
	unsigned int size;
};

struct Font
{
	std::unordered_map<char, Character> chars;
};

struct FontMetaHasher
{
	size_t operator()(const FontMeta& obj) const;
};

class TextRenderer
{
public:
	TextRenderer();
	~TextRenderer();

	void LoadFont(std::string_view path, std::string_view name, unsigned int size);

	void RenderText(std::string_view text, FontMeta font_, int screenW, int screenH, float x, float y, float scale);

protected:
	FT_Library ft;
	
	VertexBuffer* textVB;
	VertexArray* textVA;
	VertexBufferLayout* textVBL;
	Shader* textShader;

	std::unordered_map<FontMeta, Font, FontMetaHasher> fonts;
};