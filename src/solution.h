#pragma once

#include "rule_target.h"
#include "os.h"
#include "input.h"

namespace sw {

struct solution {
    abspath source_dir{"."};
    abspath binary_dir{".sw4"};
    // config

    build_settings bs{build_settings::default_build_settings()};

    // internal data
    std::vector<target_ptr> targets_;
    std::vector<rule> rules;
    std::vector<input_with_settings> inputs;

    solution() {
        rules.push_back(cl_exe_rule{});
        rules.push_back(link_exe_rule{});
    }

    template <typename T, typename... Args>
    T &add(Args &&...args) {
        auto ptr = std::make_unique<T>(*this, FWD(args)...);
        auto &t = *ptr;
        targets_.emplace_back(std::move(ptr));
        return t;
    }

    void add_input(auto &&i) {
        inputs.emplace_back(input_with_settings{i, {build_settings::default_build_settings()}});
    }
    void add_input(input_with_settings &i) {
        inputs.emplace_back(i);
    }
    void add_input(const input_with_settings &i) {
        inputs.emplace_back(i);
    }

    auto &targets() {
        return targets_;
        //| std::views::transform([](auto &&v) -> decltype(auto) {
                   //return *v;
               //});
    }
    void prepare() {
        for (auto &&t : targets()) {
            visit(t, [&](auto &&vp) {
                auto &v = *vp;
                if constexpr (std::derived_from<std::decay_t<decltype(v)>, rule_target>) {
                    for (auto &&r : rules) {
                        v += r;
                    }
                }
                if constexpr (requires { v.prepare(); }) {
                    v.prepare();
                }
            });
        }
    }
    void build() {
        executor ex;
        build(ex);
    }
    void build(auto &&ex) {
        for (auto &&i : inputs) {
            i(*this);
        }

        prepare();

        command_executor ce{ex};
        for (auto &&t : targets()) {
            visit(t, [&](auto &&vp) {
                auto &v = *vp;
                if constexpr (requires { v.commands; }) {
                    ce += v.commands;
                }
            });
        }
        ce.run();
    }
};

} // namespace sw
