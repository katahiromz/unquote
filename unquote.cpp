// unquote.cpp --- Unquote C string literals
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.

#include "unquote.hpp"
#ifdef HAVE_ICONV
    #include "iconv_wrap.hpp"
#endif
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
void unquote_store_utf16(std::basic_string<T_CHAR>& ret, int ch);

template <>
void unquote_store_utf16<wchar_t>(std::basic_string<wchar_t>& ret, int ch)
{
    ret += wchar_t(ch);
}

#if defined(_WIN32) || defined(HAVE_ICONV)
    template <>
    void unquote_store_utf16<char>(std::basic_string<char>& ret, int ch)
    {
        wchar_t wch = wchar_t(ch);
        char out[16] = { 0 };
    #ifdef _WIN32
        WideCharToMultiByte(CP_ACP, 0, &wch, 1, out, 8, NULL, NULL);
    #elif defined(HAVE_ICONV)
        #ifdef SHIFT_JIS
            static iconv_wrap ic("SHIFT_JIS", "WCHAR_T");
        #else
            static iconv_wrap ic("UTF-8", "WCHAR_T");
        #endif
        ic.reset();
        size_t in_left = sizeof(wch);
        size_t out_left = sizeof(out);
        ic.convert(&wch, &in_left, out, &out_left);
    #endif
        for (char *q = out; *q; ++q)
        {
            ret += char(*q);
        }
    }
#endif

template <typename T_CHAR>
void unquote_store_utf32(std::basic_string<T_CHAR>& ret, long ch);

#ifdef HAVE_ICONV
    template <>
    void unquote_store_utf32(std::basic_string<char>& ret, long ch)
    {
#ifdef SHIFT_JIS
        static iconv_wrap ic("SHIFT_JIS", "UTF-32");
#else
        static iconv_wrap ic("UTF-8", "UTF-32");
#endif
        ic.reset();
        char32_t uch = char32_t(ch);
        char out[16] = { 0 };
        size_t in_left = sizeof(uch);
        size_t out_left = sizeof(out);
        ic.convert(&uch, &in_left, out, &out_left);
        for (char *q = out; *q; ++q)
        {
            ret += char(*q);
        }
    }

    template <>
    void unquote_store_utf32(std::basic_string<wchar_t>& ret, long ch)
    {
        static iconv_wrap ic("WCHAR_T", "UTF-32");
        ic.reset();
        char32_t uch = char32_t(ch);
        char out[16] = { 0 };
        size_t in_left = sizeof(uch);
        size_t out_left = sizeof(out);
        ic.convert(&uch, &in_left, out, &out_left);
        for (char *q = out; *q; ++q)
        {
            ret += char(*q);
        }
    }

    #if __cplusplus >= 201103L  // C++11
        template <>
        void unquote_store_utf32(std::basic_string<char16_t>& ret, long ch)
        {
            static iconv_wrap ic("UTF-16", "UTF-32");
            ic.reset();
            char32_t uch = char32_t(ch);
            char out[16] = { 0 };
            size_t in_left = sizeof(uch);
            size_t out_left = sizeof(out);
            ic.convert(&uch, &in_left, out, &out_left);
            for (char *q = out; *q; ++q)
            {
                ret += char(*q);
            }
        }
    #endif
#endif

#if __cplusplus >= 201103L  // C++11
    template <>
    void unquote_store_utf16<char16_t>(std::basic_string<char16_t>& ret, int ch)
    {
        ret += char16_t(ch);
    }

    template <>
    void unquote_store_utf16<char32_t>(std::basic_string<char32_t>& ret, int ch)
    {
        ret += char32_t(ch);
    }

    template <>
    void unquote_store_utf32<char32_t>(std::basic_string<char32_t>& ret, long ch)
    {
        ret += char32_t(ch);
    }
#endif

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
#if defined(HAVE_ICONV) || defined(_WIN32)
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
                    int utf16 = int(strtoul(hex.c_str(), NULL, 16));
                    unquote_store_utf16<T_CHAR>(ret, utf16);
                    --pch;
                }
                break;
#endif
#ifdef HAVE_ICONV
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
                    long utf32 = (long)strtoul(hex.c_str(), NULL, 16);
                    unquote_store_utf32<T_CHAR>(ret, utf32);
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

#if __cplusplus >= 201103L
    std::u16string unquote(const char16_t *str)
    {
        return unquote_generic<char16_t>(str);
    }

    std::u32string unquote(const char32_t *str)
    {
        return unquote_generic<char32_t>(str);
    }
#endif

void unquote_unittest(void)
{
    assert(unquote("\"\"") == "");
    assert(unquote("\"\\2\"") == "\x02");
    assert(unquote("\"\\02\"") == "\x02");
    assert(unquote("\"\\002\"") == "\x02");
    assert(unquote("\"\\x2\"") == "\x02");
    assert(unquote("\"\\x02\"") == "\x02");
    assert(unquote("\"\\x22\" \"BBB\"") == "\x22" "BBB");
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

    assert(unquote(u"\"\"") == u"");
    assert(unquote(u"\"\\2\"") == u"\x02");
    assert(unquote(u"\"\\02\"") == u"\x02");
    assert(unquote(u"\"\\002\"") == u"\x02");
    assert(unquote(u"\"\\x2\"") == u"\x02");
    assert(unquote(u"\"\\x02\"") == u"\x02");
    assert(unquote(u"\"\\x22\" \"BBB\"") == u"\x22" u"BBB");
    assert(unquote(u"\"A\"") == u"A");
    assert(unquote(u"\"ABC\"") == u"ABC");
    assert(unquote(u"   \"ABC\"  ") == u"ABC");
    assert(unquote(u"   \"ABC  ") == u"ABC  ");
    assert(unquote(u"   \"A\" \"BC\"  ") == u"ABC");
    assert(unquote(u"\"\\001\"") == u"\x01");
    assert(unquote(u"\"\\010\"") == u"\x08");
    assert(unquote(u"\"\\100\"") == u"\x40");
    assert(unquote(u"\"\\007ABC\"") == u"\x07" u"ABC");
    assert(unquote(u"\"\\x20\"") == u"\x20");
    assert(unquote(u"\"\\x40\"") == u"\x40");
    assert(unquote(u"\"hello\\r\\n\"") == u"hello\r\n");
    assert(unquote(u"\"Hello,\nworld!\\r\\n\"") == u"Hello,\nworld!\r\n");
    assert(unquote(u"\"This\\nis\\na\\ntest.\"") == u"This\nis\na\ntest.");

#if defined(HAVE_ICONV) || defined(_WIN32)
    puts("UTF-16");
    assert(unquote("\"\\u0002\"") == "\u0002");
    assert(unquote(u"\"\\u0002\"") == u"\u0002");
    assert(unquote(L"\"\\u0002\"") == L"\u0002");
    assert(unquote(u"\"\\u3042\\u3044\\u3046\"") == u"\u3042\u3044\u3046");
#endif

#if (defined(HAVE_ICONV) || defined(_WIN32)) && defined(SHIFT_JIS)
    puts("SHIFT_JIS");
    assert(unquote("\"\\u3042\\u3044\\u3046\"") == "\x82\xA0\x82\xA2\x82\xA4");
#endif

    puts("OK");
}
