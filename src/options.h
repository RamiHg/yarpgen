/*
Copyright (c) 2017, Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <string>

namespace yarpgen {

// This structure stores all of the options, required for YARPGen
struct Options {
  // Yarpgen version supposed to be changed every time the generation algorithm
  // is changed, so version+seed should unambiguously correspond to generated
  // test.
  // TODO: with more extra parameters taken into account, like target platform
  // properties, limits, generation policies, and output language, we may want
  // to encode all this in the seed.
  std::string yarpgen_version = "1.2";

  std::string plane_yarpgen_version;

  // IDs for all supported language standards
  enum StandardID {
    C99,
    C11,
    MAX_CStandardID,
    CXX98,
    CXX03,
    CXX11,
    CXX14,
    CXX17,
    MAX_CXXStandardID,
  };

  // This map matches StandardIDs to string literals for them
  static const std::map<std::string, StandardID> str_to_standard;

  Options();
  bool is_c();
  bool is_cxx();

  StandardID standard_id;
  bool mode_64bit;
  bool single_file = true;

  bool include_valarray;
  bool include_vector;
  bool include_array;

  uint32_t max_arith_depth = 5;

  uint32_t min_scope_stmt_count = 5;
  uint32_t max_scope_stmt_count = 10;

  uint32_t max_cse_count = 5;
  uint32_t max_if_depth = 3;

  uint32_t min_struct_type_count = 0;
  uint32_t max_struct_type_count = 6;
  uint32_t min_inp_struct_count = 0;
  uint32_t max_inp_struct_count = 6;
  uint32_t min_mix_struct_count = 0;
  uint32_t max_mix_struct_count = 6;
  uint32_t min_out_struct_count = 0;
  uint32_t max_out_struct_count = 8;

  bool enable_arrays = true;
  bool enable_bit_fields = false;
  bool print_assignments = false;
};

extern Options *options;
}  // namespace yarpgen
