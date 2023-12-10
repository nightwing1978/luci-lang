#include "Object.h"
#include "Util.h"

#include "Evaluator.h" // to support evaluating the destructor
#include "Ast.h"       // to support function evaluation, knowing the type hierarchy from Node->BlockStatement

#include <sstream>

namespace obj
{
    std::atomic_int Object::instancesConstructed = 0;
    std::atomic_int Object::instancesDestructed = 0;

    std::atomic_int UserObject::userInstancesWronglyDestructed = 0;

    std::atomic_int Environment::instancesConstructed = 0;
    std::atomic_int Environment::instancesDestructed = 0;

    Environment::Environment()
    {
        instancesConstructed += 1;
    }

    Environment::~Environment()
    {
        instancesDestructed += 1;
    }

    std::string toString(const ObjectType &type)
    {
        switch (type)
        {
        case ObjectType::Null:
            return "Null";
        case ObjectType::Integer:
            return "Integer";
        case ObjectType::Boolean:
            return "Boolean";
        case ObjectType::Double:
            return "Double";
        case ObjectType::Complex:
            return "Complex";
        case ObjectType::String:
            return "String";
        case ObjectType::Function:
            return "Function";
        case ObjectType::Error:
            return "Error";
        case ObjectType::ReturnValue:
            return "ReturnValue";
        case ObjectType::Array:
            return "Array";
        case ObjectType::ArrayDouble:
            return "ArrayDouble";
        case ObjectType::ArrayComplex:
            return "ArrayComplex";
        case ObjectType::Dictionary:
            return "Dictionary";
        case ObjectType::Set:
            return "Set";
        case ObjectType::BoundBuiltinTypeFunction:
            return "BoundBuiltinTypeFunction";
        case ObjectType::BoundBuiltinTypeProperty:
            return "BoundBuiltinTypeProperty";
        case ObjectType::BoundUserTypeFunction:
            return "BoundUserTypeFunction";
        case ObjectType::BoundUserTypeProperty:
            return "BoundUserTypeProperty";
        case ObjectType::UserObject:
            return "UserObject";
        case ObjectType::IOObject:
            return "IOObject";
        case ObjectType::Iterator:
            return "Iterator";
        case ObjectType::Char:
            return "Char";
        case ObjectType::Freezer:
            return "Freezer";
        case ObjectType::Module:
            return "Module";
        case ObjectType::Thread:
            return "Thread";
        case ObjectType::Range:
            return "Range";
        case ObjectType::Regex:
            return "Regex";
        };
        return "Unknown Type";
    }

    Object::Object(ObjectType itype) : type(itype)
    {
        ++instancesConstructed;
    };

    Object::~Object()
    {
        ++instancesDestructed;
    };

    String::~String(){};

    Module::Module() : Object(ObjectType::Module)
    {
        environment = std::make_shared<obj::Environment>();
    }

    std::string Module::inspect() const
    {
        return "module";
    }

    std::string Null::inspect() const
    {
        return "null";
    }

    Integer::~Integer()
    {
    }

    std::string Integer::inspect() const
    {
        return std::to_string(value);
    }

    std::string Double::inspect() const
    {
        return std::to_string(value);
    }

    std::string Complex::inspect() const
    {
        return std::to_string(value.real()) + "+" + std::to_string(value.imag()) + "j";
    }

    std::string Boolean::inspect() const
    {
        return value ? "true" : "false";
    }

    std::string Char::inspect() const
    {
        return std::to_string(value);
    }

    std::string String::inspect() const
    {
        return "\"" + value + "\"";
    }

    std::string Range::inspect() const
    {
        return std::to_string(lower) + ":" + std::to_string(upper) + ":" + std::to_string(stride);
    }

    int64_t Range::length() const
    {
        if (lower >= upper)
            return 0;

        if (stride == 1)
            return upper - lower;

        int64_t count = 0;
        for (int64_t idx = lower; idx < upper; idx += stride)
            ++count;

        return count;
    }

    std::vector<int64_t> Range::values() const
    {
        std::vector<int64_t> ret;
        for (int64_t idx = lower; idx < upper; idx += stride)
            ret.push_back(idx);

        return ret;
    }

    std::string Array::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
        {
            if (element)
                elements.push_back(element->inspect());
            else
                elements.push_back("<Null>");
        }

        ss << "[";
        ss << util::join(elements, ", ");
        ss << "]";
        return ss.str();
    }

    Array::Array(const std::vector<double> &ivalue) : Object(ObjectType::Array)
    {
        value.reserve(ivalue.size() + 1);
        for (const auto &element : ivalue)
            value.push_back(std::make_shared<obj::Double>(element));
    };

    Array::Array(const std::vector<std::complex<double>> &ivalue) : Object(ObjectType::Array)
    {
        value.reserve(ivalue.size() + 1);
        for (const auto &element : ivalue)
            value.push_back(std::make_shared<obj::Complex>(element));
    };

    std::string ArrayDouble::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
            elements.push_back(std::to_string(element));

        ss << "[";
        ss << util::join(elements, ", ");
        ss << "]";
        return ss.str();
    }

    std::string ArrayComplex::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
            elements.push_back(std::to_string(element.real()) + "+" + std::to_string(element.imag()) + "j");

        ss << "[";
        ss << util::join(elements, ", ");
        ss << "]";
        return ss.str();
    }

    std::string Iterator::inspect() const
    {
        std::stringstream ss;
        ss << "Iterator()";
        return ss.str();
    }

    std::string Dictionary::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> pairs;
        for (const auto &element : value)
            pairs.push_back(util::join({element.first->inspect(), element.second->inspect()}, ":"));

        ss << "{";
        ss << util::join(pairs, ", ");
        ss << "}";
        return ss.str();
    }

    std::string Set::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
            elements.push_back(element->inspect());

        ss << "{";
        ss << util::join(elements, ", ");
        ss << "}";
        return ss.str();
    }

    std::string ReturnValue::inspect() const
    {
        return "Return(" + value->inspect() + ")";
    }

    std::string BreakValue::inspect() const
    {
        return "Break()";
    }

    std::string Error::inspect() const
    {
        if (!token.fileName || token.fileName->empty())
            return "Error(" + msg + ") at (" + std::to_string(token.lineNumber) + "," + std::to_string(token.columnNumber) + ")";

        return "Error(" + msg + ") at " + *token.fileName + "(" + std::to_string(token.lineNumber) + "," + std::to_string(token.columnNumber) + ")";
    }

    std::shared_ptr<Error> makeTypeError(const std::string &msg)
    {
        return std::make_shared<Error>(msg, ErrorType::TypeError);
    }

    std::string Exit::inspect() const
    {
        return "Exit(" + std::to_string(value) + ") at " + std::to_string(token.lineNumber) + ":" + std::to_string(token.columnNumber);
    }

    std::string Function::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> args;
        for (const auto &arg : arguments)
            args.push_back(arg.text());

        ss << "fn";
        ss << "(";
        ss << util::join(args, ",");
        ss << ")\n";
        ss << "{\n";
        ss << body->text();
        ss << "}\n";
        return ss.str();
    }

    std::string Builtin::inspect() const
    {
        return "Builtin function";
    }

    bool Environment::has(const std::string &name) const
    {
        auto storeIt = store.find(name);
        if (storeIt == store.end())
        {
            if (outer)
                return outer->has(name);
            return false;
        }
        return true;
    }

    std::shared_ptr<Object> Environment::get(const std::string &name) const
    {
        auto storeIt = store.find(name);
        if (storeIt == store.end())
        {
            if (outer)
                return outer->get(name);
            return std::make_unique<obj::Error>(obj::Error("Identifier not found: " + name, obj::ErrorType::IdentifierNotFound));
        }
        return storeIt->second.obj;
    }

    ast::TypeExpression *Environment::getType(const std::string &name) const
    {
        auto storeIt = store.find(name);
        if (storeIt == store.end())
        {
            if (outer)
                return outer->getType(name);
            return nullptr;
        }
        return storeIt->second.type;
    }

    std::shared_ptr<Object> Environment::set(const std::string &name, std::shared_ptr<Object> value)
    {
        auto storeIt = store.find(name);
        if (storeIt == store.end())
        {
            if (outer)
                return outer->set(name, value);
            return std::make_unique<obj::Error>("identifier not found: " + name, obj::ErrorType::IdentifierNotFound);
        }

        if (storeIt->second.constant)
            return std::make_unique<obj::Error>("variable is const: " + name, obj::ErrorType::ConstError);

        storeIt->second.obj = std::move(value);
        return storeIt->second.obj;
    }

    std::shared_ptr<Object> Environment::add(const std::string &name, std::shared_ptr<Object> value, bool constant, ast::TypeExpression *type)
    {
        auto storeIt = store.find(name);
        if (storeIt != store.end())
        {
            return std::make_unique<obj::Error>("identifier already found: " + name, obj::ErrorType::IdentifierAlreadyExists);
        }

        auto storeInsertionIt = store.insert(std::make_pair(name, TTokenSharedObj({value, constant, type})));
        return value;
    }

    std::string BuiltinType::inspect() const
    {
        return "Builtin type";
    }

    std::string UserType::inspect() const
    {
        return "User type " + name;
    }

    UserObject::~UserObject()
    {
        if (destructor)
        {
            ++userInstancesWronglyDestructed;
            // throw std::runtime_error("UserObject should have their destructors called explicitly");
            // auto emptyEnvironment = std::make_shared<obj::Environment>();
            // auto sunkenValue = evalAndResetDestructor(emptyEnvironment);
        }
    }

    std::shared_ptr<obj::Object> UserObject::evalAndResetDestructor(const std::shared_ptr<obj::Environment> &environment)
    {
        if (destructor)
        {
            auto retValue = evalDestructor(destructor.get(), this, environment);
            destructor.reset();
            return retValue;
        }
        return NullObject;
    }

    std::string UserObject::inspect() const
    {
        return "<" + userType->name + ">";
    }

    std::string BoundBuiltinTypeFunction::inspect() const
    {
        return "BoundBuiltin type function";
    }

    std::string BoundBuiltinTypeProperty::inspect() const
    {
        return property->obj->inspect();
    }

    std::string BoundUserTypeFunction::inspect() const
    {
        return "Bound Usertype function";
    }

    std::string BoundUserTypeProperty::inspect() const
    {
        return property->obj->inspect();
    }

    std::string IOObject::inspect() const
    {
        return "<ioobject>";
    }

    void IOObject::open(const std::string &fileName, const std::ios_base::openmode openMode)
    {
        if (!fStream)
            fStream.reset(new std::fstream());

        fStream->open(fileName, openMode);
    }

    void IOObject::close()
    {
        if (fStream)
            fStream->close();
    }

    bool IOObject::isOpen() const
    {
        if (!fStream)
            return false;

        return fStream->is_open();
    }

    void IOObject::flush()
    {
        if (!fStream)
            return;

        fStream->flush();
    }

    std::string IOObject::read(size_t maxSize)
    {
        if (!fStream)
            return std::string("");

        std::string result;

        size_t toRead = maxSize;
        const size_t blockSize = 4096;
        char buffer[blockSize + 1];

        while (toRead > 0 && !fStream->eof() && !fStream->fail())
        {
            fStream->read(&buffer[0], std::min<size_t>(blockSize, toRead));
            size_t bytesRead = fStream->gcount();
            buffer[bytesRead] = '\0';
            result += std::string(buffer);
            if (bytesRead >= toRead)
                toRead = 0;
            else
                toRead -= bytesRead;
        }
        return result;
    }

    std::string IOObject::readLine(size_t maxSize)
    {
        if (!fStream)
            return std::string("");

        std::string aLine;
        std::getline(*fStream, aLine);
        return aLine;
    }

    std::vector<std::string> IOObject::readLines(size_t maxSize)
    {
        if (!fStream)
            return std::vector<std::string>();

        size_t totalSize = 0;
        std::vector<std::string> result;

        while (totalSize < maxSize && !fStream->eof() && !fStream->fail())
        {
            std::string aLine;
            std::getline(*fStream, aLine);
            result.push_back(aLine);
            totalSize += aLine.size();
        }
        return result;
    }

    void IOObject::seek(size_t off, int whence)
    {
        if (!fStream)
            return;

        fStream->seekg(off, static_cast<std::ios_base::seekdir>(whence));
    }

    int64_t IOObject::tell() const
    {
        if (!fStream)
            return -1;

        return fStream->tellg();
    }

    void IOObject::write(const std::string &bytes)
    {
        if (!fStream)
            return;

        fStream->write(bytes.data(), bytes.size());
    }

    Thread::Thread() : Object(ObjectType::Thread)
    {
    }

    Thread::~Thread()
    {
    }

    void Thread::join()
    {
        if (thread)
            thread->join();
    }

    void Thread::run()
    {
        auto environment = std::make_shared<obj::Environment>();
        environment->outer = function->environment;
        functionReturnValue = eval(function->body, environment);
    }

    void Thread::start()
    {
        if (thread)
        {
            throw std::runtime_error("Cannot start thread twice");
        }

        thread.reset(new std::thread(&Thread::run, this));
    }

    bool Thread::joinable() const
    {
        if (thread)
            return thread->joinable();
        return true;
    }

    std::string Thread::inspect() const
    {
        return "<thread>";
    }

    Regex::Regex(const std::regex &re) : Object(ObjectType::Regex)
    {
        regex = std::make_shared<std::regex>(re);
    }

    Regex::~Regex()
    {
    }

    std::string Regex::inspect() const
    {
        return "<regex>";
    }

}
