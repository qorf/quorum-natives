/*
    A small stand-in for ATL's CString, with only what the speech code uses.
    It is not ATL. See atlbase.h for when this folder is used.
*/
#pragma once
#include "atlbase.h"
#include <string>

class CString : public std::wstring {
public:
    CString() {}
    CString(const wchar_t* text) : std::wstring(text ? text : L"") {}

    // Like ATL, char text is converted with the system's ANSI code page.
    CString(const char* text)
    {
        if (text == nullptr) return;
        int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
        if (length <= 1) return;
        std::wstring wide(length - 1, L'\0');
        MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], length);
        assign(wide);
    }

    operator const wchar_t*() const { return c_str(); }

    // Case-sensitive, like ATL's CString::Compare.
    int Compare(const wchar_t* other) const { return compare(other ? other : L""); }
};
