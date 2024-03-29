#pragma once
#include <string>

namespace exprtk
{
	template <typename T>
	class expression;
}

class Function
{
public:
	Function(std::string_view exprStr_);

	~Function();

	// Copy constructor
	Function(const Function& other);

	// Delete move contructor
	Function(Function&& other) = delete;

	double operator()(double x_, double y_);

	void Construct(std::string_view exprStr_);

	bool isValid;

protected:
	double x = 0, y = 0;
	exprtk::expression<double>* expr = nullptr;
	std::string exprStr;
};