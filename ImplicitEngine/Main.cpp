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
	wxGLAttributes atts;
	atts.PlatformDefaults().RGBA().DoubleBuffer();
	if (useAntialiasing)
		atts.SampleBuffers(1).Samplers(antialiasingSamples);
	atts.EndList();

	canvas = new Canvas(horizSplitter, atts);

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

void Main::OnMenuExit(wxCommandEvent&)
{
	Destroy();
}

void Main::OnGearPressed(wxCommandEvent&)
{
	wxDialog* dialog = new wxDialog(this, wxID_ANY, "Advanced Render Settings", wxDefaultPosition, wxSize(305, 135));
	wxPanel* dialogPanel = new wxPanel(dialog);
	dialogPanel->SetFocus();

	// Spinners
	wxSpinCtrl* seedNumSpinner, * prefResSpinner, * finalResSpinner;

	new wxStaticText(dialogPanel, wxID_ANY, "Prefiltering Seeds", wxPoint(10, 8));
	seedNumSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 5), wxSize(65, 25),
		wxALIGN_LEFT | wxSP_ARROW_KEYS, 32, 131072, canvas->renderer->GetSeedNum());

	seedNumSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt) { canvas->renderer->SetSeedNum(evt.GetValue()); });

	new wxStaticText(dialogPanel, wxID_ANY, "Prefiltering Resolution", wxPoint(10, 38));
	prefResSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 35), wxSize(65, 25),
		wxALIGN_LEFT | wxSP_ARROW_KEYS, 2, 8, canvas->renderer->GetFilterMeshRes());

	new wxStaticText(dialogPanel, wxID_ANY, "Final Mesh Resolution", wxPoint(10, 68));
	finalResSpinner = new wxSpinCtrl(dialogPanel, wxID_ANY, "", wxPoint(140, 65), wxSize(65, 25),
		wxALIGN_LEFT | wxSP_ARROW_KEYS, canvas->renderer->GetFilterMeshRes(), 12, canvas->renderer->GetFinalMeshRes());

	prefResSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt)
		{
			canvas->renderer->SetFilterMeshRes(evt.GetValue());
			finalResSpinner->SetMin(evt.GetValue());
		});

	finalResSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt)
		{
			canvas->renderer->SetFinalMeshRes(evt.GetValue());
			detailSpinner->SetValue(evt.GetValue());
			prefResSpinner->SetMax(evt.GetValue());
		});

	// Buttons
	wxButton* autoSeedsBtn = new wxButton(dialogPanel, wxID_ANY, "Auto", wxPoint(215, 5), wxSize(60, 25));
	autoSeedsBtn->SetToolTip("Automatically decide a number of seeds based on prefiltering resolution");
	autoSeedsBtn->Bind(wxEVT_BUTTON, [=](wxCommandEvent&)
		{
			seedNumSpinner->SetValue((int)pow(4, 0.5 + prefResSpinner->GetValue()));
			canvas->renderer->SetSeedNum(seedNumSpinner->GetValue());
		});

	wxButton* autoResBtn = new wxButton(dialogPanel, wxID_ANY, "Auto", wxPoint(215, 35), wxSize(60, 25));
	autoResBtn->SetToolTip("Automatically decide a prefiltering resolution based on the number of seeds");
	autoResBtn->Bind(wxEVT_BUTTON, [=](wxCommandEvent&)
		{
			prefResSpinner->SetValue((int)floor(log(seedNumSpinner->GetValue()) / log(4)));
			canvas->renderer->SetFilterMeshRes(prefResSpinner->GetValue());
			finalResSpinner->SetMin(prefResSpinner->GetValue());
		});

	dialog->ShowModal();
}

void Main::OnDisplayStandardOutput(wxCommandEvent& evt)
{
	canvas->DisplayStandard(evt.IsChecked());
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