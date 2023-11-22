#include <string>
#include <iostream>
#include <sstream>
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"

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
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();
        if (!parser->errorMsgs.empty())
        {
            std::stringstream ss;
            for (const auto &msg : parser->errorMsgs)
                ss << msg << std::endl;
            std::cerr << ss.str();
        }
        std::cout << program->text();
        std::cout << std::endl;
    };

    return 0;
}