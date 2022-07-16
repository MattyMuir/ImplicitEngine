#include "FunctionPack.h"

FunctionPack::FunctionPack(std::string_view funcStr_, int size)
	: funcStr(funcStr_)
{
	funcs.reserve(size);
	for (int i = 0; i < size; i++)
		funcs.push_back(new Function(funcStr));
}

FunctionPack::~FunctionPack()
{
	for (Function* func : funcs)
		delete func;
}

void FunctionPack::Resize(int size)
{
	int dif = size - funcs.size();
	for (int i = 0; i < dif; i++)
		funcs.push_back(new Function(funcStr));
}

void FunctionPack::Change(std::string_view funcStr_)
{
	funcStr = funcStr_;
	for (Function* func : funcs)
		func->Construct(funcStr);
}

Function* FunctionPack::operator[](int index)
{
	return funcs[index];
}