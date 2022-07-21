#pragma once
// wxWidgets includes
#include <wx/wx.h>
#include "glall.h"
#include "glhelpers.h"
#include <wx/glcanvas.h>
#include <wx/display.h>

//STL includes
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <atomic>
#include <format>

// OpenGL includes
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "VertexArray.h"
#include "Shader.h"
#include "TextRenderer.h"

// Custom header files
#include "FilteringRenderer.h"
#include "Timer.h"

class Main;

struct MousePosition
{
	double x = 0, y = 0;
	bool valid = false;
};

struct Point { float x, y; };

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent, const wxGLAttributes& attribs);
	~Canvas();
	Main* mainPtr;

	void JobProcessingFinished();

	void DisplaySeeds(bool display);
	void DisplayMeshes(bool display);

protected:
	int w = 0, h = 0;

	// OpenGL wrappers
	wxGLContext* context;
	VertexBuffer* vb;
	VertexBufferLayout* vbl;
	VertexArray* va;
	Shader* shader;
	
	TextRenderer* textRenderer;

	// Renderer
	FilteringRenderer* renderer;
	bool displaySeeds = false;
	bool displayMeshes = false;

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

	void DrawGrid();
	void DrawAxes(float width);
	void DrawGridlines(double spacing, float opacity);
	std::pair<int, int> RoundMajorGridValue(double val);
	void DrawAxisText(std::pair<int, int> spacingSF);
	void DrawSeeds(const std::shared_ptr<Seeds>& seeds);
	void DrawMesh(const std::shared_ptr<Mesh>& mesh);
	void DrawContour(const std::vector<double>& verts);

	void RecalculateBounds();
	void UpdateJobs();

	friend Main;
	wxDECLARE_EVENT_TABLE();
};