#include <iostream>
#include <map>
#include <vector>
#include "Token.h"
#include "Lexer.h"


void testNextToken()
{
	std::string input = "=+(){},;";

	std::vector< std::pair<TokenType, std::string> > expected = {
		{ TokenType::ASSIGN, "=" },
		{ TokenType::PLUS, "+" },
		{ TokenType::LPAREN, "(" },
		{ TokenType::RPAREN, ")" },
		{ TokenType::LBRACE, "{" },
		{ TokenType::RBRACE, "}" },
		{ TokenType::COMMA, "," },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::EOF_T, "" },
	};

	auto lexer = createLexer(input,"");

	for (const auto& [tt, li] : expected)
	{
		auto tok = nextToken(*lexer);
		if (tok.type != tt)
			throw std::runtime_error("Unexpected token, expected " + toString(tt) + " but got " + toString(tok.type));
		if (tok.literal != li)
			throw std::runtime_error("Unexpected literal, expected " + li + " but got " + tok.literal);
	}
}


void testDouble()
{
	std::string input = "3.0;3.0e1;3.0e-1;+5.0;3.0e+3;3.0e--3;";

	std::vector< std::pair<TokenType, std::string> > expected = {
		{ TokenType::DOUBLE, "3.0" },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::DOUBLE, "3.0e1" },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::DOUBLE, "3.0e-1" },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::PLUS, "+" },
		{ TokenType::DOUBLE, "5.0" },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::DOUBLE, "3.0e+3" },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::ILLEGAL, "3.0e-" },
		{ TokenType::MINUS, "-" },
		{ TokenType::INT, "3" },
		{ TokenType::SEMICOLON, ";" },
		{ TokenType::EOF_T, "" },
	};

	auto lexer = createLexer(input,"");

	for (const auto& [tt, li] : expected)
	{
		auto tok = nextToken(*lexer);
		if (tok.type != tt)
			throw std::runtime_error("Unexpected token, expected " + toString(tt) + " but got " + toString(tok.type));
		if (tok.literal != li)
			throw std::runtime_error("Unexpected literal, expected " + li + " but got " + tok.literal);
	}
}


void testNextToken2()
{
	std::string input = "let five = 5; \
let ten = 10;\
let add = fn(x, y) {\
	x + y;\
};\
let result = add(five, ten); ";

	std::vector< std::pair<TokenType, std::string> > expected = {
		{TokenType::LET, "let"},
		{TokenType::IDENT, "five"},
		{TokenType::ASSIGN, "="},
		{TokenType::INT, "5"},
		{TokenType::SEMICOLON, ";"},
		{TokenType::LET, "let"},
		{TokenType::IDENT, "ten"},
		{TokenType::ASSIGN, "="},
		{TokenType::INT, "10"},
		{TokenType::SEMICOLON, ";"},
		{TokenType::LET, "let"},
		{TokenType::IDENT, "add"},
		{TokenType::ASSIGN, "="},
		{TokenType::FUNCTION, "fn"},
		{TokenType::LPAREN, "("},
		{TokenType::IDENT, "x"},
		{TokenType::COMMA, ","},
		{TokenType::IDENT, "y"},
		{TokenType::RPAREN, ")"},
		{TokenType::LBRACE, "{"},
		{TokenType::IDENT, "x"},
		{TokenType::PLUS, "+"},
		{TokenType::IDENT, "y"},
		{TokenType::SEMICOLON, ";"},
		{TokenType::RBRACE, "}"},
		{TokenType::SEMICOLON, ";"},
		{TokenType::LET, "let"},
		{TokenType::IDENT, "result"},
		{TokenType::ASSIGN, "="},
		{TokenType::IDENT, "add"},
		{TokenType::LPAREN, "("},
		{TokenType::IDENT, "five"},
		{TokenType::COMMA, ","},
		{TokenType::IDENT, "ten"},
		{TokenType::RPAREN, ")"},
		{TokenType::SEMICOLON, ";"},
		{TokenType::EOF_T, ""}
	};

	auto lexer = createLexer(input,"");

	for (const auto& [tt, li] : expected)
	{
		auto tok = nextToken(*lexer);
		if (tok.type != tt)
			throw std::runtime_error("Unexpected token, expected " + toString(tt) + " but got " + toString(tok.type));
		if (tok.literal != li)
			throw std::runtime_error("Unexpected literal, expected " + li + " but got " + tok.literal);
	}
}

int main()
{
	try
	{
		testNextToken();
		testNextToken2();
		testDouble();
		std::cerr << "All tests passed" << std::endl;
		return 0;
	}
	catch (std::runtime_error& e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}
}