#include "TextRenderer.h"

TextRenderer::TextRenderer()
{
	if (FT_Init_FreeType(&ft))
		std::cerr << "Failed to initialize FreeType!\n";

    textVB = new VertexBuffer;
    textVA = new VertexArray;
    textVBL = new VertexBufferLayout;

    textVBL->Push<float>(4);
    textVA->AddVBuffer(*textVB, *textVBL);

    textShader = Shader::CompileString(
#include "textshader"
    );

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

TextRenderer::~TextRenderer()
{
	FT_Done_FreeType(ft);

    delete textVB;
    delete textVA;
    delete textVBL;
    delete textShader;
}

void TextRenderer::LoadFont(std::string_view path, std::string_view name, unsigned int size)
{
	FT_Face face;

    // Load font into face
    if (FT_New_Face(ft, path.data(), 0, &face))
    {
        std::cerr << "Failed to load font: " << path << '\n';
        return;
    }

    // Add font to fonts map
    auto& font = fonts[{ std::string(name), size }];

	// Set font size
	FT_Set_Pixel_Sizes(face, 0, size);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load all characters
    for (unsigned char c = 0; c <= 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }

        // Generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Now store character for later use
        Character character = {
            texture,
            face->glyph->bitmap.width, face->glyph->bitmap.rows,
            face->glyph->bitmap_left, face->glyph->bitmap_top,
            face->glyph->advance.x
        };

        font.chars.insert(std::pair<char, Character>(c, character));
    }

    // Reset texture bind
    glBindTexture(GL_TEXTURE_2D, 0);

    // Free face obj
	FT_Done_Face(face);
}

void TextRenderer::RenderText(std::string_view text, FontMeta font_, int screenW, int screenH, float x, float y, float scale)
{
    // Bind shader
    textShader->Bind();

    glUniform3f(textShader->GetUniformLocation("textColor"), 0.0f, 0.0f, 0.0f);
    glActiveTexture(GL_TEXTURE0);
    textVA->Bind();

    if (!fonts.contains(font_))
        LoadFont("C:\\Windows\\Fonts\\" + font_.name + ".ttf", font_.name, font_.size);

    if (!fonts.contains(font_))
        return;

    Font& font = fonts[font_];

    // Convert x and y into OpenGL coordinates
    x = (x / screenW) * 2 - 1;
    y = (y / screenH) * 2 - 1;

    // iterate through all characters
    for (char c : text)
    {
        Character ch = font.chars[c];

        float xpos = x + (ch.bx * scale) / screenW;
        float ypos = y - ((ch.h - ch.by) * scale) / screenH;

        float w = ch.w * scale / screenW;
        float h = ch.h * scale / screenH;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.texID);
        // Update contents of VB
        textVB->SetData(vertices, sizeof(vertices));

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Advance cursors for next glyph
        x += (ch.advance >> 6) * scale / screenW;
    }
    textVA->Unbind();
    glBindTexture(GL_TEXTURE_2D, 0);
}

size_t FontMetaHasher::operator()(const FontMeta& obj) const
{
    size_t h1 = std::hash<std::string>{}(obj.name);
    size_t h2 = std::hash<unsigned int>{}(obj.size);
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

bool FontMeta::operator==(const FontMeta& other) const
{
    return name == other.name && size == other.size;
}
