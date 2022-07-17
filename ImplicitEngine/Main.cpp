#include "Main.h"
#include "Canvas.h"

wxBEGIN_EVENT_TABLE(Main, wxFrame)
	EVT_MENU(20001, Main::OnMenuExit)
	EVT_MENU(30001, Main::OnDisplayStandardOutput)
	EVT_MENU(30002, Main::OnDisplaySeeds)
	EVT_MENU(30003, Main::OnDisplayMesh)
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
	viewMenu->Append(30003, "Prefiltering Mesh")->SetCheckable(true);
	wxMenuItemList& menuItemList = viewMenu->GetMenuItems();
	menuItemList[0]->Check(true);

	menuBar->Append(fileMenu, "File");
	menuBar->Append(viewMenu, "View");

	SetMenuBar(menuBar);

	// Top bar
	topBar = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(0, 35));
	detailLabel = new wxStaticText(topBar, wxID_ANY, "Plot Detail", wxPoint(10, 10), wxSize(75, 20));
	detailSpinner = new wxSpinCtrl(topBar, 10001, "", wxPoint(90, 5), wxSize(50, 25), wxALIGN_LEFT | wxSP_ARROW_KEYS, 3, 12, 5);

	detailSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt) { canvas->renderer->SetFinalMeshRes(evt.GetValue()); });

	detailGearButton = new wxButton(topBar, 10002, "", wxPoint(140, 5), wxSize(25, 25));
	detailGearButton->SetBitmap(wxImage(25, 25, gearCol, gearAlpha, true));
	detailGearButton->SetToolTip("Advanced render settings");

	// Main controls
	horizSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_BORDER | wxSP_LIVE_UPDATE);

	// Initialize editable list
	equationList = new wxEditableListBox(horizSplitter, 5000, "Equations");
	equationList->GetListCtrl()->Bind(wxEVT_LIST_DELETE_ITEM, &Main::OnEquationDelete, this);
	equationList->SetFocus();
	equationList->GetListCtrl()->Bind(wxEVT_LIST_END_LABEL_EDIT, &Main::OnEquationEdit, this);

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

	detailSpinner->SetValue(canvas->renderer->GetFinalMeshRes());

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
	wxSpinCtrl* seedNumSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 5), wxSize(60, 25),
		wxALIGN_LEFT | wxSP_ARROW_KEYS, 256, 131072, canvas->renderer->GetSeedNum());

	seedNumSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt) { canvas->renderer->SetSeedNum(evt.GetValue()); });

	wxStaticText* prefResLabel = new wxStaticText(dialogPanel, wxID_ANY, "Prefiltering Resolution", wxPoint(10, 38));
	wxSpinCtrl* prefResSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 35), wxSize(60, 25),
		wxALIGN_LEFT | wxSP_ARROW_KEYS, 2, 8, canvas->renderer->GetFilterMeshRes());

	prefResSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt) { canvas->renderer->SetFilterMeshRes(evt.GetValue()); });

	wxStaticText* finalResLabel = new wxStaticText(dialogPanel, wxID_ANY, "Final Mesh Resolution", wxPoint(10, 68));
	wxSpinCtrl* finalResSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 65), wxSize(60, 25),
		wxALIGN_LEFT | wxSP_ARROW_KEYS, 3, 12, canvas->renderer->GetFinalMeshRes());

	finalResSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt)
		{
			canvas->renderer->SetFinalMeshRes(evt.GetValue());
			detailSpinner->SetValue(evt.GetValue());
		});

	dialog->ShowModal();
}

void Main::OnDisplayStandardOutput(wxCommandEvent& evt)
{

}

void Main::OnDisplaySeeds(wxCommandEvent& evt)
{
	canvas->DisplaySeeds(evt.IsChecked());
}

void Main::OnDisplayMesh(wxCommandEvent& evt)
{
	canvas->DisplayMeshes(evt.IsChecked());
}

void Main::OnEquationEdit(wxListEvent& evt)
{
	int i = evt.GetIndex();
	size_t data = equationList->GetListCtrl()->GetItemData(i);
	if (!data)
	{
		// New equation
		// Assign new equation with custom ID
		size_t id = nextEqnID++;
		equationList->GetListCtrl()->SetItemData(i, id);

		std::string eqnStr = evt.GetText().ToStdString();
		std::string funcStr = ProcessEquationString(eqnStr);
		canvas->renderer->NewJob(funcStr, canvas->bounds, id);
	}
	else
	{
		// Edit existing equation
		size_t id = data;

		std::string eqnStr = evt.GetText().ToStdString();
		std::string funcStr = ProcessEquationString(eqnStr);
		canvas->renderer->EditJob(id, funcStr);
	}
	evt.Skip();
}

void Main::OnEquationDelete(wxListEvent& evt)
{
	canvas->renderer->DeleteJob(evt.GetData());
	canvas->Refresh();
	evt.Skip();
}

std::string Main::ProcessEquationString(std::string_view eqnStr)
{
	std::stringstream ss;
	ss << eqnStr;

	std::string left, right;
	std::getline(ss, left, '=');
	std::getline(ss, right, '=');

	return left + "-(" + right + ")";
}