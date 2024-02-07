/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_OBJECT_H
#define GUARDIAN_OF_INCLUSION_OBJECT_H

#include "Ast.h"
#include <chrono>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <atomic>
#include <regex>
#include <thread>

namespace obj
{
    enum class ObjectType
    {
        Unknown = -1,
        Null = 0,
        Integer = 1,
        Double = 2,
        Complex = 3,
        Boolean = 4,
        String = 5,
        Function = 6,
        Error = 7,
        ReturnValue = 8,
        Builtin = 9,
        BreakValue = 10,
        Array = 11,
        Dictionary = 12,
        Set = 13,
        Exit = 14,
        UserType = 15,
        BuiltinType = 16,
        BoundBuiltinTypeFunction = 17,
        BoundBuiltinTypeProperty = 18,
        BoundUserTypeFunction = 19,
        BoundUserTypeProperty = 20,
        UserObject = 21,
        IOObject = 22,
        ArrayDouble = 23,
        ArrayComplex = 24,
        Iterator = 25,
        Char = 26,
        Freezer = 27,
        Module = 28,
        Thread = 29,
        Range = 30,
        Regex = 31,
        ContinueValue = 32,
        Clock = 33,
        TimePoint = 34,
    };

    std::string toString(const ObjectType &type);

    struct Object
    {
        static std::atomic_int instancesConstructed;
        static std::atomic_int instancesDestructed;

        int frozen = 0; /*< when frozen larger than 0 no updates allowed to object */
        ObjectType type;
        ast::TypeExpression *declaredType = nullptr; /*< objects that have a declared type will carry non-nullptrs */
        virtual std::string inspect() const;
        virtual std::size_t hash() const;
        virtual bool hashAble() const;
        virtual bool eq(const Object *other) const;
        virtual std::shared_ptr<Object> clone() const;
        Object(ObjectType itype = ObjectType::Unknown);
        virtual ~Object();

        void *operator new(size_t size);
        void operator delete(void *ptr);
    };

    struct ObjectFreezer : public Object
    {
        std::shared_ptr<Object> obj;
        ObjectFreezer(std::shared_ptr<Object> iobj) : Object(ObjectType::Freezer), obj(iobj) { ++obj->frozen; }
        ~ObjectFreezer() { --obj->frozen; }
    };

    enum class ErrorType : int
    {
        UndefinedError = 0,          /* */
        TypeError = 1,               /* wrong number of arguments, wrong arguments */
        ConstError = 2,              /* trying to modify a variable marked as const */
        IdentifierNotFound = 3,      /* trying to access an identifier that is not found */
        IdentifierAlreadyExists = 4, /* trying to redefine an identifier */
        ValueError = 5,              /* result of operation generated an invalid value, say when casting to a particular type */
        KeyError = 6,                /* trying to access a key in a dict that is not present */
        IndexError = 7,              /* trying to access an index out of range */
        ImportError = 8,             /* trying to import a module that didn't exist or gave problems */
        SyntaxError = 9,             /* trying to dynamically run a program gave a syntax/parsing error */
        OSError = 10,                /* error related to the OS or file system */
    };

    struct Error : public Object
    {
        Token token;
        std::string msg;
        ErrorType errorType = ErrorType::UndefinedError;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<Error>(msg, errorType, token); };
        Error(const std::string &imsg, ErrorType iErrorType) : Object(ObjectType::Error), msg(imsg), errorType(iErrorType){};
        Error(const std::string &imsg, ErrorType iErrorType, Token itoken) : Object(ObjectType::Error), msg(imsg), errorType(iErrorType), token(itoken){};
    };

    // return a TypeError object
    std::shared_ptr<Error> makeTypeError(const std::string &msg);

    struct Environment
    {
        static std::atomic_int instancesConstructed;
        static std::atomic_int instancesDestructed;

        Environment();
        ~Environment();

        std::shared_ptr<Environment> outer;
        struct TTokenSharedObj
        {
            std::shared_ptr<Object> obj;
            bool constant;
            ast::TypeExpression *type;
        };
        std::unordered_map<std::string, TTokenSharedObj> store;

        bool has(const std::string &) const;
        std::shared_ptr<Object> get(const std::string &) const;
        ast::TypeExpression *getType(const std::string &) const;
        std::shared_ptr<Object> set(const std::string &, std::shared_ptr<Object> value);
        std::shared_ptr<Object> add(const std::string &, std::shared_ptr<Object> value, bool constant, ast::TypeExpression *type);
    };

    enum class ModuleState
    {
        Unknown = -1,
        Loaded = 1,  /*< a module has been imported and all variables and functions are available */
        Defined = 2, /*< a module only serves as a path to reach into a submodule and no functions nor variables of it are available */
    };

    struct Module : public Object
    {
        ModuleState state = obj::ModuleState::Unknown;
        std::shared_ptr<Environment> environment;
        std::string fileName;

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Module>(); };
        virtual std::size_t hash() const override { return 0; };
        virtual bool hashAble() const override { return false; };
        virtual bool eq(const Object *other) const override { return false; };

        Module();
    };

    struct Null : public Object
    {
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Null>(); };
        virtual std::size_t hash() const override { return 0; };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return true; };

        Null() : Object(ObjectType::Null){};
    };

    struct Integer : public Object
    {
        int64_t value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Integer>(value); };
        virtual std::size_t hash() const override { return std::hash<int64_t>{}(value); };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return static_cast<const obj::Integer *>(other)->value == value; };
        Integer(int64_t ivalue) : Object(ObjectType::Integer), value(ivalue){};
        virtual ~Integer();
    };

    struct Double : public Object
    {
        double value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Double>(value); };
        virtual std::size_t hash() const override { return std::hash<double>{}(value); };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return static_cast<const obj::Double *>(other)->value == value; };
        Double(double ivalue) : Object(ObjectType::Double), value(ivalue){};
    };

    struct Complex : public Object
    {
        std::complex<double> value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Complex>(value); };
        virtual std::size_t hash() const override { return std::hash<double>{}(value.real()) ^ std::hash<double>{}(value.imag()); };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return static_cast<const obj::Complex *>(other)->value == value; };
        Complex(const std::complex<double> &ivalue) : Object(ObjectType::Complex), value(ivalue){};
    };

    struct Boolean : public Object
    {
        bool value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Boolean>(value); };
        virtual std::size_t hash() const override { return std::hash<bool>{}(value); };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return static_cast<const obj::Boolean *>(other)->value == value; };
        Boolean(bool ivalue) : Object(ObjectType::Boolean), value(ivalue){};
    };

    struct Char : public Object
    {
        int value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::Char>(value); };
        virtual std::size_t hash() const override { return std::hash<int>{}(value); };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return static_cast<const obj::Char *>(other)->value == value; };
        Char(int ivalue) : Object(ObjectType::Char), value(ivalue){};
    };

    struct String : public Object
    {
        std::string value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<obj::String>(value); };
        virtual std::size_t hash() const override { return std::hash<std::string>{}(value); };
        virtual bool hashAble() const override { return true; };
        virtual bool eq(const Object *other) const override { return static_cast<const obj::String *>(other)->value == value; };

        String(const std::string &ivalue) : Object(ObjectType::String), value(ivalue){};
        virtual ~String();
    };

    struct Range : public Object
    {
        int64_t lower;
        int64_t upper;
        int64_t stride;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<obj::Range>(lower, upper, stride);
        }
        virtual bool eq(const Object *other) const override;
        int64_t length() const;
        std::vector<int64_t> values() const;
        Range() : Object(ObjectType::Range), lower(0), upper(0), stride(1){};
        Range(int64_t iLower, int64_t iUpper, int64_t iStride) : Object(ObjectType::Range), lower(iLower), upper(iUpper), stride(iStride){};
    };

    struct Iterator : public Object
    {
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<obj::Iterator>();
        }
        virtual bool isValid() const { return false; };
        virtual std::shared_ptr<Object> next() { return std::make_shared<obj::Object>(); }
        Iterator() : Object(ObjectType::Iterator){};
    };

    struct ArrayDouble : public Object
    {
        std::vector<double> value;
        static std::shared_ptr<Object> valueConstruct(const double &value);

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool eq(const Object *other) const;
        ArrayDouble(const std::vector<double> &ivalue);
    };

    struct ArrayComplex : public Object
    {
        std::vector<std::complex<double>> value;
        static std::shared_ptr<Object> valueConstruct(const std::complex<double> &value);

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool eq(const Object *other) const;
        ArrayComplex(const std::vector<std::complex<double>> &ivalue);
    };

    struct Array : public Object
    {
        std::vector<std::shared_ptr<Object>> value;
        static std::shared_ptr<obj::Object> valueConstruct(std::shared_ptr<Object> obj);

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool eq(const Object *other) const override;
        virtual bool hashAble() const override;
        virtual std::size_t hash() const override;

        Array(const std::vector<std::shared_ptr<Object>> &ivalue);
        Array(const std::vector<double> &ivalue);
        Array(const std::vector<std::complex<double>> &ivalue);
    };

    template <typename TArrayType>
    struct ArrayIterator : public Iterator
    {
        std::shared_ptr<TArrayType> array;
        ObjectFreezer freezer;
        size_t index = 0;

        virtual std::string inspect() const override
        {
            return "ArrayIterator()";
        }
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<obj::ArrayIterator<TArrayType>>(array, index);
        }
        virtual bool isValid() const override
        {
            return index < array->value.size();
        };
        virtual std::shared_ptr<Object> next() override
        {
            if (isValid())
            {
                ++index;
                return TArrayType::valueConstruct(array->value[index - 1]);
            }
            return std::make_shared<obj::Error>("next referencing invalid iterator", obj::ErrorType::TypeError);
        }
        ArrayIterator(std::shared_ptr<TArrayType> iarray, size_t iindex) : Iterator(), array(iarray), freezer(iarray), index(iindex){};
    };

    struct RangeIterator : public Iterator
    {
        std::shared_ptr<Range> rangeObj;
        int64_t current;

        virtual std::string inspect() const override
        {
            return "RangeIterator()";
        }
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<RangeIterator>(rangeObj, current);
        }
        virtual bool isValid() const
        {
            return current < rangeObj->upper;
        };
        virtual std::shared_ptr<Object> next() override
        {
            if (isValid())
            {
                int64_t currentValue = current;
                current += rangeObj->stride;
                return std::make_shared<Integer>(currentValue);
            }
            return std::make_shared<obj::Error>("next referencing invalid iterator", obj::ErrorType::TypeError);
        }
        RangeIterator(std::shared_ptr<Range> irange, int64_t icurrent) : Iterator(), rangeObj(irange), current(icurrent){};
    };

    struct StringIterator : public Iterator
    {
        std::shared_ptr<String> stringObj;
        ObjectFreezer freezer;
        size_t index;

        virtual std::string inspect() const override
        {
            return "StringIterator()";
        }
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<StringIterator>(stringObj, index);
        }
        virtual bool isValid() const override
        {
            return index < stringObj->value.size();
        };
        virtual std::shared_ptr<Object> next() override
        {
            if (isValid())
            {
                ++index;
                std::string charStr = std::string("") + stringObj->value[index - 1];
                return std::make_shared<String>(String(charStr));
            }
            return std::make_shared<obj::Error>("next referencing invalid iterator", obj::ErrorType::TypeError);
        }
        StringIterator(std::shared_ptr<String> istring, size_t iindex) : Iterator(), stringObj(istring), freezer(istring), index(iindex){};
    };

    struct Hash
    {
        std::size_t operator()(const std::shared_ptr<Object> &a) const
        {
            if (!a->hashAble())
                throw std::runtime_error("trying to hash an unhashable type");
            return a->hash();
        }
    };

    struct Equal
    {
        bool operator()(const std::shared_ptr<Object> &a, const std::shared_ptr<Object> &b) const
        {
            if (a.get() == b.get())
                return true;

            if (a->type != b->type)
                return false;

            return a->eq(b.get());
        }
    };

    typedef std::unordered_map<std::shared_ptr<Object>, std::shared_ptr<Object>, obj::Hash, obj::Equal> TDictionaryMap;
    struct Dictionary : public Object
    {
        TDictionaryMap value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool hashAble() const override;
        virtual std::size_t hash() const override;
        virtual bool eq(const Object *other) const override;
        Dictionary(const TDictionaryMap &ivalue);
    };

    struct DictionaryIterator : public Iterator
    {
        std::shared_ptr<Dictionary> dict;
        ObjectFreezer freezer;
        TDictionaryMap::iterator iterator;

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool isValid() const override;
        virtual std::shared_ptr<Object> next() override;
        DictionaryIterator(std::shared_ptr<Dictionary> idict, TDictionaryMap::iterator iiterator);
    };

    typedef std::unordered_set<std::shared_ptr<Object>, obj::Hash, obj::Equal> TSetSet;
    struct Set : public Object
    {
        TSetSet value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool hashAble() const override;
        virtual std::size_t hash() const override;
        virtual bool eq(const Object *other) const override;
        Set(const TSetSet &ivalue);
    };

    struct SetIterator : public Iterator
    {
        std::shared_ptr<Set> setObj;
        ObjectFreezer freezer;
        TSetSet::iterator iterator;

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override;
        virtual bool isValid() const override;
        virtual std::shared_ptr<Object> next() override;
        SetIterator(std::shared_ptr<Set> iset, TSetSet::iterator iiterator);
    };

    struct Function : public Object
    {
        std::string doc;
        std::vector<ast::Identifier> arguments;
        std::vector<ast::TypeExpression *> argumentTypes;
        ast::TypeExpression *returnType = 0;
        ast::BlockStatement *body = 0;
        std::shared_ptr<obj::Environment> environment;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override
        {
            auto func = std::make_shared<obj::Function>();
            func->arguments = arguments;
            func->argumentTypes = argumentTypes;
            func->returnType = returnType;
            func->body = body;
            func->environment = environment;
            return func;
        };
        Function() : Object(ObjectType::Function) {}
    };

    typedef std::shared_ptr<obj::Object> (*TBuiltinFunction)(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment);
    struct Builtin : public Object
    {
        TBuiltinFunction function;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<Builtin>(function); };
        Builtin(TBuiltinFunction ifunction = nullptr) : Object(ObjectType::Builtin), function(ifunction) {}
    };

    struct ReturnValue : public Object
    {
        std::shared_ptr<Object> value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<ReturnValue>(value->clone()); };
        ReturnValue(std::shared_ptr<Object> ivalue) : Object(ObjectType::ReturnValue), value(std::move(ivalue)){};
    };

    struct BreakValue : public Object
    {
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<BreakValue>(); };
        BreakValue() : Object(ObjectType::BreakValue){};
    };

    struct ContinueValue : public Object
    {
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<ContinueValue>(); };
        ContinueValue() : Object(ObjectType::ContinueValue){};
    };

    struct Exit : public Object
    {
        Token token;
        int value;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<Exit>(value); };
        Exit(int ivalue) : Object(ObjectType::Exit), value(ivalue){};
        Exit(int ivalue, Token itoken) : Object(ObjectType::Exit), value(ivalue), token(itoken){};
    };

    typedef std::shared_ptr<obj::Object> (*TBuiltinTypeFunction)(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments);
    struct TBuiltinTypeFunctionDefinition
    {
        TBuiltinTypeFunction function;
        ast::TypeFunction *functionType = nullptr;
    };

    struct TPropertyObj
    {
        std::shared_ptr<Object> obj;
        bool constant;
        ast::TypeExpression *type = nullptr;
    };

    struct BoundBuiltinTypeFunction : public Object
    {
        std::shared_ptr<Object> boundTo;
        TBuiltinTypeFunction function;
        ast::TypeFunction *functionType = nullptr;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<BoundBuiltinTypeFunction>(boundTo, function, functionType); };
        BoundBuiltinTypeFunction(const std::shared_ptr<Object> &iboundTo, TBuiltinTypeFunction ifunction, ast::TypeFunction *ifuncionType) : Object(ObjectType::BoundBuiltinTypeFunction), boundTo(iboundTo), function(ifunction), functionType(ifuncionType) {}
    };

    struct BoundBuiltinTypeProperty : public Object
    {
        std::shared_ptr<Object> boundTo;
        TPropertyObj *property = nullptr; // this assumes a stable pointer into the std::unordered_map
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<BoundBuiltinTypeProperty>(boundTo, property); };
        BoundBuiltinTypeProperty(const std::shared_ptr<Object> &iboundTo, TPropertyObj *iproperty) : Object(ObjectType::BoundBuiltinTypeProperty), boundTo(iboundTo), property(iproperty) {}
    };

    struct BuiltinType : public Object
    {
        obj::ObjectType builtinObjectType = obj::ObjectType::Unknown;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<BuiltinType>(); };
        BuiltinType() : Object(ObjectType::BuiltinType){};

        std::unordered_map<std::string, TBuiltinTypeFunctionDefinition> functions;
        std::unordered_map<std::string, TPropertyObj> properties;
    };

    struct UserType : public Object
    {
        std::string doc;
        std::string name;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<UserType>(); };
        UserType() : Object(ObjectType::UserType){};

        std::unordered_map<std::string, std::shared_ptr<Function>> functions;
        std::unordered_map<std::string, TPropertyObj> properties;
    };

    struct BoundUserTypeFunction : public Object
    {
        std::shared_ptr<Object> boundTo;
        std::shared_ptr<Function> function;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<BoundUserTypeFunction>(boundTo, function); };
        BoundUserTypeFunction(const std::shared_ptr<Object> &iboundTo, std::shared_ptr<Function> ifunction) : Object(ObjectType::BoundUserTypeFunction), boundTo(iboundTo), function(ifunction) {}
    };

    struct UserObject : public Object
    {
        static std::atomic_int userInstancesWronglyDestructed;

        std::shared_ptr<UserType> userType;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<UserType>(); };
        std::unordered_map<std::string, TPropertyObj> properties;
        std::shared_ptr<Function> destructor; /*< assigned to at object creation time */
        std::shared_ptr<obj::Object> evalAndResetDestructor(const std::shared_ptr<obj::Environment> &environment);

        UserObject() : Object(ObjectType::UserObject){};
        virtual ~UserObject();
    };

    struct BoundUserTypeProperty : public Object
    {
        std::shared_ptr<Object> boundTo;
        TPropertyObj *property; // this assumes a stable pointer into the std::unordered_map
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override { return std::make_shared<BoundUserTypeProperty>(boundTo, property); };
        BoundUserTypeProperty(const std::shared_ptr<Object> &iboundTo, TPropertyObj *iproperty) : Object(ObjectType::BoundUserTypeProperty), boundTo(iboundTo), property(iproperty) {}
    };

    struct IOObject : public Object
    {
        std::shared_ptr<std::fstream> fStream;
        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<obj::IOObject>();
        };
        IOObject() : Object(ObjectType::IOObject){};
        virtual ~IOObject(){};

        virtual void open(const std::string &fileName, const std::ios_base::openmode = std::ios_base::in);
        virtual void close();
        virtual bool isOpen() const;
        virtual void flush();
        virtual std::string read(size_t maxSize);
        virtual std::string readLine(size_t maxSize);
        virtual std::vector<std::string> readLines(size_t hint);
        virtual void seek(size_t off, int whence);
        virtual int64_t tell() const;
        virtual void write(const std::string &bytes);
    };

    struct Regex : public Object
    {
        std::shared_ptr<std::regex> regex;

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<obj::Regex>(*regex);
        };
        Regex(const std::regex &re);
        virtual ~Regex();
    };

    // struct Clock : public Object
    // {
    //     std::shared_ptr<std::chrono::steady_clock> clock_;

    //     virtual std::string inspect() const override;
    //     virtual std::shared_ptr<Object> clone() const override
    //     {
    //         return std::make_shared<obj::Clock>();
    //     };
    //     Clock();
    //     virtual ~Clock();
    // };

    // struct TimePoint : public Object
    // {
    //     typedef std::chrono::time_point<std::chrono::steady_clock> TTimePoint;

    //     TTimePoint timePoint;

    //     virtual std::string inspect() const override;
    //     virtual std::shared_ptr<Object> clone() const override
    //     {
    //         return std::make_shared<obj::TimePoint>(timePoint);
    //     };
    //     TimePoint(const TTimePoint &tp);
    //     virtual ~TimePoint();
    // };

    struct Thread : public Object
    {
        std::shared_ptr<std::thread> thread;
        std::shared_ptr<Function> function;
        std::shared_ptr<Object> argument;
        std::shared_ptr<Object> functionReturnValue;

        virtual std::string inspect() const override;
        virtual std::shared_ptr<Object> clone() const override
        {
            return std::make_shared<obj::Thread>();
        };
        Thread();
        virtual ~Thread();

        void start();
        void join();
        bool joinable() const;
        void detach();

        void run();
    };
}

#endif