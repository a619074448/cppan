// Copyright (C) 2017-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "jumppad.h"
#include "options.h"

#include <sw/builder/command.h>
#include <sw/builder/file.h>

#include <functional>

namespace sw
{

struct NativeExecutedTarget;

namespace cmd
{

struct Prefix : String {};

namespace detail
{

struct tag_path
{
    path p;

    void populate(const path &f) { p = f; }
};

struct tag_files
{
    FilesOrdered files;

    void populate(const path &f) { files.push_back(f); }
    void populate(const Files &f) { files.insert(files.end(), f.begin(), f.end()); }
    void populate(const FilesOrdered &f) { files.insert(files.end(), f.begin(), f.end()); }
};

struct tag_targets
{
    std::vector<NativeExecutedTarget *> targets;

    void populate(NativeExecutedTarget &t) { targets.push_back(&t); }
};

// in/out data tags
struct tag_do_not_add_to_targets {};
struct tag_skip {};
struct tag_append {};
struct tag_normalize_path {};

struct tag_files_data
{
    bool add_to_targets = true;
    String prefix;
    bool skip = false; // keep order for ADD macro
    bool normalize = false;

    void populate(tag_normalize_path) { normalize = true; }
    void populate(tag_do_not_add_to_targets) { add_to_targets = false; }
    void populate(tag_skip) { skip = true; }
    void populate(const Prefix &p) { prefix = p; }
};

struct tag_io_file : tag_path, tag_targets, tag_files_data
{
    using tag_path::populate;
    using tag_targets::populate;
    using tag_files_data::populate;
};

struct tag_io_files : tag_files, tag_targets, tag_files_data
{
    using tag_files::populate;
    using tag_targets::populate;
    using tag_files_data::populate;
};

struct tag_out_err
{
    bool append = false;
    void populate(tag_append) { append = true; }
};

template <class T, class First, class... Args>
void populate(T &t, const First &arg, Args &&... args)
{
    t.populate(arg);
    if constexpr (sizeof...(Args) > 0)
        populate(t, std::forward<Args>(args)...);
}

template <class T, class... Args>
T in_out(const String &name, Args &&... args)
{
    T t;
    if constexpr (sizeof...(Args) > 0)
        populate(t, std::forward<Args>(args)...);
    if (t.files.empty())
        throw std::logic_error("At least one file must be specified for cmd::" + name);
    return t;
}

} // namespace detail

static detail::tag_do_not_add_to_targets DoNotAddToTargets;
static detail::tag_skip Skip;
static detail::tag_append Append;
static detail::tag_normalize_path NormalizePath;
// TODO: add target list tag, this will simplify lots of code

template <class T>
struct tag_prog { T *t; };
struct tag_wdir : detail::tag_path {};
struct tag_in : detail::tag_io_files {};
struct tag_out : detail::tag_io_files {};
struct tag_stdin : detail::tag_io_file {};
struct tag_stdout : detail::tag_io_file, detail::tag_out_err
{
    using tag_io_file::populate;
    using tag_out_err::populate;
};
struct tag_stderr : detail::tag_io_file, detail::tag_out_err
{
    using tag_io_file::populate;
    using tag_out_err::populate;
};
struct tag_env { String k, v; };
struct tag_end {};

struct tag_dep : detail::tag_targets
{
    std::vector<DependencyPtr> target_ptrs;

    void add(const NativeExecutedTarget &t)
    {
        targets.push_back((NativeExecutedTarget*)&t);
    }

    void add(const DependencyPtr &t)
    {
        target_ptrs.push_back(t);
    }

    void add_array()
    {
    }

    template <class T, class ... Args>
    void add_array(const T &f, Args && ... args)
    {
        add(f);
        add_array(std::forward<Args>(args)...);
    }
};

template <class T>
tag_prog<T> prog(const T &t)
{
    return { (T*)&t };
}

inline tag_wdir wdir(const path &file)
{
    return { file };
}

inline tag_end end()
{
    return {};
}

#define ADD(X)                                                                              \
    inline tag_##X X(const path &file, bool add_to_targets)                                 \
    {                                                                                       \
        std::vector<NativeExecutedTarget *> targets;                                        \
        return {FilesOrdered{file}, targets, add_to_targets};                               \
    }                                                                                       \
                                                                                            \
    inline tag_##X X(const path &file, const String &prefix, bool add_to_targets)           \
    {                                                                                       \
        std::vector<NativeExecutedTarget *> targets;                                        \
        return {FilesOrdered{file}, targets, add_to_targets, prefix};                       \
    }                                                                                       \
                                                                                            \
    inline tag_##X X(const FilesOrdered &files, bool add_to_targets)                        \
    {                                                                                       \
        std::vector<NativeExecutedTarget *> targets;                                        \
        return {files, targets, add_to_targets};                                            \
    }                                                                                       \
                                                                                            \
    inline tag_##X X(const FilesOrdered &files, const String &prefix, bool add_to_targets)  \
    {                                                                                       \
        std::vector<NativeExecutedTarget *> targets;                                        \
        return {files, targets, add_to_targets, prefix};                                    \
    }                                                                                       \
                                                                                            \
    inline tag_##X X(const Files &files, bool add_to_targets)                               \
    {                                                                                       \
        std::vector<NativeExecutedTarget *> targets;                                        \
        return {FilesOrdered{files.begin(), files.end()}, targets, add_to_targets};         \
    }                                                                                       \
                                                                                            \
    inline tag_##X X(const Files &files, const String &prefix, bool add_to_targets)         \
    {                                                                                       \
        std::vector<NativeExecutedTarget *> targets;                                        \
        return {FilesOrdered{files.begin(), files.end()}, targets, add_to_targets, prefix}; \
    }

ADD(in)
ADD(out)

#define ADD2(X)                                                          \
    template <class... Args>                                             \
    tag_##X X(Args &&... args)                                           \
    {                                                                    \
        return detail::in_out<tag_##X>(#X, std::forward<Args>(args)...); \
    }

ADD2(in)
ADD2(out)

#undef ADD
#undef ADD2

inline
tag_stdin std_in(const path &file, bool add_to_targets)
{
    std::vector<NativeExecutedTarget*> targets;
    return { file, targets, add_to_targets };
}

template <class ... Args>
tag_stdin std_in(const path &file, Args && ... args)
{
    std::vector<NativeExecutedTarget*> targets;
    (targets.push_back(&args), ...);
    return { file, targets };
}

inline
tag_stdout std_out(const path &file, bool add_to_targets)
{
    std::vector<NativeExecutedTarget*> targets;
    return { file, targets, add_to_targets };
}

/*template <class ... Args>
tag_stdout std_out(const path &file, Args && ... args)
{
    std::vector<NativeExecutedTarget*> targets;
    (targets.push_back(&args), ...);
    return { file, targets };
}*/

template <class... Args>
tag_stdout std_out(Args &&... args)
{
    tag_stdout t;
    detail::populate(t, std::forward<Args>(args)...);
    return t;
}

inline
tag_stderr std_err(const path &file, bool add_to_targets)
{
    std::vector<NativeExecutedTarget*> targets;
    return { file, targets, add_to_targets };
}

/*template <class ... Args>
tag_stderr std_err(const path &file, Args && ... args)
{
    std::vector<NativeExecutedTarget*> targets;
    (targets.push_back(&args), ...);
    return { file, targets };
}*/

template <class... Args>
tag_stderr std_err(Args &&... args)
{
    tag_stderr t;
    detail::populate(t, std::forward<Args>(args)...);
    return t;
}

template <class ... Args>
tag_dep dep(Args && ... args)
{
    tag_dep d;
    d.add_array(std::forward<Args>(args)...);
    return d;
}

inline tag_env env(const String &k, const String &v)
{
    tag_env d;
    d.k = k;
    d.v = v;
    return d;
}

} // namespace cmd

namespace driver
{

namespace detail
{

struct SW_DRIVER_CPP_API Command : ::sw::builder::Command
{
    using Base = ::sw::builder::Command;

    bool ignore_deps_generated_commands = false;

    Command(const SwContext &swctx);
    Command(const SwContext &swctx, ::sw::FileStorage &fs);
};

}

struct SW_DRIVER_CPP_API Command : detail::Command
{
    using Base = detail::Command;
    using LazyCallback = std::function<String(void)>;
    using LazyAction = std::function<void(void)>;

    bool program_set = false;
    std::weak_ptr<Dependency> dependency;

    Command(const SwContext &swctx);
    Command(const SwContext &swctx, ::sw::FileStorage &fs);
    virtual ~Command() = default;

    virtual std::shared_ptr<Command> clone() const;
    path getProgram() const override;
    void prepare() override;

    using Base::setProgram;
    void setProgram(const std::shared_ptr<Dependency> &d);

    void pushLazyArg(LazyCallback f);
    void addLazyAction(LazyAction f);

    using Base::operator|;
    Command &operator|(struct CommandBuilder &);

private:
    std::map<int, LazyCallback> callbacks;
    std::vector<LazyAction> actions;
    bool dependency_set = false;
};

struct SW_DRIVER_CPP_API ExecuteBuiltinCommand : detail::Command
{
    using F = std::function<void(void)>;

    ExecuteBuiltinCommand(const SwContext &swctx);
    ExecuteBuiltinCommand(const SwContext &swctx, const String &cmd_name, void *f = nullptr, int version = SW_JUMPPAD_DEFAULT_FUNCTION_VERSION);
    virtual ~ExecuteBuiltinCommand() = default;

    //path getProgram() const override { return "ExecuteBuiltinCommand"; };

    //template <class T>
    //auto push_back(T &&v) { args.push_back(v); }

    void push_back(const Files &files);

    bool isTimeChanged() const override;

private:
    void execute1(std::error_code *ec = nullptr) override;
    size_t getHash1() const override;
    void prepare() override {}
};

#ifdef _MSC_VER
#define SW_MAKE_EXECUTE_BUILTIN_COMMAND(var_name, target, func_name, ...) \
    SW_MAKE_CUSTOM_COMMAND(::sw::driver::ExecuteBuiltinCommand, var_name, target, func_name, __VA_ARGS__)
#define SW_MAKE_EXECUTE_BUILTIN_COMMAND_AND_ADD(var_name, target, func_name, ...) \
    SW_MAKE_CUSTOM_COMMAND_AND_ADD(::sw::driver::ExecuteBuiltinCommand, var_name, target, func_name, __VA_ARGS__)
#else
#define SW_MAKE_EXECUTE_BUILTIN_COMMAND(var_name, target, func_name, ...) \
    SW_MAKE_CUSTOM_COMMAND(::sw::driver::ExecuteBuiltinCommand, var_name, target, func_name, ## __VA_ARGS__)
#define SW_MAKE_EXECUTE_BUILTIN_COMMAND_AND_ADD(var_name, target, func_name, ...) \
    SW_MAKE_CUSTOM_COMMAND_AND_ADD(::sw::driver::ExecuteBuiltinCommand, var_name, target, func_name, ## __VA_ARGS__)
#endif

struct VSCommand : Command
{
    using Command::Command;

    std::shared_ptr<Command> clone() const override;

private:
    void postProcess1(bool ok) override;
};

struct GNUCommand : Command
{
    path deps_file;

    using Command::Command;

    std::shared_ptr<Command> clone() const override;

private:
    void postProcess1(bool ok) override;
};

struct SW_DRIVER_CPP_API CommandBuilder
{
    std::shared_ptr<Command> c;
    std::vector<NativeExecutedTarget*> targets;
    bool stopped = false;

    CommandBuilder(const SwContext &swctx);
    CommandBuilder(const SwContext &swctx, ::sw::FileStorage &fs);
    CommandBuilder(const CommandBuilder &) = default;
    CommandBuilder &operator=(const CommandBuilder &) = default;

    CommandBuilder &operator|(CommandBuilder &);
    CommandBuilder &operator|(::sw::builder::Command &);
};

#define DECLARE_STREAM_OP(t) \
    SW_DRIVER_CPP_API        \
    CommandBuilder &operator<<(CommandBuilder &, const t &)

DECLARE_STREAM_OP(NativeExecutedTarget);
DECLARE_STREAM_OP(::sw::cmd::tag_in);
DECLARE_STREAM_OP(::sw::cmd::tag_out);
DECLARE_STREAM_OP(::sw::cmd::tag_stdin);
DECLARE_STREAM_OP(::sw::cmd::tag_stdout);
DECLARE_STREAM_OP(::sw::cmd::tag_stderr);
DECLARE_STREAM_OP(::sw::cmd::tag_wdir);
DECLARE_STREAM_OP(::sw::cmd::tag_end);
DECLARE_STREAM_OP(::sw::cmd::tag_dep);
DECLARE_STREAM_OP(::sw::cmd::tag_env);
DECLARE_STREAM_OP(Command::LazyCallback);

/*template <class T>
CommandBuilder operator<<(std::shared_ptr<Command> &c, const T &t)
{
    CommandBuilder cb;
    cb.c = c;
    cb << t;
    return cb;
}*/

template <class T>
CommandBuilder &operator<<(CommandBuilder &cb, const cmd::tag_prog<T> &t)
{
    if constexpr (std::is_same_v<T, path> || std::is_convertible_v<T, String>)
    {
        throw SW_RUNTIME_ERROR("not implemented");

        cb.c->setProgram(*t.t);
        cb.c->program_set = true;
        return cb;
    }

    bool once = false;
    for (auto tgt : cb.targets)
    {
        auto d = *tgt + *t.t;
        d->setDummy(true);
        if (!once)
        {
            cb.c->setProgram(d);
            once = true;
        }
    }
    cb.c->program_set = true;
    return cb;
}

template <class T>
CommandBuilder &operator<<(CommandBuilder &cb, const T &t)
{
    auto add_arg = [&cb](const String &s)
    {
        if (cb.stopped)
            return;
        if (cb.c->args.empty() && !cb.c->program_set)
        {
            cb.c->program = s;
            cb.c->program_set = true;
        }
        else
            cb.c->args.push_back(s);
    };

    if constexpr (std::is_same_v<T, path>)
    {
        if (!cb.stopped)
            add_arg(t.u8string());
    }
    else if constexpr (std::is_same_v<T, String>)
    {
        if (!cb.stopped)
            add_arg(t);
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        if (!cb.stopped)
            add_arg(std::to_string(t));
    }
    else if constexpr (std::is_base_of_v<NativeExecutedTarget, T>)
    {
        return cb << (const NativeExecutedTarget&)t;
    }
    else
    {
        // add static assert?
        if (!cb.stopped)
            add_arg(t);
    }
    return cb;
}

#undef DECLARE_STREAM_OP

SW_DRIVER_CPP_API
String getInternalCallBuiltinFunctionName();

enum class BuiltinCommandArgumentId
{
    ArgumentKeyword,
    ModulePath,
    FunctionName,
    FunctionVersion,
    FirstArgument,
};

} // namespace driver

} // namespace sw