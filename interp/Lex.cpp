/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include <string>
#include <iostream>
#include "Token.h"
#include "Lexer.h"

int main()
{
    const std::string prompt = ">> ";

    while (true)
    {
        std::cout << prompt;
        std::string text;
        std::getline(std::cin, text);
        if (text.empty())
            break;

        auto lexer = createLexer(text, "");
        for (auto token = nextToken(*lexer); token.type != TokenType::EOF_T; token = nextToken(*lexer))
        {
            std::cout << token.literal << " -> " << toString(token.type) << std::endl;
        }
    };

    return 0;
}