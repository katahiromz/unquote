// unquote.hpp --- Unquote C string literals
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#ifndef UNQUOTE_HPP_
#define UNQUOTE_HPP_    5   // Version 5

#include <string>

std::string  unquote(const char *str);
std::wstring unquote(const wchar_t *str);

#if __cplusplus >= 201103L  // C++11
    #if defined(HAVE_ICONV) || defined(_WIN32)
        std::u16string unquote(const char16_t *str);
    #endif
    #ifdef HAVE_ICONV
        std::u32string unquote(const char32_t *str);
    #endif
#endif

void unquote_unittest(void);

#endif  // ndef UNQUOTE_HPP_
