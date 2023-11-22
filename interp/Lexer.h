#ifndef GUARDIAN_OF_INCLUSION_LEXER_H
#define GUARDIAN_OF_INCLUSION_LEXER_H

#include <string>
#include <memory>
#include "Token.h"

struct Lexer
{
    std::shared_ptr<std::string> fileName;
    std::string input;
    size_t position = 0;     /**< points to current character */
    size_t readPosition = 0; /**< points to reading position in input (after current char) */
    size_t lineNumber = 1;
    size_t columnNumber = 0;
    char ch = 0; /**< current character */
};

std::unique_ptr<Lexer> createLexer(const std::string &input, const std::string &fileName);
void readChar(Lexer &lexer);
Token nextToken(Lexer &lexer);

std::string keyword(TokenType tokenType); /**< returns the language keyword for the token type */

#endif