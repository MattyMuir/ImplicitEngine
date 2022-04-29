#include "App.h"

wxIMPLEMENT_APP(App);

App::App()
{

}

bool App::OnInit()
{
	freopen("log.log", "w", stdout);
	freopen("err.log", "w", stderr);

	frame = new Main;
	frame->Show();

	return true;
}