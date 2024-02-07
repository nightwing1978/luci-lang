cmake_minimum_required (VERSION 3.8)

add_library(UtilLib 
    "Util.h"
    "Util.cpp"
)
add_library(LexLib 
    "Token.h"
    "Token.cpp"
    "Lexer.h"
    "Lexer.cpp"
)
add_library(ParseLib 
    "Ast.h"
    "Ast.cpp"
    "Parser.h"
    "Parser.cpp"
)


add_executable (LexerTest 
    "LexerTest.cpp"
)
target_link_libraries(LexerTest LexLib UtilLib)


add_executable (ParserTest 
    "ParserTest.cpp" 
)
target_link_libraries(ParserTest ParseLib LexLib UtilLib)


add_library(luciLib 
    "Object.h"
    "Object.cpp"
    "Evaluator.h"
    "Evaluator.cpp"
    "Version.h"
    "Version.cpp"
    "Typing.cpp"
    "Typing.h"
    "builtin/Array.h"
    "builtin/Array.cpp"
    "builtin/Dictionary.h"
    "builtin/Dictionary.cpp"
    "builtin/Error.h"
    "builtin/Error.cpp"
    "builtin/ErrorType.h"
    "builtin/ErrorType.cpp"
    "builtin/IO.h"
    "builtin/IO.cpp"
    "builtin/Json.h"
    "builtin/Json.cpp"
    "builtin/Math.h"
    "builtin/Math.cpp"
    "builtin/OS.h"
    "builtin/OS.cpp"
    "builtin/Regex.h"
    "builtin/Regex.cpp"
    "builtin/Set.h"
    "builtin/Set.cpp"
    "builtin/String.h"
    "builtin/String.cpp"
    "builtin/Freeze.h"
    "builtin/Freeze.cpp"
    "builtin/Time.h"
    "builtin/Time.cpp"
    "builtin/Thread.h"
    "builtin/Thread.cpp"
    "builtin/Threading.h"
    "builtin/Threading.cpp"
    "builtin/Typing.h"
    "builtin/Typing.cpp"
    "format/Format.h"
    "format/Format.cpp"
)


add_executable (EvalTest 
    "EvalTest.cpp"
)
target_link_libraries(EvalTest luciLib ParseLib LexLib UtilLib)


add_executable (Lex "Lex.cpp")
target_link_libraries(Lex LexLib UtilLib)


add_executable (Parse "Parse.cpp")
target_link_libraries(Parse ParseLib LexLib UtilLib)


add_executable (luci 
    "Interp.cpp" 
)
target_link_libraries(luci luciLib ParseLib LexLib UtilLib)

add_executable (luci_analyze 
    "Analyze.cpp" 
)
target_link_libraries(luci_analyze luciLib ParseLib LexLib UtilLib)