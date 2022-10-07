#include "Function.h"
#include <exprtk.hpp>

Function::Function(std::string_view exprStr_)
{
	Construct(exprStr_);
}

Function::~Function()
{
	delete expr;
}

Function::Function(const Function& other)
{
	Construct(other.exprStr);
}

double Function::operator()(double x_, double y_)
{
	x = x_;
	y = y_;
	return expr->value();
}

void Function::Construct(std::string_view exprStr_)
{
	if (!expr) expr = new exprtk::expression<double>;

	exprStr = exprStr_;
	exprtk::symbol_table<double> symbols;
	symbols.add_constants();
	symbols.add_variable("x", x);
	symbols.add_variable("y", y);
	expr->register_symbol_table(symbols);

	exprtk::parser<double> parser;
	isValid = parser.compile(exprStr, *expr);
}