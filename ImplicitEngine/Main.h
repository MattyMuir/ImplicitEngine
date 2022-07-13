#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include <wx/editlbox.h>

#include "gearIcon.h"

class Canvas;

class Main : public wxFrame
{
public:
	Main();
	~Main();

protected:

	void OnMenuExit(wxCommandEvent& evt);
	void OnGearPressed(wxCommandEvent& evt);

	// Menu
	wxMenuBar* menuBar = nullptr;
	wxMenu* fileMenu = nullptr;
	wxMenu* viewMenu = nullptr;

	// === UI controls ===
	// Containers
	wxBoxSizer* vertSizer = nullptr;
	wxSplitterWindow* horizSplitter = nullptr;
	wxPanel* topBar = nullptr;

	// Top controls
	wxStaticText* detailLabel = nullptr;
	wxSpinCtrl* detailSpinner = nullptr;
	wxButton* detailGearButton = nullptr;

	// Main controls
	wxEditableListBox* equationList = nullptr;

	// Canvas
	bool useAntialiasing = true;
	int antialiasingSamples = 4;
	Canvas* canvas;

	wxDECLARE_EVENT_TABLE();
};