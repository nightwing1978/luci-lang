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

		auto lexer = createLexer(text,"");
		for (auto token = nextToken(*lexer); token.type != TokenType::EOF_T; token = nextToken(*lexer))
		{
			std::cout << token.literal << " -> " << toString(token.type) << std::endl;
		}
	
	};

	return 0;
}