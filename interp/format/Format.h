/*******************************************************************
 * Copyright (c) 2022-2024 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_FORMAT_FORMAT_H
#define GUARDIAN_OF_INCLUSION_FORMAT_FORMAT_H

#include "../Object.h"

namespace builtin
{
    struct Formatting
    {
        enum class Align
        {
            None = 0,
            Left = 1,
            Right = 2,
            Center = 3,
        };
        bool fill = false;
        char fillChar = ' ';
        Align alignment = Align::None;

        enum class Sign
        {
            None = 0,
            Plus = 1,
            Minus = 2,
            Space = 3,
        };
        Sign sign = Sign::None;

        bool alternativeForm = false;
        bool zeroPadding = false;
        int padding = -1;
        int width = -1;
        int precision = -1;
        bool localeSpecific = false;

        enum class Type
        {
            None = 0,
            Question = 1,
            b = 2,
            B = 3,
            c = 4,
            d = 5,
            o = 6,
            x = 7,
            X = 8,
            a = 9,
            A = 10,
            e = 11,
            E = 12,
            f = 13,
            F = 14,
            g = 15,
            G = 16,
            p = 17,
            P = 18,
        };
        Type type = Type::None;

        std::string error;
    };

    Formatting parseFormatting(const std::string &formatStr);

    std::string format_impl(obj::Object *obj, const Formatting &formatting);
}

#endif