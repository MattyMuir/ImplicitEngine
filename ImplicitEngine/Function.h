#pragma once
#include <string>

// ===== Settings =====
#define USE_EXPRTK 0
#define USE_INLINING 1
// ====================

#if USE_INLINING == 1
#define FUNC inline
#else
#define FUNC __declspec(noinline)
#endif

#if USE_EXPRTK == 1
#include "exprtk.hpp"
#endif

class Function
{
public:
#if USE_EXPRTK == 1
	Function(std::string_view exprStr_)
	{
		Construct(exprStr_);
	}

	// Copy constructor
	Function(const Function& other)
	{
		Construct(other.exprStr);
	}

	// Move constructor
	Function(Function&& other)
	{
		throw;
	}

	double operator()(double x_, double y_)
	{
		x = x_;
		y = y_;
		return expr.value();
	}
#else
	Function(std::string_view exprStr_)
		: exprStr(exprStr_) {}

	FUNC double operator()(double x, double y)
	{
		return x * x + y * y - 1;
	}
#endif

#if USE_EXPRTK == 1
	void Construct(std::string_view exprStr_)
	{
		exprStr = exprStr_;
		exprtk::symbol_table<double> symbols;
		symbols.add_variable("x", x);
		symbols.add_variable("y", y);
		expr.register_symbol_table(symbols);

		exprtk::parser<double> parser;
		parser.compile(exprStr, expr);
	}

	double x = 0, y = 0;
	exprtk::expression<double> expr;
#endif
	std::string exprStr;
};