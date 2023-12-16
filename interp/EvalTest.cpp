/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "Parser.h"
#include "Object.h"
#include "Evaluator.h"

void testEvalIntegerExpressions()
{
    std::string input = "if (true) {3} else {2}";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    auto environment = std::make_shared<obj::Environment>();
    auto object = eval(std::move(program), environment);
    if (object->type != obj::ObjectType::Integer || object->inspect() != "3")
        throw std::runtime_error("Expected value 3 got something else");
}

int main()
{
    try
    {
        testEvalIntegerExpressions();
        std::cerr << "All tests passed" << std::endl;
        return 0;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
}