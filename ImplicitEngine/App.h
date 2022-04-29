#pragma once
#include "wx/wx.h"
#include "Main.h"

class App : public wxApp
{
public:
	App();
	virtual bool OnInit();

private:
	Main* frame = nullptr;
};