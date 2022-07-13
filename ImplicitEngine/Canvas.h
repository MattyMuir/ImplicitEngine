#pragma once
// wxWidgets includes
#include <wx/wx.h>
#include <wx/glcanvas.h>

//STL includes
#include <string>

// OpenGL includes
#include "glall.h"
#include "glhelpers.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "VertexArray.h"
#include "Shader.h"

// Custom header files
#include "FilteringRenderer.h"
#include "Timer.h"

class Main;

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent, int* attribs);
	~Canvas();
	Main* mainPtr;

protected:
	int w = 0, h = 0;

	wxGLContext* context;
	VertexBuffer* vb;
	VertexBufferLayout* vbl;
	VertexArray* va;
	Shader* shader;

	FilteringRenderer* renderer;

	void OnDraw();
	void OnPaint(wxPaintEvent& evt);
	void Resized(wxSizeEvent& evt);
	void OnIdle(wxIdleEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);

	wxDECLARE_EVENT_TABLE();
};