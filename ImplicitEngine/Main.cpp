#include "Main.h"
#include "Canvas.h"

wxBEGIN_EVENT_TABLE(Main, wxFrame)
wxEND_EVENT_TABLE()

Main::Main() : wxFrame(nullptr, wxID_ANY, "Window Application", wxPoint(30, 30), wxSize(600, 600))
{
	int attribs[] = { WX_GL_SAMPLE_BUFFERS, 1,
						WX_GL_SAMPLES, antialiasingSamples,
						WX_GL_CORE_PROFILE,
						WX_GL_RGBA,
						WX_GL_DOUBLEBUFFER,
						0 };

	int* attribList = attribs;
	if (!useAntialiasing) attribList += 4;
	canvas = new Canvas(this, attribList);
}

Main::~Main()
{

}