// Copyright (C) 2017-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "dependency.h"
#include "types.h"

#include <sw/builder/configuration.h>
#include <sw/manager/package.h>
#include <sw/manager/property.h>

#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <variant>

namespace sw
{

namespace builder
{
struct Command;
}

struct NativeOptions;
struct Target;
struct Package;

using DefinitionKey = std::string;
using DefinitionValue = PropertyValue;
using VariableValue = PropertyValue;

struct DefinitionsType : std::map<DefinitionKey, VariableValue>
{
    using base = std::map<DefinitionKey, VariableValue>;

    base::mapped_type &operator[](const base::key_type &k)
    {
        if (!k.empty() && k.back() != '=')
            base::operator[](k);// = 1;
        return base::operator[](k);
    }

    // add operator+= ?
};

struct VariablesType : std::map<DefinitionKey, VariableValue>
{
    using base = std::map<DefinitionKey, VariableValue>;

    bool has(const typename base::key_type &k) const
    {
        return find(k) != end();
    }
};

template <class T>
class UniqueVector : public std::vector<T>
{
    using unique_type = std::unordered_set<T>;
    using base = std::vector<T>;

public:
    std::pair<typename base::iterator, bool> insert(const T &e)
    {
        auto[_, inserted] = u.insert(e);
        if (!inserted)
            return { base::end(), false };
        this->push_back(e);
        return { --base::end(), true };
    }

    template <class B, class E>
    void insert(const B &b, const E &e)
    {
        for (auto i = b; i != e; ++i)
            insert(*i);
    }

    void erase(const T &e)
    {
        if (!u.erase(e))
            return;
        base::erase(std::remove(base::begin(), base::end(), e), base::end());
    }

private:
    unique_type u;
};

struct FancyFilesOrdered : FilesOrdered
{
    using base = FilesOrdered;

    using base::insert;
    using base::erase;

    // fix return type
    void insert(const path &p)
    {
        push_back(p);
    }

    void erase(const path &p)
    {
        base::erase(std::remove(begin(), end(), p), end());
    }
};

//using PathOptionsType = FilesSorted;
using PathOptionsType = UniqueVector<path>;

struct SW_DRIVER_CPP_API Variable
{
    String v;
};

struct SW_DRIVER_CPP_API Definition
{
    String d;

    Definition() = default;
    explicit Definition(const String &p);
};

struct SW_DRIVER_CPP_API IncludeDirectory
{
    String i;

    IncludeDirectory() = default;
    explicit IncludeDirectory(const String &p);
    explicit IncludeDirectory(const path &p);
};

struct SW_DRIVER_CPP_API LinkDirectory
{
    String d;

    LinkDirectory() = default;
    explicit LinkDirectory(const String &p);
    explicit LinkDirectory(const path &p);
};

struct SW_DRIVER_CPP_API LinkLibrary
{
    String l;

    LinkLibrary() = default;
    explicit LinkLibrary(const String &p);
    explicit LinkLibrary(const path &p);
};

struct SW_DRIVER_CPP_API SystemLinkLibrary
{
    String l;

    SystemLinkLibrary() = default;
    explicit SystemLinkLibrary(const String &p);
    explicit SystemLinkLibrary(const path &p);
};

struct SW_DRIVER_CPP_API FileRegex
{
    path dir;
    std::regex r;
    bool recursive;

    FileRegex(const String &r, bool recursive);
    FileRegex(const std::regex &r, bool recursive);
    FileRegex(const path &dir, const String &r, bool recursive);
    FileRegex(const path &dir, const std::regex &r, bool recursive);

    String getRegexString() const;

private:
    String regex_string;
};

using DependenciesType = UniqueVector<DependencyPtr>;

struct SW_DRIVER_CPP_API NativeCompilerOptionsData
{
    DefinitionsType Definitions;
    DefinitionsType Definitions2; // untouched defs
    UniqueVector<String> CompileOptions;
    PathOptionsType PreIncludeDirectories;
    PathOptionsType IncludeDirectories;
    PathOptionsType PostIncludeDirectories;

    PathOptionsType gatherIncludeDirectories() const;
    bool IsIncludeDirectoriesEmpty() const;
    void merge(const NativeCompilerOptionsData &o, const GroupSettings &s = GroupSettings(), bool merge_to_system = false);

    void add(const Definition &d);
    void remove(const Definition &d);
    void add(const DefinitionsType &defs);
    void remove(const DefinitionsType &defs);
};

using LinkLibrariesType = FancyFilesOrdered;

struct SW_DRIVER_CPP_API NativeLinkerOptionsData
{
    PathOptionsType Frameworks; // macOS
    // it is possible to have repeated link libraries on the command line
    LinkLibrariesType LinkLibraries;
    LinkLibrariesType LinkLibraries2; // untouched link libs
    Strings LinkOptions;
    PathOptionsType PreLinkDirectories;
    PathOptionsType LinkDirectories;
    PathOptionsType PostLinkDirectories;

    PathOptionsType gatherLinkDirectories() const;
    LinkLibrariesType gatherLinkLibraries() const;
    bool IsLinkDirectoriesEmpty() const;
    void merge(const NativeLinkerOptionsData &o, const GroupSettings &s = GroupSettings());

    void add(const LinkDirectory &l);
    void remove(const LinkDirectory &l);

    void add(const LinkLibrary &l);
    void remove(const LinkLibrary &l);
};

struct SW_DRIVER_CPP_API NativeCompilerOptions : NativeCompilerOptionsData
{
    NativeCompilerOptionsData System;

    using NativeCompilerOptionsData::add;
    using NativeCompilerOptionsData::remove;

    void merge(const NativeCompilerOptions &o, const GroupSettings &s = GroupSettings());
    //void unique();

    void addDefinitionsAndIncludeDirectories(builder::Command &c) const;
    void addEverything(builder::Command &c) const;
    PathOptionsType gatherIncludeDirectories() const;
};

struct SW_DRIVER_CPP_API NativeLinkerOptions : NativeLinkerOptionsData
{
    DependenciesType Dependencies_;
    NativeLinkerOptionsData System;

    using NativeLinkerOptionsData::add;
    using NativeLinkerOptionsData::remove;

    void add(const Target &t);
    void remove(const Target &t);

    void add(const DependencyPtr &t);
    void remove(const DependencyPtr &t);

    void add(const UnresolvedPackage &t);
    void remove(const UnresolvedPackage &t);

    void add(const UnresolvedPackages &t);
    void remove(const UnresolvedPackages &t);

    void add(const PackageId &t);
    void remove(const PackageId &t);

    void add(const SystemLinkLibrary &l);
    void remove(const SystemLinkLibrary &l);

    void merge(const NativeLinkerOptions &o, const GroupSettings &s = GroupSettings());
    void addEverything(builder::Command &c) const;
    FilesOrdered gatherLinkLibraries() const;

    DependencyPtr operator+(const Target &t);
    DependencyPtr operator+(const DependencyPtr &d);
    DependencyPtr operator+(const PackageId &d);
};

using UnresolvedDependenciesType = std::unordered_map<UnresolvedPackage, DependencyPtr>;

struct SW_DRIVER_CPP_API NativeOptions : NativeCompilerOptions,
    NativeLinkerOptions
{
    using NativeCompilerOptions::add;
    using NativeCompilerOptions::remove;
    using NativeLinkerOptions::add;
    using NativeLinkerOptions::remove;

    void merge(const NativeOptions &o, const GroupSettings &s = GroupSettings());
};

template <class T>
struct InheritanceStorage : std::vector<T*>
{
    using base = std::vector<T*>;

    InheritanceStorage(T *pvt)
        : base(toIndex(InheritanceType::Max), nullptr)
    {
        base::operator[](1) = pvt;
    }

    ~InheritanceStorage()
    {
        // we do not own 0 and 1 elements
        for (int i = toIndex(InheritanceType::Min) + 1; i < toIndex(InheritanceType::Max); i++)
            delete base::operator[](i);
    }

    T &operator[](int i)
    {
        auto &e = base::operator[](i);
        if (!e)
            e = new T;
        return *e;
    }

    const T &operator[](int i) const
    {
        return *base::operator[](i);
    }

    T &operator[](InheritanceType i)
    {
        return operator[](toIndex(i));
    }

    const T &operator[](InheritanceType i) const
    {
        return operator[](toIndex(i));
    }

    base &raw() { return *this; }
    const base &raw() const { return *this; }
};

/**
* \brief By default, group items considered as Private scope.
*/
template <class T>
struct InheritanceGroup : T
{
private:
    InheritanceStorage<T> data;

public:
    /**
    * \brief visible only in current target
    */
    T &Private;

    /**
    * \brief visible only in target and current project
    */
    T &Protected;
    // T &Project; ???

    /**
    * \brief visible both in target and its users
    */
    T &Public;

    /**
    * \brief visible in target's users
    */
    T &Interface;

    InheritanceGroup()
        : T()
        , data(this)
        , Private(*this)
        , Protected(data[InheritanceType::Protected])
        , Public(data[InheritanceType::Public])
        , Interface(data[InheritanceType::Interface])
    {
    }

    using T::operator=;

    T &get(InheritanceType Type)
    {
        switch (Type)
        {
        case InheritanceType::Private:
            return Private;
        case InheritanceType::Protected:
            return Protected;
        case InheritanceType::Public:
            return Public;
        case InheritanceType::Interface:
            return Interface;
        default:
            return data[Type];
        }
        throw SW_RUNTIME_ERROR("unreachable code");
    }

    const T &get(InheritanceType Type) const
    {
        switch (Type)
        {
        case InheritanceType::Private:
            return Private;
        case InheritanceType::Protected:
            return Protected;
        case InheritanceType::Public:
            return Public;
        case InheritanceType::Interface:
            return Interface;
        default:
            return data[Type];
        }
        throw SW_RUNTIME_ERROR("unreachable code");
    }

    template <class U>
    void inheritance(const InheritanceGroup<U> &g, const GroupSettings &s = GroupSettings())
    {
        // Private
        if (s.has_same_parent)
            Private.merge(g.Protected);
        Private.merge(g.Public);
        Private.merge(g.Interface);

        // Protected
        if (s.has_same_parent)
            Protected.merge(g.Protected);
        Protected.merge(g.Public);
        Protected.merge(g.Interface);

        // Public
        if (s.has_same_parent)
            Protected.merge(g.Protected);
        Public.merge(g.Public);
        Public.merge(g.Interface);
    }

    template <class F>
    void iterate(F &&f) const
    {
        for (int i = toIndex(InheritanceType::Min); i < toIndex(InheritanceType::Max); i++)
        {
            auto s = getInheritanceStorage().raw()[i];
            if (s)
                f(*s, (InheritanceType)i);
        }
    }

    // merge to T, always w/o interface
    void merge(const GroupSettings &s = GroupSettings())
    {
        T::merge(Protected, s);
        T::merge(Public, s);
    }

    // merge from other group, always w/ interface
    template <class U>
    void merge(const InheritanceGroup<U> &g, const GroupSettings &s = GroupSettings())
    {
        T::merge(g.Protected, s);
        T::merge(g.Public, s);
        T::merge(g.Interface, s);
    }

    InheritanceStorage<T> &getInheritanceStorage() { return data; }
    const InheritanceStorage<T> &getInheritanceStorage() const { return data; }
};

}

namespace std
{

template<> struct hash<sw::Definition>
{
    size_t operator()(const sw::Definition& d) const
    {
        return std::hash<decltype(d.d)>()(d.d);
    }
};

}
