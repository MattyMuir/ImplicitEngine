#pragma once
#include "wx/wx.h"

class Canvas;

class Main : public wxFrame
{
public:
	Main();
	~Main();

protected:
	Canvas* canvas;

	bool useAntialiasing = true;
	int antialiasingSamples = 4;

	wxDECLARE_EVENT_TABLE();
};