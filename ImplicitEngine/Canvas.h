#pragma once
// wxWidgets includes
#include <wx/wx.h>
#include "glall.h"
#include "glhelpers.h"
#include <wx/glcanvas.h>

//STL includes
#include <string>
#include <vector>

// OpenGL includes
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "VertexArray.h"
#include "Shader.h"

// Custom header files
#include "FilteringRenderer.h"
#include "Timer.h"

class Main;

struct MousePosition
{
	double x, y;
	bool valid = false;
};

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent, int* attribs);
	~Canvas();
	Main* mainPtr;

	void JobProcessingFinished();

protected:
	int w = 0, h = 0;

	// OpenGL wrappers
	wxGLContext* context;
	VertexBuffer* vb;
	VertexBufferLayout* vbl;
	VertexArray* va;
	Shader* shader;

	// Renderer
	FilteringRenderer* renderer;

	// Coordinate system
	double relXScale = 300.0, relYScale = 300.0;
	double xOffset = 0.0, yOffset = 0.0;

	Bounds bounds;
	MousePosition lastMouse;

	void OnDraw();
	void OnPaint(wxPaintEvent& evt);
	void Resized(wxSizeEvent& evt);
	void OnScroll(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseDrag(double delX, double delY);
	void ToScreen(float& xout, float& yout, double x, double y);

	void RecalculateBounds();
	void UpdateJobs();

	friend Main;
	wxDECLARE_EVENT_TABLE();
};