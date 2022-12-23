// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include "helpers.h"
#include "os.h"

namespace sw {

struct solution;

// from source code
/// from some regular file
//InlineSpecification,
// entry point
struct source_code_input {
    std::function<void(solution &)> entry_point;

    void operator()(auto &sln, const build_settings &bs) {
        sln.bs = &bs;
        entry_point(sln);
    }
};

struct specification_file_input {
    path fn;
};

/// no input file, use some heuristics
struct directory_input {};

/// only try to find spec file
struct directory_specification_file_input {};

using input = variant<source_code_input>;
using entry_point = source_code_input;

struct input_with_settings {
    input i;
    std::set<build_settings> settings;

    void operator()(auto &sln) {
        for (auto &&s : settings) {
            visit(i, [&](auto &&v){v(sln, s);});
        }
    }
};

// in current dir
//void detect_inputs(){}

} // namespace sw
