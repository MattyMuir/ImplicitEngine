#pragma once
#pragma warning(push, 1)
#include <wx/wx.h>
#pragma warning(pop)
#include "Main.h"

class App : public wxApp
{
public:
	App();
	virtual bool OnInit();

private:
	Main* frame = nullptr;
};