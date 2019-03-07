// unquote.cpp --- Unquote C string literals
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.

#include "unquote.hpp"

#ifdef _WIN32
    #include <windows.h>
#endif
#include <cassert>

template <typename T_CHAR>
inline bool unquote_isxdigit(T_CHAR ch)
{
    if (T_CHAR('0') <= ch && ch <= T_CHAR('9'))
        return true;
    if (T_CHAR('A') <= ch && ch <= T_CHAR('F'))
        return true;
    if (T_CHAR('a') <= ch && ch <= T_CHAR('f'))
        return true;
    return false;
}

template <typename T_CHAR>
inline bool unquote_isspace(T_CHAR ch)
{
    switch (ch)
    {
    case T_CHAR(' '):
    case T_CHAR('\t'):
    case T_CHAR('\n'):
    case T_CHAR('\r'):
    case T_CHAR('\f'):
    case T_CHAR('\v'):
        return true;
    default:
        return false;
    }
}

template <typename T_CHAR>
std::basic_string<T_CHAR>
unquote_generic(const T_CHAR *str)
{
    std::basic_string<T_CHAR> ret;
    bool in_quote = false;
    for (const T_CHAR *pch = str; *pch; ++pch)
    {
        if (!in_quote)
        {
            while (unquote_isspace(*pch))
            {
                ++pch;
            }

            if (*pch == 0)
                break;

            if (*pch == T_CHAR('"'))
            {
                in_quote = true;
            }
            else
            {
                ret.clear();
                return ret;     // invalid
            }

            continue;
        }

        if (*pch == T_CHAR('\"'))
        {
            if (pch[1] == T_CHAR('\"'))
            {
                ret += T_CHAR('\"');
                ++pch;
            }
            else
            {
                in_quote = false;
            }
            continue;
        }

        if (*pch == T_CHAR('\\'))
        {
            ++pch;
            switch (*pch)
            {
            case 'a': ret += T_CHAR('\a'); break;
            case 'b': ret += T_CHAR('\b'); break;
            case 'f': ret += T_CHAR('\f'); break;
            case 'n': ret += T_CHAR('\n'); break;
            case 'r': ret += T_CHAR('\r'); break;
            case 't': ret += T_CHAR('\t'); break;
            case 'v': ret += T_CHAR('\v'); break;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                {
                    int value = *pch - '0';
                    ++pch;
                    if ('0' <= *pch && *pch < '8')
                    {
                        value *= 8;
                        value += *pch - '0';
                        ++pch;
                        if ('0' <= *pch && *pch < '8')
                        {
                            value *= 8;
                            value += *pch - '0';
                            ++pch;
                        }
                    }
                    ret += T_CHAR(value);
                    --pch;
                }
                break;
            case 'x':
                {
                    std::string hex;
                    size_t i = 0;
                    ++pch;
                    while (i < 2 && unquote_isxdigit(*pch))
                    {
                        hex += char(*pch);
                        ++pch;
                        ++i;
                    }
                    int value = int(strtoul(hex.c_str(), NULL, 16));
                    ret += T_CHAR(value);
                    --pch;
                }
                break;
            case 'u':
                {
                    std::string hex;
                    size_t i = 0;
                    ++pch;
                    while (i < 4 && unquote_isxdigit(*pch))
                    {
                        hex += char(*pch);
                        ++pch;
                        ++i;
                    }
                    wchar_t uni = wchar_t(strtoul(hex.c_str(), NULL, 16));
#ifdef _WIN32
                    if (sizeof(T_CHAR) == 1 && uni >= 0x7F)
                    {
                        char sz[8] = { 0 };
                        WideCharToMultiByte(CP_ACP, 0, &uni, 1, sz, 8, NULL, NULL);
                        for (char *q = sz; *q; ++q)
                        {
                            ret += T_CHAR(*q);
                        }
                    }
                    else
#endif
                    {
                        ret += T_CHAR(uni);
                    }
                    --pch;
                }
                break;
#ifndef _WIN32
            case 'U':
                {
                    std::string hex;
                    size_t i = 0;
                    ++pch;
                    while (i < 8 && unquote_isxdigit(*pch))
                    {
                        hex += char(*pch);
                        ++pch;
                        ++i;
                    }
                    T_CHAR uni = (T_CHAR)strtoul(hex.c_str(), NULL, 16);
                    ret += uni;
                    --pch;
                }
                break;
#endif
            case '\\': case '"':
            default:
                ret += *pch;
                break;
            }
        }
        else
        {
            ret += *pch;
        }
    }

    return ret;
}

std::string unquote(const char *str)
{
    return unquote_generic<char>(str);
}

std::wstring unquote(const wchar_t *str)
{
    return unquote_generic<wchar_t>(str);
}

void unquote_unittest(void)
{
    assert(unquote("\"\"") == "");
    assert(unquote("\"\\2\"") == "\x02");
    assert(unquote("\"\\02\"") == "\x02");
    assert(unquote("\"\\002\"") == "\x02");
    assert(unquote("\"\\x2\"") == "\x02");
    assert(unquote("\"\\x02\"") == "\x02");
    assert(unquote("\"\\x22\" \"BBB\"") == "\x22" "BBB");
    assert(unquote("\"\\u0002\"") == "\u0002");
    assert(unquote("\"A\"") == "A");
    assert(unquote("\"ABC\"") == "ABC");
    assert(unquote("   \"ABC\"  ") == "ABC");
    assert(unquote("   \"ABC  ") == "ABC  ");
    assert(unquote("   \"A\" \"BC\"  ") == "ABC");
    assert(unquote("\"\\001\"") == "\x01");
    assert(unquote("\"\\010\"") == "\x08");
    assert(unquote("\"\\100\"") == "\x40");
    assert(unquote("\"\\007ABC\"") == "\x07" "ABC");
    assert(unquote("\"\\x20\"") == "\x20");
    assert(unquote("\"\\x40\"") == "\x40");
    assert(unquote("\"hello\\r\\n\"") == "hello\r\n");
    assert(unquote("\"Hello,\nworld!\\r\\n\"") == "Hello,\nworld!\r\n");
    assert(unquote("\"This\\nis\\na\\ntest.\"") == "This\nis\na\ntest.");

    assert(unquote(L"\"\"") == L"");
    assert(unquote(L"\"\\2\"") == L"\x02");
    assert(unquote(L"\"\\02\"") == L"\x02");
    assert(unquote(L"\"\\002\"") == L"\x02");
    assert(unquote(L"\"\\x2\"") == L"\x02");
    assert(unquote(L"\"\\x02\"") == L"\x02");
    assert(unquote(L"\"\\x22\" \"BBB\"") == L"\x22" L"BBB");
    assert(unquote(L"\"\\u0002\"") == L"\u0002");
    assert(unquote(L"\"A\"") == L"A");
    assert(unquote(L"\"ABC\"") == L"ABC");
    assert(unquote(L"   \"ABC\"  ") == L"ABC");
    assert(unquote(L"   \"ABC  ") == L"ABC  ");
    assert(unquote(L"   \"A\" \"BC\"  ") == L"ABC");
    assert(unquote(L"\"\\001\"") == L"\x01");
    assert(unquote(L"\"\\010\"") == L"\x08");
    assert(unquote(L"\"\\100\"") == L"\x40");
    assert(unquote(L"\"\\007ABC\"") == L"\x07" L"ABC");
    assert(unquote(L"\"\\x20\"") == L"\x20");
    assert(unquote(L"\"\\x40\"") == L"\x40");
    assert(unquote(L"\"hello\\r\\n\"") == L"hello\r\n");
    assert(unquote(L"\"Hello,\nworld!\\r\\n\"") == L"Hello,\nworld!\r\n");
    assert(unquote(L"\"This\\nis\\na\\ntest.\"") == L"This\nis\na\ntest.");

#if defined(JAPAN) && defined(_WIN32)
    puts("japan");
    // Hiragana A I U
    assert(unquote("\"\\u3042\\u3044\\u3046\"") == "\x82\xA0\x82\xA2\x82\xA4");
    assert(unquote(L"\"\\u3042\\u3044\\u3046\"") == L"\u3042\u3044\u3046");
#endif

    puts("OK");
}

#ifdef UNQUOTE_UNITTEST
int main(void)
{
    unquote_unittest();
    return 0;
}
#endif
