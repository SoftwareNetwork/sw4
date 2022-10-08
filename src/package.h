// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include "helpers.h"

namespace sw {

struct istring : string {
    using string::string;
    using string::operator=;

     //std::strong_ordering operator<=>(const istring &rhs) const { return stricmp(data(), rhs.data()); }
};

struct package_name {
    //std::vector<istring> elements;
    std::vector<string> elements;

    package_name() {
    }
    package_name(const string &s) : elements{s} {
    }

    operator string() const {
        if (!elements.empty()) {
            return elements[0];
        }
        return "";
    }
};

auto split_string(const string &s, string_view split) {
    std::vector<string_view> r;
    size_t p = 0;
    size_t pold = 0;
    while ((p = s.find(split, pold)) != -1) {
        r.emplace_back(s.data() + pold, s.data() + p);
        pold = p + split.size();
    }
    r.emplace_back(s.data() + pold, s.data() + s.size());
    return r;
}

struct package_version {
    struct number_version {
        struct numbers {
            std::vector<int> value;

            numbers() = default;
            numbers(const std::initializer_list<int> &s) : value{s} {}
            numbers(const string &s) {
                std::ranges::copy(split_string(s, "."sv) | std::views::transform([&](auto &&v) {
                    int i;
                    auto [_,ec] = std::from_chars(v.data(), v.data() + v.size(), i);
                    if (ec != std::errc{}) {
                        throw std::runtime_error{"bad version"};
                    }
                    return i;
                }), std::back_inserter(value));
            }

            bool operator<(const numbers &rhs) const {
                return std::tie(value) < std::tie(rhs.value);
            }
        };
        numbers elements;
        string extra;

        bool is_pre_release() const { return !extra.empty(); }
        bool is_release() const { return !is_pre_release(); }

        bool operator<(const number_version &rhs) const {
            if (is_release() && rhs.is_release()) {
                return elements < rhs.elements;
            } else if (is_release()) {
                return false;
            } else if (rhs.is_release()) {
                return true;
            } else {
                return std::tie(elements, extra) < std::tie(rhs.elements, rhs.extra);
            }
        }
    };
    using version_type = std::variant<number_version, string>;
    version_type version;

    package_version() : version{number_version{{0,0,1}}} {
    }
    package_version(const string &s) {
        if (s.empty()) {
            throw std::runtime_error{"empty version"};
        }
        auto is_alpha = [](char c) {
            return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_';
        };
        auto alnum = std::ranges::all_of(s, [&](auto &&c) {
            return c >= '0' && c <= '9' || is_alpha(c);
        });
        if (alnum && is_alpha(s[0] == '_')) {
            version = s;
        } else {
            version = number_version{s};
        }
    }
    package_version(const version_type &s) : version{s} {
    }

    bool is_pre_release() const {
        return std::holds_alternative<number_version>(version) && std::get<number_version>(version).is_pre_release();
    }
    bool is_release() const {
        return std::holds_alternative<number_version>(version) && std::get<number_version>(version).is_release();
    }
    bool is_version() const {
        return std::holds_alternative<number_version>(version);
    }
    bool is_branch() const {
        return std::holds_alternative<string>(version);
    }

    bool operator<(const package_version &rhs) const {
        if (is_version() && rhs.is_version()) {
            return std::tie(version) < std::tie(rhs.version);
        } else if (is_version()) {
            return false;
        } else if (rhs.is_version()) {
            return true;
        } else {
            return std::tie(version) < std::tie(rhs.version);
        }
    }
};

struct package_id {
    package_name name;
    package_version version;

    package_id() = default;
    package_id(const string &s) : name{s} {
    }
    package_id(const string &p, const string &v) : name{p}, version{v} {
    }
    void operator=(const string &s) {
        name = s;
    }

    operator string() const { return name; }
};

}
