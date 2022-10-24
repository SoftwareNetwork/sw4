// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include "rule.h"

namespace sw {

// basic target: throw files, rules etc.
struct rule_target : files_target {
    using base = files_target;

    using base::base;
    using base::operator+=;
    using base::add;
    using base::remove;

    path binary_dir;
    std::vector<rule> rules;
    std::map<path, rule_flag> processed_files;
    std::vector<command> commands;

    void add(const rule &r) {
        rules.push_back(r);
    }
    void init_rules(this auto &&self) {
        for (auto &&r : self.rules) {
            std::visit([&](auto &&v){
                if constexpr (requires {v(self);}) {
                    v(self);
                }
            }, r);
            for (auto &&c : self.commands) {
                visit(c, [&](auto &&c) {
                    for (auto &&o : c.outputs) {
                        self.processed_files[o];
                    }
                });
            }
        }
    }

    void prepare(this auto &&self) {
        self.init_rules();
    }
    void build(this auto &&self) {
        self.prepare();

        command_executor ce;
        ce.run(self);
    }

    //auto visit()
};

struct native_target : rule_target {
    using base = rule_target;

    using base::operator+=;
    using base::add;
    using base::remove;

    compile_options_t compile_options;
    link_options_t link_options;

    native_target(auto &&id) : base{id} {
        *this += native_sources_rule{};
    }

    void add(const system_link_library &l) {
        link_options.system_link_libraries.push_back(l);
    }

    //void build() { operator()(); }
    //void run(){}
};

struct executable_target : native_target {
    using base = native_target;

    using base::base;

    path executable;

    void run(auto && ... args) {
        // make rule?
    }
};

using target = variant<files_target, rule_target, native_target, executable_target>;

} // namespace sw
