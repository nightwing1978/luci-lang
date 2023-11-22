#include "Lexer.h"

#include <stdexcept>
#include <ctype.h>
#include <map>
#include <unordered_map>
#include <vector>

Token newToken(const TokenType &type, const char ch, size_t lineNumber, size_t columnNumber, const std::shared_ptr<std::string> &fileName)
{
    Token tok;
    tok.fileName = fileName;
    tok.type = type;
    tok.literal = ch;
    tok.lineNumber = lineNumber;
    tok.columnNumber = columnNumber;
    return tok;
}

Token newToken(const TokenType &type, const std::string &literal, size_t lineNumber, size_t columnNumber, const std::shared_ptr<std::string> &fileName)
{
    Token tok;
    tok.fileName = fileName;
    tok.type = type;
    tok.literal = literal;
    tok.lineNumber = lineNumber;
    tok.columnNumber = columnNumber;
    return tok;
}

char peekChar(Lexer &lexer)
{
    if (lexer.readPosition >= lexer.input.size())
        return '\0';
    return lexer.input[lexer.readPosition];
}

void readChar(Lexer &lexer)
{
    if (lexer.readPosition >= lexer.input.size())
    {
        lexer.ch = 0;
    }
    else
    {
        lexer.ch = lexer.input[lexer.readPosition];
    }
    lexer.position = lexer.readPosition;
    lexer.readPosition += 1;
    lexer.columnNumber += 1;
}

namespace
{
    std::vector<std::pair<TokenType, std::string>> tokenToKeyword = {
        {TokenType::TYPE, "type"},
        {TokenType::FUNCTION, "fn"},
        {TokenType::LET, "let"},
        {TokenType::IMPORT, "import"},
        {TokenType::SCOPE, "scope"},
        {TokenType::IF, "if"},
        {TokenType::ELSE, "else"},
        {TokenType::RETURN, "return"},
        {TokenType::TRUE, "true"},
        {TokenType::FALSE, "false"},
        {TokenType::NULL_T, "null"},
        {TokenType::WHILE, "while"},
        {TokenType::BREAK, "break"},
        {TokenType::CONST, "const"},
        {TokenType::ANY, "any"},
        {TokenType::ALL, "all"},
        {TokenType::OPERATOR, "op"},
        {TokenType::IN, "in"},
        {TokenType::FOR, "for"},
        {TokenType::TRY, "try"},
        {TokenType::EXCEPT, "except"},
    };
}

std::string keyword(TokenType tokenType)
{
    static std::map<TokenType, std::string> keywords;
    if (keywords.empty())
    {
        for (auto [token, keyword] : tokenToKeyword)
            keywords.emplace(token, keyword);
    }
    return keywords.at(tokenType);
}

TokenType lookupIdent(const std::string &ident)
{
    static std::map<std::string, TokenType> keywords;
    if (keywords.empty())
    {
        for (auto [token, keyword] : tokenToKeyword)
            keywords.emplace(keyword, token);
    }

    const auto identF = keywords.find(ident);
    if (identF == keywords.end())
        return TokenType::IDENT;
    return identF->second;
}

std::string readIdentifier(Lexer &lexer)
{
    size_t position = lexer.position;
    while (isalpha(lexer.ch) || isdigit(lexer.ch) || lexer.ch == '_')
    {
        readChar(lexer);
    }
    return lexer.input.substr(position, lexer.position - position);
}

std::string readNumber(Lexer &lexer)
{
    size_t position = lexer.position;
    while (isdigit(lexer.ch))
    {
        readChar(lexer);
    }
    return lexer.input.substr(position, lexer.position - position);
}

std::string unEscape(const std::string &input)
{
    // basic unescaping of \n and \t
    std::string output;
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] != '\\')
        {
            output.push_back(input[i]);
        }
        else
        {
            if (i == input.size() - 1)
            {
                // at the end of the input string, this is an error condition, but we will let it slip for now
                output.push_back(input[i]);
            }
            else
            {
                auto nextChar = input[i + 1];
                if (nextChar == 'n')
                {
                    output.push_back('\n');
                    ++i;
                }
                else if (nextChar == 't')
                {
                    output.push_back('\t');
                    ++i;
                }
                else if (nextChar == 'r')
                {
                    output.push_back('\r');
                    ++i;
                }
                else if (nextChar == '0')
                {
                    // octal notation of escaped character, there must be another two digits
                    // following or this is an error
                    if (i > input.size() - 4)
                    {
                        // make this an error condition
                    }
                    else
                    {
                        auto nextChar2 = input[i + 2];
                        auto nextChar3 = input[i + 3];

                        auto octal = (nextChar - '0') * 64 + (nextChar2 - '0') * 8 + (nextChar3 - '0');
                        if (octal >= 0 && octal <= 255)
                        {
                            output.push_back(static_cast<char>(octal));
                        }

                        i += 3;
                    }
                }
            }
        }
    }

    return output;
}

Token readStringToken(Lexer &lexer)
{
    Token token;
    size_t position = lexer.position;

    while (peekChar(lexer) != '"' && peekChar(lexer) != 0)
    {
        readChar(lexer);
    }
    if (peekChar(lexer) == 0)
    {
        readChar(lexer);
        return newToken(TokenType::ILLEGAL, unEscape(lexer.input.substr(position, lexer.position - position)), lexer.lineNumber, lexer.columnNumber, lexer.fileName);
    }
    readChar(lexer);
    readChar(lexer);
    token.literal = unEscape(lexer.input.substr(position, lexer.position - position));
    token.type = TokenType::STRING;
    return token;
}

Token readSingleLineCommentToken(Lexer &lexer)
{
    Token token;
    size_t position = lexer.position;

    while (peekChar(lexer) != '\n' && peekChar(lexer) != 0)
    {
        readChar(lexer);
    }
    if (peekChar(lexer) == '\n')
    {
        readChar(lexer);
    }
    token.literal = lexer.input.substr(position, lexer.position - position);
    token.type = TokenType::COMMENT;
    token.fileName = lexer.fileName;
    return token;
}

Token readSingleLineDocToken(Lexer &lexer)
{
    Token token;
    size_t position = lexer.position;

    while (peekChar(lexer) != '\n' && peekChar(lexer) != 0)
    {
        readChar(lexer);
    }
    if (peekChar(lexer) == '\n')
    {
        readChar(lexer);
    }
    token.literal = lexer.input.substr(position, lexer.position - position);
    token.type = TokenType::DOC;
    return token;
}

Token readNumberToken(Lexer &lexer)
{
    Token token;

    size_t position = lexer.position;
    while (isdigit(lexer.ch))
    {
        readChar(lexer);
    }
    if (lexer.ch == '.')
    {
        readChar(lexer);
        token.type = TokenType::DOUBLE;
        while (isdigit(lexer.ch))
        {
            readChar(lexer);
        }
        if (lexer.ch == 'e')
        {
            // scientific notation ahead
            readChar(lexer);
            if (lexer.ch == '-' || lexer.ch == '+')
            {
                readChar(lexer);
            }
            if (!isdigit(lexer.ch))
            {
                token.literal = lexer.input.substr(position, lexer.position - position);
                token.type = TokenType::ILLEGAL;
                return token;
            }
            while (isdigit(lexer.ch))
            {
                readChar(lexer);
            }
        }
        token.literal = lexer.input.substr(position, lexer.position - position);
    }
    else
    {
        token.type = TokenType::INT;
        token.literal = lexer.input.substr(position, lexer.position - position);
    }
    return token;
}

void skipWhitespace(Lexer &lexer)
{
    while ((lexer.ch == ' ') || (lexer.ch == '\t') || (lexer.ch == '\n') || (lexer.ch == '\r'))
    {
        if (lexer.ch == '\n')
        {
            lexer.lineNumber += 1;
            lexer.columnNumber = 0;
        }
        readChar(lexer);
    }
}

std::unique_ptr<Lexer> createLexer(const std::string &input, const std::string &fileName)
{
    std::unique_ptr<Lexer> lexer = std::make_unique<Lexer>();
    lexer->input = input;
    lexer->fileName = std::make_shared<std::string>(fileName);
    readChar(*lexer);
    return lexer;
}

Token nextToken(Lexer &lexer)
{
    Token token;

    skipWhitespace(lexer);

    switch (lexer.ch)
    {
    case '=':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::EQ, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        else
        {
            token = newToken(TokenType::ASSIGN, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case ';':
        token = newToken(TokenType::SEMICOLON, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '(':
        token = newToken(TokenType::LPAREN, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case ')':
        token = newToken(TokenType::RPAREN, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '[':
        token = newToken(TokenType::LBRACKET, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case ']':
        token = newToken(TokenType::RBRACKET, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case ',':
        token = newToken(TokenType::COMMA, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '!':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::N_EQ, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        else
        {
            token = newToken(TokenType::BANG, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case '+':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::PLUSASSIGN, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        token = newToken(TokenType::PLUS, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '-':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::MINUSASSIGN, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        if (peekChar(lexer) == '>')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::ARROW, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        token = newToken(TokenType::MINUS, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '/':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::SLASHASSIGN, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        else if (peekChar(lexer) == '/')
        {
            const char ch = lexer.ch;
            token = readSingleLineCommentToken(lexer);
            break;
        }
        else if (peekChar(lexer) == '!')
        {
            const char ch = lexer.ch;
            token = readSingleLineDocToken(lexer);
            break;
        }
        token = newToken(TokenType::SLASH, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '*':
        if (peekChar(lexer) == '*')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::DOUBLEASTERISK, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        else if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::ASTERISKASSIGN, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        else
        {
            token = newToken(TokenType::ASTERISK, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case '%':
        token = newToken(TokenType::PERCENT, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '|':
        if (peekChar(lexer) == '|')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::DOUBLEPIPE, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        else
        {
            token = newToken(TokenType::ILLEGAL, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case '&':
        if (peekChar(lexer) == '&')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::DOUBLEAMPERSAND, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        else
        {
            token = newToken(TokenType::ILLEGAL, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case ':':
        if (peekChar(lexer) == ':')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::DOUBLECOLON, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
        else
        {
            token = newToken(TokenType::COLON, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case '<':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::LTEQ, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        else
        {
            token = newToken(TokenType::LT, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case '>':
        if (peekChar(lexer) == '=')
        {
            const char ch = lexer.ch;
            readChar(lexer);
            std::string fullLiteral;
            fullLiteral.push_back(ch);
            fullLiteral.push_back(lexer.ch);
            token = newToken(TokenType::GTEQ, fullLiteral, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        else
        {
            token = newToken(TokenType::GT, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        }
        break;
    case '{':
        token = newToken(TokenType::LBRACE, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '}':
        token = newToken(TokenType::RBRACE, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case '"':
        token.lineNumber = lexer.lineNumber;
        token.columnNumber = lexer.columnNumber;
        token = readStringToken(lexer);
        token.fileName = lexer.fileName;
        return token;
    case '.':
        token = newToken(TokenType::DOT, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        break;
    case 0:
        token = newToken(TokenType::EOF_T, '\0', lexer.lineNumber, lexer.columnNumber, lexer.fileName);
        token.literal = "";
        break;
    default:
        if (isalpha(lexer.ch) || lexer.ch == '_')
        {
            token.lineNumber = lexer.lineNumber;
            token.columnNumber = lexer.columnNumber;
            token.literal = readIdentifier(lexer);
            token.type = lookupIdent(token.literal);
            token.fileName = lexer.fileName;
            return token;
        }
        else if (isdigit(lexer.ch))
        {
            token = readNumberToken(lexer);
            token.lineNumber = lexer.lineNumber;
            token.columnNumber = lexer.columnNumber;
            token.fileName = lexer.fileName;
            return token;
        }
        else
        {
            token = newToken(TokenType::ILLEGAL, lexer.ch, lexer.lineNumber, lexer.columnNumber, lexer.fileName);
            break;
        }
    }

    readChar(lexer);
    return token;
}