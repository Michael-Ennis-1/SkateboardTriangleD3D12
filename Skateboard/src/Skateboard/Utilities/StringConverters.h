#pragma once
#include <codecvt>
#include <locale>

namespace Skateboard
{
	__inline std::wstring ToWString(const std::string& str)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converter;

		return converter.from_bytes(str);
	}

	__inline std::string ToString(const std::wstring& wstr)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converter;

		return converter.to_bytes(wstr);
	}
}
