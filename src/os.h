// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include "helpers.h"

namespace sw {

namespace os {

struct windows {
    static constexpr auto name = "windows"sv;

    static constexpr auto executable_extension = ".exe";
    static constexpr auto object_file_extension = ".obj";
    static constexpr auto static_library_extension = ".lib";
    static constexpr auto shared_library_extension = ".dll";

    // deps:
    // kernel32 dependency (winsdk.um)
};

struct unix {
    static constexpr auto object_file_extension = ".o";
    static constexpr auto static_library_extension = ".a";
};

struct linux : unix {
    static constexpr auto name = "linux"sv;

    static constexpr auto shared_library_extension = ".so";
};

struct darwin : unix {
    static constexpr auto shared_library_extension = ".dylib";
};

struct macos : darwin {
    static constexpr auto name = "macos"sv;
};
// ios etc

} // namespace os

namespace arch {

struct x86 {
    static constexpr auto name = "x86"sv;
};
struct x64 {
    static constexpr auto name = "x64"sv;
};
using amd64 = x64;
using x86_64 = x64;

struct arm {
    static constexpr auto name = "arm"sv;
};
struct arm64 {
    static constexpr auto name = "arm64"sv;
};
using aarch64 = arm64;

} // namespace arch

namespace build_type {

struct debug {
    static constexpr auto name = "debug"sv;
};
struct minimum_size_release {
    static constexpr auto name = "minimum_size_release"sv;
};
struct release_with_debug_information {
    static constexpr auto name = "release_with_debug_information"sv;
};
struct release {
    static constexpr auto name = "release"sv;
};

} // namespace build_type

namespace library_type {

struct static_ {
    static constexpr auto name = "static"sv;
};
struct shared {
    static constexpr auto name = "shared"sv;
};

} // namespace library_type

struct build_settings {
    template <typename ... Types>
    struct special_variant : variant<Types...> {
        using base = variant<Types...>;
        using base::base;
        using base::operator=;

        template <typename T>
        bool is() const {
            return contains<T, Types...>();
        }
    };

    using os_type = special_variant<os::linux, os::macos, os::windows>;
    using arch_type = special_variant<arch::x86, arch::x64, arch::arm, arch::arm64>;
    using build_type_type = special_variant<build_type::debug, build_type::minimum_size_release,
        build_type::release_with_debug_information, build_type::release>;
    using library_type_type = special_variant<library_type::static_, library_type::shared>;

    os_type os;
    arch_type arch;
    build_type_type build_type;
    library_type_type library_type;
    // default compilers
    //

    auto for_each() const {
        return std::tie(os, arch, build_type, library_type);
    }

    void visit(auto &&...f) {
        std::apply([&](auto && ... args){
            auto f2 = [&](auto && a) {
                visit_any(a, FWD(f)...);
            };
            (f2(FWD(args)),...);
        }, for_each());
    }

    template <typename T>
    bool is1() {
        int cond = 0;
        std::apply(
            [&](auto &&...args) {
                auto f2 = [&](auto &&a) {
                    if (cond == 1) {
                        return;
                    }
                    if constexpr (contains<T>((std::decay_t<decltype(a)>**)nullptr)) {
                        cond += std::holds_alternative<T>(a);
                    }
                };
                (f2(FWD(args)), ...);
            },
            for_each());
        return cond == 1;
    }
    template <typename T, typename ... Types>
    bool is() {
        return (is1<T>() && ... && is1<Types>());
    }

    size_t hash() const {
        size_t h = 0;
        std::apply(
            [&](auto &&...args) {
                auto f2 = [&](auto &&a) {
                    ::sw::visit(a, [&](auto &&v) {
                        h ^= std::hash<string_view>()(v.name);
                    });
                };
                (f2(FWD(args)), ...);
            },
            for_each());
        return h;
    }

    static auto default_build_settings() {
        build_settings bs;
        // see more definitions at https://opensource.apple.com/source/WTF/WTF-7601.1.46.42/wtf/Platform.h.auto.html
#if defined(_WIN32)
        bs.os = os::windows{};
#elif defined(__APPLE__)
        bs.os = os::macos{};
#else
        bs.os = os::linux{};
#endif
#if defined(__x86_64__) || defined(_M_X64)
        bs.arch = arch::x64{};
#elif defined(__i386__) || defined(_M_IX86)
        bs.arch = arch::x86{};
#elif defined(__arm64__) || defined(__aarch64__)
        bs.arch = arch::arm64{};
#elif defined(__arm__)
        bs.arch = arch::arm{};
#endif
        bs.build_type = build_type::release{};
        bs.library_type = library_type::shared{};
        return bs;
    }
};

} // namespace sw
