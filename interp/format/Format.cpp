/*******************************************************************
 * Copyright (c) 2022-2024 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/
#include "Format.h"
#include <stdio.h>

namespace builtin
{
    Formatting parseFormatting(const std::string &formatStr)
    {
        size_t currentFormatPosition = 0;
        Formatting formatting;
        if (formatStr.size() > 1 && std::string("<>^").find(formatStr[currentFormatPosition + 1]) != std::string::npos)
        {
            // first character will be a fill character
            formatting.fill = true;
            formatting.fillChar = formatStr[currentFormatPosition];
            switch (formatStr[currentFormatPosition + 1])
            {
            case '<':
                formatting.alignment = Formatting::Align::Left;
                break;
            case '>':
                formatting.alignment = Formatting::Align::Right;
                break;
            case '^':
                formatting.alignment = Formatting::Align::Center;
                break;
            default:
                formatting.error = "Unknown alignment specification " + formatStr[currentFormatPosition + 1];
                return formatting;
            }
            currentFormatPosition += 2;
        }

        // sign (optional)
        if (std::string("+- ").find(formatStr[currentFormatPosition]) != std::string::npos)
        {
            switch (formatStr[currentFormatPosition])
            {
            case ' ':
                formatting.sign = Formatting::Sign::Space;
                break;
            case '+':
                formatting.sign = Formatting::Sign::Plus;
                break;
            case '-':
                formatting.sign = Formatting::Sign::Minus;
                break;
            default:
                formatting.error = "Unknown sign specification " + formatStr[currentFormatPosition];
                return formatting;
            }
            ++currentFormatPosition;
        }

        if (currentFormatPosition < formatStr.size() && formatStr[currentFormatPosition] == '#')
        {
            formatting.alternativeForm = true;
            ++currentFormatPosition;
        }

        if (currentFormatPosition < formatStr.size() && formatStr[currentFormatPosition] == '0')
        {
            formatting.zeroPadding = true;
            ++currentFormatPosition;
        }

        // read the width
        size_t startWidth = currentFormatPosition;
        while (currentFormatPosition < formatStr.size() && std::isdigit(formatStr[currentFormatPosition]))
        {
            ++currentFormatPosition;
        }

        if (currentFormatPosition - startWidth > 0)
        {
            formatting.width = std::atoi(formatStr.substr(startWidth, currentFormatPosition - startWidth).c_str());
        }

        if (currentFormatPosition < formatStr.size() && formatStr[currentFormatPosition] == '.')
        {
            // read the precision
            ++currentFormatPosition;
            size_t startPrecision = currentFormatPosition;
            while (currentFormatPosition < formatStr.size() && std::isdigit(formatStr[currentFormatPosition]))
            {
                ++currentFormatPosition;
            }
            if (currentFormatPosition - startPrecision > 0)
            {
                formatting.precision = std::atoi(formatStr.substr(startPrecision, currentFormatPosition - startPrecision).c_str());
            }
        }

        if (currentFormatPosition < formatStr.size() && formatStr[currentFormatPosition] == 'L')
        {
            formatting.localeSpecific = true;
            ++currentFormatPosition;
        }

        if (currentFormatPosition < formatStr.size())
        {
            switch (formatStr[currentFormatPosition])
            {
            case '?':
                formatting.type = Formatting::Type::Question;
                break;
            case 'b':
                formatting.type = Formatting::Type::b;
                break;
            case 'B':
                formatting.type = Formatting::Type::B;
                break;
            case 'c':
                formatting.type = Formatting::Type::c;
                break;
            case 'd':
                formatting.type = Formatting::Type::d;
                break;
            case 'o':
                formatting.type = Formatting::Type::o;
                break;
            case 'x':
                formatting.type = Formatting::Type::x;
                break;
            case 'X':
                formatting.type = Formatting::Type::X;
                break;
            case 'a':
                formatting.type = Formatting::Type::a;
                break;
            case 'A':
                formatting.type = Formatting::Type::A;
                break;
            case 'e':
                formatting.type = Formatting::Type::e;
                break;
            case 'E':
                formatting.type = Formatting::Type::E;
                break;
            case 'f':
                formatting.type = Formatting::Type::f;
                break;
            case 'F':
                formatting.type = Formatting::Type::F;
                break;
            case 'g':
                formatting.type = Formatting::Type::g;
                break;
            case 'G':
                formatting.type = Formatting::Type::G;
                break;
            case 'p':
                formatting.type = Formatting::Type::p;
                break;
            case 'P':
                formatting.type = Formatting::Type::P;
                break;
            default:
                formatting.error = "Unknown type specifier " + formatStr[currentFormatPosition];
                return formatting;
            };
        }

        return formatting;
    }

    std::string formatDouble(double value, const Formatting &formatting)
    {
        char buffer[64];
        std::string formatDelegator = "%";

        if (formatting.zeroPadding)
            formatDelegator += "0";

        if (formatting.width >= 0)
            formatDelegator += std::to_string(formatting.width);

        if (formatting.precision >= 0)
            formatDelegator += "." + std::to_string(formatting.precision);

        switch (formatting.type)
        {
        case Formatting::Type::None:
        case Formatting::Type::f:
            formatDelegator += "f";
            break;
        case Formatting::Type::F:
            formatDelegator += "F";
            break;
        case Formatting::Type::e:
            formatDelegator += "e";
            break;
        case Formatting::Type::E:
            formatDelegator += "E";
            break;
        case Formatting::Type::g:
            formatDelegator += "g";
            break;
        case Formatting::Type::G:
            formatDelegator += "G";
            break;
        default:
            return std::string(); // empty indicates error happened
        }

        sprintf_s(buffer, sizeof(buffer) - 1, formatDelegator.c_str(), value);

        auto result = std::string(buffer);

        switch (formatting.alignment)
        {
        case Formatting::Align::Left:
        {
            size_t originalLength = result.size();
            // perform an lstrip() equivalent operation
            result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch)
                                                      { return !std::isspace(ch); }));
            size_t newLength = result.size();
            result.append(originalLength - newLength, formatting.fill ? formatting.fillChar : ' ');
            break;
        }
        case Formatting::Align::Right:
        {
            size_t originalLength = result.size();
            // perform an lstrip() equivalent operation
            result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch)
                                                      { return !std::isspace(ch); }));
            size_t newLength = result.size();
            result.insert(result.begin(), originalLength - newLength, formatting.fill ? formatting.fillChar : ' ');
            break;
        }
        break;
        }

        // if (formatting.fill)
        //     formatDelegator += formatting.fillChar;

        return result;
    }

    std::string format_impl(obj::Object *obj, const Formatting &formatting)
    {
        switch (obj->type)
        {
        case obj::ObjectType::Double:
            return formatDouble(static_cast<obj::Double *>(obj)->value, formatting);
        }
        return obj->inspect();
    };
}