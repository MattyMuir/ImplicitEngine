#include "App.h"

wxIMPLEMENT_APP(App);

App::App()
{

}

bool App::OnInit()
{
	freopen("log.txt", "w", stdout);
	freopen("err.txt", "w", stderr);

	frame = new Main;
	frame->Show();

	return true;
}