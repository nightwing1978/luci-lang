/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>

#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "Evaluator.h"
#include "Util.h"
#include "Version.h"

int interactiveMode(std::shared_ptr<obj::Environment> environment)
{
    const std::string prompt = util::color::colorize(">> ", util::color::fg::yellow);

    while (true)
    {
        std::cout << prompt;
        std::string text;
        std::getline(std::cin, text);

        if (text.empty())
            continue;
        auto lexer = createLexer(text, "");
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();

        if (!parser->errorMsgs.empty())
        {
            std::stringstream ss;
            for (const auto &msg : parser->errorMsgs)
                ss << msg << std::endl;
            std::cerr << util::color::colorize(ss.str(), util::color::fg::red);
        }
        else
        {
            if (program)
            {
                auto object = evalProgram(program.get(), environment);
                if (object && object->type == obj::ObjectType::Exit)
                {
                    auto exitObj = dynamic_cast<obj::Exit *>(object.get());
                    if (!exitObj)
                        return -1;
                    return exitObj->value;
                }
                if (object && object->type != obj::ObjectType::Null)
                {
                    if (object->type == obj::ObjectType::Error)
                        std::cerr << util::color::colorize(object->inspect(), util::color::fg::red) << std::endl;
                    else
                        std::cout << object->inspect() << std::endl;
                }
            }
        }
    };

    return 0;
}

void usage(int argc, char **argv)
{
    std::cout << argv[0] << "\n";
    std::cout << "Usage: \n";
    std::cout << argv[0] << " [-i] [-s] [-v] [file_name]\n";
    std::cout << "  -i			enter interactive mode after running the provided file_name\n";
    std::cout << "  -s			print statistics\n";
    std::cout << "  -v			print version\n";
    std::cout << "  -h			show this usage\n";
    std::cout << "  file_name	run the given file_name, when none given, enter interactive mode\n";
}

std::string compilerName()
{
#ifdef __GNUC__
    return "GCC";
#endif

#ifdef __clang__
    return "Clang";
#endif

#ifdef _MSC_VER
    return "MSVC";
#endif

#ifdef __MINGW32__
    return "MinGW";
#endif
}

void version(int argc, char **argv)
{
    std::cout << "Luci " << majorVersion << "." << minorVersion << "." << patchVersion << " (" << compilerName() << ")"
              << "\n";
}

int main(int argc, char **argv)
try
{
    //
    double cumulativeTime = 0.0;

    const std::string interactiveArgShort = "-i";
    const std::string interactiveArgLong = "--interactive";
    bool enterInteractive = false;

    const std::string statsArgShort = "-s";
    const std::string statsArgLong = "--statistics";
    bool showStatistics = false;

    const std::string versionArgShort = "-v";
    const std::string versionArgLong = "--version";
    bool showVersion = false;

    const std::string helpArgShort = "-h";
    const std::string helpArgLong = "--help";
    bool showHelp = false;

    std::string fileToRun = "";

    initialize();
    initializeArg(argc, argv);
    auto environment = std::make_shared<obj::Environment>();
    int returnValue = 2;
    if (argc == 1)
    {
        enterInteractive = true;
    }
    else
    {
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i] == statsArgShort || argv[i] == statsArgLong)
            {
                showStatistics = true;
            }
            else if (argv[i] == interactiveArgShort || argv[i] == interactiveArgLong)
            {
                enterInteractive = true;
            }
            else if (argv[i] == helpArgShort || argv[i] == helpArgLong)
            {
                usage(argc, argv);
                return 0;
            }
            else if (argv[i] == versionArgShort || argv[i] == versionArgLong)
            {
                version(argc, argv);
                return 0;
            }
            else
            {
                fileToRun = std::string(argv[i]);
            }
        }
    }

    if (!fileToRun.empty())
    {
        std::string text;
        std::string line;
        std::ifstream inputf;
        inputf.open(fileToRun);
        if (inputf.is_open())
        {
            while (std::getline(inputf, line))
            {
                text += line + "\n";
            }
        }
        else
        {
            std::cerr << "File " << fileToRun << " cannot be read" << std::endl;
            returnValue = 2;
        }
        auto lexer = createLexer(text, fileToRun);
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();

        if (!parser->errorMsgs.empty())
        {
            std::stringstream ss;
            for (const auto &msg : parser->errorMsgs)
                ss << msg << std::endl;
            std::cerr << util::color::colorize(ss.str(), util::color::fg::red);
        }
        else
        {
            if (program)
            {
                auto start = std::chrono::high_resolution_clock::now();
                auto object = evalProgram(program.get(), environment);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end - start;
                cumulativeTime += elapsed.count();
                if (object->type == obj::ObjectType::Exit)
                {
                    auto exitObj = dynamic_cast<obj::Exit *>(object.get());
                    if (!exitObj)
                        returnValue = -1;
                    else
                        returnValue = exitObj->value;
                }
                else
                {
                    if (object->type == obj::ObjectType::Error)
                        std::cerr << util::color::colorize(object->inspect(), util::color::fg::red) << std::endl;
                    else if (object->type != obj::ObjectType::Null)
                        std::cout << object->inspect() << std::endl;
                    if (object->type == obj::ObjectType::Error)
                        returnValue = 1;
                    else
                        returnValue = 0;
                }
            }
        }
    }

    if (enterInteractive)
    {
        returnValue = interactiveMode(environment);
    }

    environment.reset();
    finalize();

    if (showStatistics)
    {
        std::cout << "Object statistics:" << std::endl;
        std::cout << " created: " << obj::Object::instancesConstructed << ", destructed: " << obj::Object::instancesDestructed << std::endl;
        std::cout << " user objects wrongly destructed: " << obj::UserObject::userInstancesWronglyDestructed << std::endl;
        std::cout << "Environment statistics:" << std::endl;
        std::cout << " created: " << obj::Environment::instancesConstructed << ", destructed: " << obj::Environment::instancesDestructed << std::endl;
        std::cout << "Usertime: " << cumulativeTime << "ms" << std::endl;
    }

    return returnValue;
}
catch (std::runtime_error &e)
{
    std::cerr << util::color::colorize(e.what(), util::color::fg::red);
    return -1;
}