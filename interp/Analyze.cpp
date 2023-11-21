#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>

#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "Util.h"


void linearizeAstStatementTree(ast::Node* node, std::vector<ast::Node*>& nodes)
{
    nodes.push_back(node);
    switch (node->type)
    {
        case ast::NodeType::Program:
            {
                auto program = static_cast<ast::Program*>(node);
                for (const auto& statement : program->statements)
                    linearizeAstStatementTree(statement.get(), nodes);
            }
            break;
        case ast::NodeType::BlockStatement:
            {
                auto block = static_cast<ast::BlockStatement*>(node);
                for (const auto& statement : block->statements)
                    linearizeAstStatementTree(statement.get(), nodes);
            }
            break;
    }
}


void analyzeProgram(ast::Program* program)
{
    const int maxSizeWithEllipsis = 60;
    std::vector<ast::Node*> nodes;
    linearizeAstStatementTree(program, nodes);
    for (const auto& node : nodes)
    {
        if (node->type == ast::NodeType::LetStatement)
        {
            std::cout << "LetStatement: " << util::withEllipsis(node->text(), maxSizeWithEllipsis) << std::endl;
            auto letStatement = static_cast<ast::LetStatement*>(node);
            if (letStatement->type)
            {
                std::cout << "  " << util::withEllipsis(letStatement->type->text(), maxSizeWithEllipsis) << std::endl;
            }
            else
            {
                std::cout << "  no type info provided" << std::endl;
            }
            if (letStatement->value && letStatement->value)
            {
                std::cout << "  value-type=" << static_cast<int>(letStatement->value->type) << std::endl;
            }
        }
    }
}


void usage(int argc, char* argv)
{
	std::cout << argv[0] << "\n";
	std::cout << "Usage: \n";
	std::cout << argv[0] << " file_name\n";
	std::cout << "  file_name	analyze the given file_name\n";
}


int main(int argc, char** argv)
try
{
	std::string fileToAnalyze = "";

	int returnValue = 2;
	if (argc == 2)
	{
		fileToAnalyze = std::string(argv[1]);
	}

    std::cout << "Analyzing " << fileToAnalyze << std::endl;

	if (!fileToAnalyze.empty())
	{
		std::string text;
		std::string line;
		std::ifstream inputf;
		inputf.open(fileToAnalyze);
		if (inputf.is_open())
		{
			while (std::getline(inputf, line))
			{
				text += line + "\n";
			}
		}
		else
		{
			std::cerr << "File " << fileToAnalyze << " cannot be read" << std::endl;
			returnValue = 2;
		}
		auto lexer = createLexer(text,fileToAnalyze);
		auto parser = createParser(std::move(lexer));
		auto program = parser->parseProgram();

		if (!parser->errorMsgs.empty())
		{
			std::stringstream ss;
			for (const auto& msg : parser->errorMsgs)
				ss << msg << std::endl;
            std::cerr << util::color::colorize(ss.str(), util::color::fg::red);
		}
		else
		{
			if (program)
			{
                analyzeProgram(program.get());
                returnValue = 0;
			}
            else
            {
                std::cerr << "Could not create program tree" << std::endl;
            }
		}
    }
    else
    {
        std::cerr << "No file specified" << std::endl;
    }
	return returnValue;
}
catch (std::runtime_error& e)
{
    std::cerr << util::color::colorize(e.what(), util::color::fg::red);
    return -1;
}