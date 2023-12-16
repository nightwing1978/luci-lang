/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include <filesystem>
#include <functional>
#include <cstdlib>

#include "OS.h"
#include "../Evaluator.h"

namespace builtin
{
    std::shared_ptr<obj::Object> path_join(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("join: expected 1 argument of [str]");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type != obj::ObjectType::Array)
            return obj::makeTypeError("join: expected argument 1 to be [str]");

        std::filesystem::path pathArr;
        auto arr = static_cast<obj::Array *>(evaluatedExpr.get());
        for (const auto &element : arr->value)
        {
            if (element->type != obj::ObjectType::String)
                return obj::makeTypeError("join: expected argument 1 to be [str]");

            auto pathEl = static_cast<obj::String *>(element.get());
            pathArr /= pathEl->value;
        }

        return std::make_shared<obj::String>(pathArr.generic_string());
    }

    std::shared_ptr<obj::Object> path_r_str_str(std::function<std::filesystem::path(const std::filesystem::path &)> func, const std::string &name, const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError(name + ": expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError(name + ": expected argument 1 to be str");

        std::filesystem::path pathValue(static_cast<obj::String *>(evaluatedExpr.get())->value);
        auto result = func(pathValue);
        return std::make_shared<obj::String>(result.generic_string());
    }

    std::shared_ptr<obj::Object> path_r_bool_str(std::function<bool(const std::filesystem::path &)> func, const std::string &name, const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError(name + ": expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError(name + ": expected argument 1 to be str");

        std::filesystem::path pathValue(static_cast<obj::String *>(evaluatedExpr.get())->value);
        auto result = func(pathValue);
        return std::make_shared<obj::Boolean>(result);
    }

    std::shared_ptr<obj::Object> path_str_str(std::function<void(const std::filesystem::path &, const std::filesystem::path &)> func, const std::string &name, const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return obj::makeTypeError(name + ": expected 2 arguments (str,str)");

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr1->type == obj::ObjectType::Error)
            return evaluatedExpr1;
        if (evaluatedExpr1->type != obj::ObjectType::String)
            return obj::makeTypeError(name + ": expected argument 1 to be str");

        auto evaluatedExpr2 = evalExpression(arguments->back().get(), environment);
        if (evaluatedExpr2->type == obj::ObjectType::Error)
            return evaluatedExpr2;
        if (evaluatedExpr2->type != obj::ObjectType::String)
            return obj::makeTypeError(name + ": expected argument 1 to be str");

        std::filesystem::path pathValue1(static_cast<obj::String *>(evaluatedExpr1.get())->value);
        std::filesystem::path pathValue2(static_cast<obj::String *>(evaluatedExpr2.get())->value);
        func(pathValue1, pathValue2);
        return NullObject;
    }

    std::shared_ptr<obj::Object> path_r_uintmax_str(std::function<std::uintmax_t(const std::filesystem::path &)> func, const std::string &name, const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError(name + ": expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError(name + ": expected argument 1 to be str");

        std::filesystem::path pathValue(static_cast<obj::String *>(evaluatedExpr.get())->value);
        auto result = func(pathValue);
        return std::make_shared<obj::Integer>(static_cast<int64_t>(result));
    }

    std::shared_ptr<obj::Object> root_name(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::root_name, "root_name", arguments, environment);
    }

    std::shared_ptr<obj::Object> root_directory(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::root_directory, "root_directory", arguments, environment);
    }

    std::shared_ptr<obj::Object> root_path(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::root_path, "root_path", arguments, environment);
    }

    std::shared_ptr<obj::Object> relative_path(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::relative_path, "relative_path", arguments, environment);
    }

    std::shared_ptr<obj::Object> parent_path(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::parent_path, "parent_path", arguments, environment);
    }

    std::shared_ptr<obj::Object> filename(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::filename, "filename", arguments, environment);
    }

    std::shared_ptr<obj::Object> stem(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::stem, "stem", arguments, environment);
    }

    std::shared_ptr<obj::Object> extension(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_str_str(&std::filesystem::path::extension, "extension", arguments, environment);
    }

    std::shared_ptr<obj::Object> is_relative(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_bool_str(&std::filesystem::path::is_relative, "is_relative", arguments, environment);
    }

    std::shared_ptr<obj::Object> is_absolute(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return path_r_bool_str(&std::filesystem::path::is_relative, "is_absolute", arguments, environment);
    }

    std::shared_ptr<obj::Object> absolute(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        std::filesystem::path (*func)(const std::filesystem::path &p) = &std::filesystem::absolute;
        return path_r_str_str(func, "absolute", arguments, environment);
    }

    std::shared_ptr<obj::Object> canonical(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        std::filesystem::path (*func)(const std::filesystem::path &p) = &std::filesystem::canonical;
        return path_r_str_str(func, "canonical", arguments, environment);
    }

    std::shared_ptr<obj::Object> weakly_canonical(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        std::filesystem::path (*func)(const std::filesystem::path &p) = &std::filesystem::weakly_canonical;
        return path_r_str_str(func, "weakly_canonical", arguments, environment);
    }

    std::shared_ptr<obj::Object> current_path(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 1)
            return obj::makeTypeError("current_path: expected 0 or 1 argument");

        if (arguments->size() == 1)
        {
            auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
            if (evaluatedExpr->type == obj::ObjectType::Error)
                return evaluatedExpr;
            if (evaluatedExpr->type != obj::ObjectType::String)
                return obj::makeTypeError("current_path: expected argument 1 to be str");

            std::filesystem::current_path(static_cast<obj::String *>(evaluatedExpr.get())->value);
            return NullObject;
        }
        return std::make_shared<obj::String>(std::filesystem::current_path().generic_string());
    }

    std::shared_ptr<obj::Object> temp_directory_path(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (!arguments->empty())
            return obj::makeTypeError("temp_directory_path: expected no arguments");

        return std::make_shared<obj::String>(std::filesystem::temp_directory_path().generic_string());
    }

    std::shared_ptr<obj::Object> exists(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("exists: expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError("exists: expected argument 1 to be str");

        return std::make_shared<obj::Boolean>(std::filesystem::exists(static_cast<obj::String *>(evaluatedExpr.get())->value));
    }

    std::shared_ptr<obj::Object> list_dir_recursively(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("list_dir_recursively: expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError("list_dir_recursively: expected argument 1 to be str");

        std::vector<std::shared_ptr<obj::Object>> paths;

        auto startPath = static_cast<obj::String *>(evaluatedExpr.get())->value;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(startPath))
            paths.push_back(std::make_shared<obj::String>(entry.path().generic_string()));

        return std::make_shared<obj::Array>(paths);
    }

    std::shared_ptr<obj::Object> list_dir(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("list_dir_recursively: expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError("list_dir_recursively: expected argument 1 to be str");

        std::vector<std::shared_ptr<obj::Object>> paths;

        auto startPath = static_cast<obj::String *>(evaluatedExpr.get())->value;
        for (const auto &entry : std::filesystem::directory_iterator(startPath))
            paths.push_back(std::make_shared<obj::String>(entry.path().generic_string()));

        return std::make_shared<obj::Array>(paths);
    }

    std::shared_ptr<obj::Object> create_directory(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        bool (*func)(const std::filesystem::path &p) = &std::filesystem::create_directory;
        return path_r_bool_str(func, "create_directory", arguments, environment);
    }

    std::shared_ptr<obj::Object> create_directories(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        bool (*func)(const std::filesystem::path &p) = &std::filesystem::create_directories;
        return path_r_bool_str(func, "create_directories", arguments, environment);
    }

    std::shared_ptr<obj::Object> remove(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        bool (*func)(const std::filesystem::path &p) = &std::filesystem::remove;
        return path_r_bool_str(func, "remove", arguments, environment);
    }

    std::shared_ptr<obj::Object> remove_all(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        std::uintmax_t (*func)(const std::filesystem::path &p) = &std::filesystem::remove_all;
        return path_r_uintmax_str(func, "remove_all", arguments, environment);
    }

    std::shared_ptr<obj::Object> copy(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        void (*func)(const std::filesystem::path &, const std::filesystem::path &) = &std::filesystem::copy;
        return path_str_str(func, "copy", arguments, environment);
    }

    std::shared_ptr<obj::Object> rename(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        void (*func)(const std::filesystem::path &, const std::filesystem::path &) = &std::filesystem::rename;
        return path_str_str(func, "rename", arguments, environment);
    }

    std::shared_ptr<obj::Object> system(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("system: expected 1 argument of type str");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError("system: expected argument 1 to be str");

        auto ret = std::system(static_cast<obj::String *>(evaluatedExpr.get())->value.c_str());
        return std::make_shared<obj::Integer>(ret);
    }

    std::shared_ptr<obj::Object> getenv(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("getenv: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;
        if (evaluatedExpr->type != obj::ObjectType::String)
            return obj::makeTypeError("getenv: expected argument 1 to be str");

        auto ret = std::getenv(static_cast<obj::String *>(evaluatedExpr.get())->value.c_str());
        return std::make_shared<obj::String>(ret);
    }

    std::shared_ptr<obj::Module> makeModulePath()
    {
        auto pathModule = std::make_shared<obj::Module>();
        pathModule->state = obj::ModuleState::Loaded;
        pathModule->environment = std::make_shared<obj::Environment>();
        pathModule->environment->add("join", builtin::makeBuiltInFunctionObj(path_join, "[str]", "str"), false, nullptr);
        pathModule->environment->add("root_name", builtin::makeBuiltInFunctionObj(root_name, "str", "str"), false, nullptr);
        pathModule->environment->add("root_directory", builtin::makeBuiltInFunctionObj(root_directory, "str", "str"), false, nullptr);
        pathModule->environment->add("root_path", builtin::makeBuiltInFunctionObj(root_path, "str", "str"), false, nullptr);
        pathModule->environment->add("relative_path", builtin::makeBuiltInFunctionObj(relative_path, "str", "str"), false, nullptr);
        pathModule->environment->add("parent_path", builtin::makeBuiltInFunctionObj(parent_path, "str", "str"), false, nullptr);
        pathModule->environment->add("filename", builtin::makeBuiltInFunctionObj(filename, "str", "str"), false, nullptr);
        pathModule->environment->add("stem", builtin::makeBuiltInFunctionObj(stem, "str", "str"), false, nullptr);
        pathModule->environment->add("extension", builtin::makeBuiltInFunctionObj(extension, "str", "str"), false, nullptr);

        pathModule->environment->add("is_relative", builtin::makeBuiltInFunctionObj(is_relative, "str", "bool"), false, nullptr);
        pathModule->environment->add("is_absolute", builtin::makeBuiltInFunctionObj(is_absolute, "str", "bool"), false, nullptr);
        return pathModule;
    }

    std::shared_ptr<obj::Module> makeModuleOS()
    {
        auto osModule = std::make_shared<obj::Module>();
        osModule->state = obj::ModuleState::Loaded;
        osModule->environment = std::make_shared<obj::Environment>();
        osModule->environment->add("absolute", builtin::makeBuiltInFunctionObj(absolute, "str", "str"), false, nullptr);
        osModule->environment->add("canonical", builtin::makeBuiltInFunctionObj(canonical, "str", "str"), false, nullptr);
        osModule->environment->add("weakly_canonical", builtin::makeBuiltInFunctionObj(weakly_canonical, "str", "str"), false, nullptr);

        osModule->environment->add("current_path", builtin::makeBuiltInFunctionObj(current_path, "", "str"), false, nullptr);
        osModule->environment->add("temp_directory_path", builtin::makeBuiltInFunctionObj(temp_directory_path, "", "str"), false, nullptr);

        osModule->environment->add("exists", builtin::makeBuiltInFunctionObj(exists, "", "str"), false, nullptr);

        osModule->environment->add("create_directory", builtin::makeBuiltInFunctionObj(create_directory, "str", "bool"), false, nullptr);
        osModule->environment->add("create_directories", builtin::makeBuiltInFunctionObj(create_directories, "str", "bool"), false, nullptr);
        osModule->environment->add("remove", builtin::makeBuiltInFunctionObj(remove, "str", "bool"), false, nullptr);
        osModule->environment->add("remove_all", builtin::makeBuiltInFunctionObj(remove_all, "str", "bool"), false, nullptr);

        osModule->environment->add("copy", builtin::makeBuiltInFunctionObj(copy, "str,str", "null"), false, nullptr);
        osModule->environment->add("rename", builtin::makeBuiltInFunctionObj(rename, "str,str", "null"), false, nullptr);

        osModule->environment->add("list_dir", builtin::makeBuiltInFunctionObj(list_dir, "str", "[str]"), false, nullptr);
        osModule->environment->add("list_dir_recursively", builtin::makeBuiltInFunctionObj(list_dir_recursively, "str", "[str]"), false, nullptr);

        osModule->environment->add("system", builtin::makeBuiltInFunctionObj(system, "str", "int"), false, nullptr);
        osModule->environment->add("getenv", builtin::makeBuiltInFunctionObj(getenv, "str", "str"), false, nullptr);

        // add a submodule
        osModule->environment->add("path", makeModulePath(), true, nullptr);
        return osModule;
    }
}