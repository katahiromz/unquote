// unquote.hpp --- Unquote C string literals
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#ifndef UNQUOTE_HPP_
#define UNQUOTE_HPP_    3   // Version 3

#include <string>

std::string  unquote(const char *str);
std::wstring unquote(const wchar_t *str);

void unquote_unittest(void);

#endif  // ndef UNQUOTE_HPP_
