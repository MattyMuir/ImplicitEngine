#pragma once
#include <vector>

#include "Function.h"

class FunctionPack
{
public:
	FunctionPack(std::string_view funcStr_, int size);
	~FunctionPack();

	void Resize(int size);
	void Change(std::string_view funcStr_);

	Function* operator[](int index);

protected:
	std::string funcStr;
	std::vector<Function*> funcs;
};