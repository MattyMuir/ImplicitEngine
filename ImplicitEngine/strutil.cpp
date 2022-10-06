#include "strutil.h"

#include <ranges>

std::vector<std::string> Split(std::string_view str, std::string_view delim)
{
	std::vector<std::string> parts;

	for (const auto& part : std::views::split(str, delim))
		parts.emplace_back(part.begin(), part.end());

	return parts;
}