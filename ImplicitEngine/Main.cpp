#include "Main.h"
#include "Canvas.h"

wxBEGIN_EVENT_TABLE(Main, wxFrame)
	EVT_MENU(20001, Main::OnMenuExit)
	EVT_MENU(30001, Main::OnDisplayStandardOutput)
	EVT_MENU(30002, Main::OnDisplaySeeds)
	EVT_MENU(30003, Main::OnDisplayMesh)
	EVT_BUTTON(10002, Main::OnGearPressed)
	EVT_BUTTON(10004, Main::OnHomePressed)
	EVT_BUTTON(10005, Main::OnColWheelPressed)
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
	detailSpinner = new wxSpinCtrl(topBar, 10001, "", wxPoint(90, 5), wxSize(50, 25), wxALIGN_LEFT | wxSP_ARROW_KEYS, 5, 12, 9);

	detailSpinner->Bind(wxEVT_SPINCTRL, [=](wxSpinEvent& evt) { canvas->renderer->SetFinalMeshRes(evt.GetValue()); });

	detailGearButton = new wxButton(topBar, 10002, "", wxPoint(140, 5), wxSize(25, 25));
	detailGearButton->SetBitmap(wxImage(25, 25, gearCol, gearAlpha, true));
	detailGearButton->SetToolTip("Advanced render settings");

	homeBtn = new wxButton(topBar, 10004, "", wxPoint(170, 5), wxSize(25, 25));
	homeBtn->SetBitmap(wxImage(25, 25, homeCol, homeAlpha, true));
	homeBtn->SetToolTip("Reset view");

	colBtn = new wxButton(topBar, 10005, "", wxPoint(200, 5), wxSize(25, 25));
	colBtn->SetBitmap(wxImage(25, 25, wheelCol, wheelAlpha, true));
	colBtn->SetToolTip("Change equation colors");

	// Main controls
	horizSplitter = new wxSplitterWindow(this, 10003, wxDefaultPosition, wxDefaultSize, wxSP_BORDER | wxSP_LIVE_UPDATE);

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
			detailSpinner->SetMin(evt.GetValue());
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

void Main::OnHomePressed(wxCommandEvent&)
{
	canvas->ResetView();
	Refresh();
}

void Main::OnColWheelPressed(wxCommandEvent&)
{
	wxDialog* dlg = new wxDialog(this, wxID_ANY, "Change Equation Colors", wxDefaultPosition, wxSize(200, 170));
	wxPanel* dlgPanel = new wxPanel(dlg);
	
	// Label
	new wxStaticText(dlgPanel, wxID_ANY, "Equation", { 10, 10 }, { 50, 15 });

	// Choice box
	wxArrayString eqnStrings;
	equationList->GetStrings(eqnStrings);

	wxChoice* eqnChoice = new wxChoice(dlgPanel, wxID_ANY, { 10, 30 }, { 160, 25 }, eqnStrings);

	// Color picker
	wxColourPickerCtrl* clrPicker = new wxColourPickerCtrl(dlgPanel, wxID_ANY, { 0, 0, 0 }, { 10, 60 }, { 70, 25 });

	// Choice box handler
	eqnChoice->Bind(wxEVT_CHOICE, [=](wxCommandEvent&)
		{
			int selection = eqnChoice->GetSelection();
			if (selection != wxNOT_FOUND)
			{
				size_t eqnID = equationList->GetListCtrl()->GetItemData(selection);
				clrPicker->SetColour(canvas->renderer->GetJobColour(eqnID));
			}
		});

	// Button
	wxButton* okBtn = new wxButton(dlgPanel, wxID_ANY, "Ok", { 10, 90 }, { 35, 25 });
	okBtn->Bind(wxEVT_BUTTON, [=](wxCommandEvent&)
		{
			int selection = eqnChoice->GetSelection();

			if (selection == wxNOT_FOUND) ErrorDialog("Select an equation!");
			else
			{
				size_t eqnID = equationList->GetListCtrl()->GetItemData(selection);
				canvas->renderer->SetJobColor(eqnID, clrPicker->GetColour());
				dlg->Destroy();
			}
		});

	dlg->ShowModal();
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

		bool strValid = true;
		std::string funcStr = ProcessEquationString(eqnStr, &strValid);
		bool compValid = canvas->renderer->NewJob(funcStr, canvas->bounds, id, strValid);

		if (!compValid) ErrorDialog("Compilation Failed");
	}
	else
	{
		// Edit existing equation
		size_t id = data;

		std::string eqnStr = evt.GetText().ToStdString();

		bool strValid = true;
		std::string funcStr = ProcessEquationString(eqnStr, &strValid);
		bool compValid = canvas->renderer->EditJob(id, funcStr, strValid);

		if (!compValid) ErrorDialog("Compilation Failed");
	}
	evt.Skip();
}

void Main::OnEquationDelete(wxListEvent& evt)
{
	canvas->renderer->DeleteJob(evt.GetData());
	canvas->Refresh();
	evt.Skip();
}

void Main::ErrorDialog(std::string_view msg)
{
	wxDialog* dlg = new wxDialog(this, wxID_ANY, "Error", wxDefaultPosition, wxSize(200, 100));
	new wxStaticText(dlg, wxID_ANY, msg.data(), wxPoint(0, 0));

	dlg->ShowModal();
}

std::string Main::ProcessEquationString(std::string_view eqnStr, bool* isValid)
{
	std::vector<std::string> splitEqn = Split(eqnStr, "=");

	if (isValid) *isValid = (splitEqn.size() == 2);
	if (splitEqn.size() != 2)
	{
		ErrorDialog("Equation must have exactly one equals sign.");
		return "0";
	}

	return splitEqn[0] + "-(" + splitEqn[1] + ")";
}