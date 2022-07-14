#include "Main.h"
#include "Canvas.h"

wxBEGIN_EVENT_TABLE(Main, wxFrame)
	EVT_MENU(20001, Main::OnMenuExit)
	EVT_BUTTON(10002, Main::OnGearPressed)
wxEND_EVENT_TABLE()

Main::Main() : wxFrame(nullptr, wxID_ANY, "ImplicitEngine", wxPoint(30, 30), wxSize(600, 600))
{
	// Menu setup
	menuBar = new wxMenuBar();

	fileMenu = new wxMenu();
	viewMenu = new wxMenu();

	fileMenu->Append(20001, "Exit\tAlt+F4");

	viewMenu->Append(30001, "Standard Output")->SetCheckable(true);
	viewMenu->Append(30002, "Prefiltering Seeds")->SetCheckable(true);
	viewMenu->Append(30003, "Prefiltering mesh")->SetCheckable(true);
	wxMenuItemList& menuItemList = viewMenu->GetMenuItems();
	menuItemList[0]->Check(true);

	menuBar->Append(fileMenu, "File");
	menuBar->Append(viewMenu, "View");

	SetMenuBar(menuBar);

	// Top bar
	topBar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(0, 35));
	detailLabel = new wxStaticText(topBar, wxID_ANY, "Plot Detail", wxPoint(10, 10), wxSize(75, 20));
	detailSpinner = new wxSpinCtrl(topBar, 10001, "", wxPoint(90, 5), wxSize(50, 25), wxALIGN_LEFT | wxSP_ARROW_KEYS, 0, 10, 5);

	detailGearButton = new wxButton(topBar, 10002, "", wxPoint(140, 5), wxSize(25, 25));
	detailGearButton->SetBitmap(wxImage(25, 25, gearCol, gearAlpha, true));
	detailGearButton->SetToolTip("Advanced render settings");

	// Main controls
	horizSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_BORDER | wxSP_LIVE_UPDATE);

	// Initialize editable list
	equationList = new wxEditableListBox(horizSplitter, wxID_ANY, "Equations");
	equationList->SetFocus();

	// Initialize canvas
	int attribs[] = { WX_GL_SAMPLE_BUFFERS, 1,
						WX_GL_SAMPLES, antialiasingSamples,
						WX_GL_CORE_PROFILE,
						WX_GL_RGBA,
						WX_GL_DOUBLEBUFFER,
						0 };

	int* attribList = attribs;
	if (!useAntialiasing) attribList += 4;
	canvas = new Canvas(horizSplitter, attribList);

	// Setup splitter
	horizSplitter->SplitVertically(equationList, canvas, 200);
	horizSplitter->SetMinimumPaneSize(100);

	// Add all controls to main panel
	vertSizer = new wxBoxSizer(wxVERTICAL);
	vertSizer->Add(topBar, 0, wxEXPAND);
	vertSizer->Add(horizSplitter, 1, wxEXPAND);

	SetSizer(vertSizer);
}

Main::~Main()
{

}

void Main::OnMenuExit(wxCommandEvent& evt)
{
	Destroy();
}

void Main::OnGearPressed(wxCommandEvent& evt)
{
	wxDialog* dialog = new wxDialog(this, wxID_ANY, "Advanced Render Settings", wxDefaultPosition, wxSize(225, 135));
	wxPanel* dialogPanel = new wxPanel(dialog);
	dialogPanel->SetFocus();

	wxStaticText* seedNumLabel = new wxStaticText(dialogPanel, wxID_ANY, "Prefiltering Seeds", wxPoint(10, 8));
	wxSpinCtrl* seedNumSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 5), wxSize(60, 25), wxALIGN_LEFT | wxSP_ARROW_KEYS, 512, 16000, 2048);

	wxStaticText* prefResLabel = new wxStaticText(dialogPanel, wxID_ANY, "Prefiltering Resolution", wxPoint(10, 38));
	wxSpinCtrl* prefResSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 35), wxSize(60, 25), wxALIGN_LEFT | wxSP_ARROW_KEYS, 2, 8, 5);

	wxStaticText* finalResLabel = new wxStaticText(dialogPanel, wxID_ANY, "Final Mesh Resolution", wxPoint(10, 68));
	wxSpinCtrl* finalResSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 65), wxSize(60, 25), wxALIGN_LEFT | wxSP_ARROW_KEYS, 3, 12, 9);

	dialog->ShowModal();
}