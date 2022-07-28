#pragma once
#pragma warning(push, 1)
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <wx/editlbox.h>
#pragma warning(pop)

#include <sstream>
#include <atomic>

#include "icons.h"

class Canvas;

class Main : public wxFrame
{
public:
	Main();
	~Main();

protected:

	void OnMenuExit(wxCommandEvent& evt);
	void OnGearPressed(wxCommandEvent& evt);
	void OnHomePressed(wxCommandEvent& evt);

	void OnDisplayStandardOutput(wxCommandEvent& evt);
	void OnDisplaySeeds(wxCommandEvent& evt);
	void OnDisplayMesh(wxCommandEvent& evt);

	void OnEquationEdit(wxListEvent& evt);
	void OnEquationDelete(wxListEvent& evt);

	std::string ProcessEquationString(std::string_view eqnStr);

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
	wxButton* homeBtn = nullptr;

	std::atomic<size_t> nextEqnID = 15000;

	// Canvas
	bool useAntialiasing = true;
	int antialiasingSamples = 4;
	Canvas* canvas;

	wxDECLARE_EVENT_TABLE();
};