/*
Copyright (c) 2015-2017, Intel Corporation

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

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>

#include "gen_policy.h"
#include "options.h"
#include "program.h"
#include "sym_table.h"
#include "type.h"
#include "util.h"
#include "variable.h"

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION ""
#endif

using namespace yarpgen;

void printVersion() {
  std::cout << "yarpgen version " + options->yarpgen_version + " (build "
            << BUILD_VERSION << " on " << BUILD_DATE << ")" << std::endl;
}

extern void self_test();

bool option_starts_with(char *option, const char *test) {
  return !strncmp(option, test, strlen(test));
}

// This function prints out optional error_message, help and exits
void print_usage_and_exit(std::string error_msg = "") {
  int exit_code = 0;
  if (error_msg != "") {
    std::cerr << error_msg << std::endl;
    exit_code = -1;
  }
  std::cout << std::endl;
  printVersion();
  std::cout << "usage: yarpgen\n";
  std::cout << "\t-q                        Quiet mode\n";
  std::cout << "\t-v, --version             Print yarpgen version\n";
  std::cout << "\t-d, --out-dir=<out-dir>   Output directory\n";
  std::cout << "\t-s, --seed=<seed>         Predefined seed (it is accepted in "
               "form of SSS or VV_SSS)\n";
  std::cout << "\t-m, --bit-mode=<32/64>    Generated test's bit mode\n";
  std::cout
      << "\t--std=<standard>          Generated test's language standard\n";
  auto search_for_default_std =
      [](const std::pair<std::string, Options::StandardID> &pair) {
        return pair.second == options->standard_id;
      };
  auto search_res =
      std::find_if(Options::str_to_standard.begin(),
                   Options::str_to_standard.end(), search_for_default_std);
  assert(search_res != Options::str_to_standard.end() &&
         "Can't match default standard_id and string");
  std::cout << "\t\t\t\t  Default: " << search_res->first << "\n";
  std::string all_standatds = "\t\t\t\t  Possible variants are:";
  for (const auto &iter : Options::str_to_standard)
    all_standatds += " " + iter.first + ",";
  all_standatds.pop_back();
  std::cout << all_standatds << std::endl;
  exit(exit_code);
}

// This function handles command-line options in form of "-short_arg <value>"
// and performs action(<value>)
bool parse_short_args(int argc, int &argv_iter, char **&argv,
                      std::string short_arg, std::function<void(char *)> action,
                      std::string error_msg) {
  if (!strcmp(argv[argv_iter], short_arg.c_str())) {
    if (++argv_iter == argc)
      print_usage_and_exit(error_msg);
    else {
      action(argv[argv_iter]);
      return true;
    }
  }
  return false;
}

// This function handles command-line options in form of "--long_arg=<value>"
// and performs action(<value>)
bool parse_long_args(int &argv_iter, char **&argv, std::string long_arg,
                     std::function<void(char *)> action,
                     std::string error_msg) {
  if (option_starts_with(argv[argv_iter], (long_arg + "=").c_str())) {
    size_t option_end = strlen((long_arg + "=").c_str());
    if (strlen(argv[argv_iter]) == option_end)
      print_usage_and_exit(error_msg);
    else {
      action(argv[argv_iter] + strlen((long_arg + "=").c_str()));
      return true;
    }
  }
  return false;
}

bool parse_long_and_short_args(int argc, int &argv_iter, char **&argv,
                               std::string short_arg, std::string long_arg,
                               std::function<void(char *)> action,
                               std::string error_msg) {
  return parse_long_args(argv_iter, argv, long_arg, action, error_msg) ||
         parse_short_args(argc, argv_iter, argv, short_arg, action, error_msg);
}

int main(int argc, char *argv[128]) {
  options = new Options;
  uint64_t seed = 0;
  std::string out_dir = "./";
  bool quiet = false;

  // Utility functions. They are necessary for copy-paste reduction. They
  // perform main actions during option parsing. Detects output directory
  auto out_dir_action = [&out_dir](std::string arg) { out_dir = arg; };

  // Detects predefined seed
  auto seed_action = [&seed](std::string arg) {
    std::stringstream arg_ss;

    if (arg.size() > 2 && arg[2] == '_') {
      if (arg.substr(0, 2) != options->plane_yarpgen_version) {
        ERROR("Incompatible yarpgen version in seed: " + arg);
      }
      arg_ss = std::stringstream(arg.substr(3));
    } else {
      arg_ss = std::stringstream(arg);
    }
    
    arg_ss >> seed;
  };

  // Detects YARPGen bit_mode
  auto bit_mode_action = [](std::string arg) {
    size_t *pEnd = nullptr;
    try {
      uint64_t bit_mode_arg = std::stoull(arg, pEnd, 10);
      if (bit_mode_arg == 32)
        options->mode_64bit = false;
      else if (bit_mode_arg == 64)
        options->mode_64bit = true;
      else
        print_usage_and_exit("Can't recognize bit mode: " + std::string(arg));
    } catch (std::invalid_argument &e) {
      print_usage_and_exit("Can't recognize bit mode: " + arg);
    }
  };

  // Detects desired language standard
  auto standard_action = [](std::string arg) {
    std::string search_str = arg;
    auto search_res = Options::str_to_standard.find(search_str);
    if (search_res != Options::str_to_standard.end())
      options->standard_id = search_res->second;
    else {
      print_usage_and_exit(
          "Can't recognize language standard: --std=" + search_str + "\n");
    }
  };

  auto max_arith_depth = [](std::string arg) {
    options->max_arith_depth = std::stoul(arg);
  };
  auto min_scope_stmt_count = [](std::string arg) {
    options->min_scope_stmt_count = std::stoul(arg);
  };
  auto max_scope_stmt_count = [](std::string arg) {
    options->max_scope_stmt_count = std::stoul(arg);
  };
  auto max_cse_count = [](std::string arg) {
    options->max_cse_count = std::stoul(arg);
  };
  auto max_if_depth = [](std::string arg) {
    options->max_if_depth = std::stoul(arg);
  };
  auto enable_arrays = [](std::string arg) {
    options->enable_arrays = std::stoul(arg) != 0;
  };
  auto enable_bit_fields = [](std::string arg) {
    options->enable_bit_fields = std::stoul(arg) != 0;
  };
  auto print_assignments = [](std::string arg) {
    options->print_assignments = std::stoul(arg) != 0;
  };

#define PARSE_NUM(name)                                                  \
  else if (parse_long_args(                                              \
               i, argv, "--" #name,                                      \
               [](std::string arg) { options->name = std::stoul(arg); }, \
               "Invalid " #name))

  // Main loop for parsing command-line options
  for (int i = 0; i < argc; ++i) {
    if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
      print_usage_and_exit();
    } else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
      printVersion();
      exit(0);
    } else if (!strcmp(argv[i], "-q")) {
      quiet = true;
    } else if (parse_long_args(i, argv, "--std", standard_action,
                               "Can't recognize language standard:")) {
    } else if (parse_long_and_short_args(
                   argc, i, argv, "-d", "--out-dir", out_dir_action,
                   "Output directory wasn't specified.")) {
    } else if (parse_long_and_short_args(argc, i, argv, "-s", "--seed",
                                         seed_action,
                                         "Seed wasn't specified.")) {
    } else if (parse_long_and_short_args(argc, i, argv, "-m", "--bit-mode",
                                         bit_mode_action,
                                         "Can't recognize bit mode:")) {
    } else if (parse_long_args(i, argv, "--max_arith_depth", max_arith_depth,
                               "Invalid")) {
    } else if (parse_long_args(i, argv, "--min_scope_stmt_count",
                               min_scope_stmt_count, "Invalid")) {
    } else if (parse_long_args(i, argv, "--max_scope_stmt_count",
                               max_scope_stmt_count, "Invalid")) {
    } else if (parse_long_args(i, argv, "--max_cse_count", max_cse_count,
                               "Invalid")) {
    } else if (parse_long_args(i, argv, "--max_if_depth", max_if_depth,
                               "Invalid")) {
    } else if (parse_long_args(i, argv, "--enable_arrays", enable_arrays,
                               "Invalid")) {
    } else if (parse_long_args(i, argv, "--enable_bit_fields",
                               enable_bit_fields, "Invalid")) {
    } else if (parse_long_args(i, argv, "--print_assignments",
                               print_assignments, "Invalid")) {
    }
    PARSE_NUM(min_struct_type_count) {}
    PARSE_NUM(max_struct_type_count) {}
    PARSE_NUM(min_inp_struct_count) {}
    PARSE_NUM(max_inp_struct_count) {}
    PARSE_NUM(min_mix_struct_count) {}
    PARSE_NUM(max_mix_struct_count) {}
    PARSE_NUM(min_out_struct_count) {}
    PARSE_NUM(max_out_struct_count) {}
    else if (argv[i][0] == '-') {
      print_usage_and_exit("Unknown option: " + std::string(argv[i]));
    }
  }

  if (argc == 1 && !quiet) {
    std::cerr << "Using default options" << std::endl;
    std::cerr << "For help type " << argv[0] << " -h" << std::endl;
  }

  rand_val_gen = std::make_shared<RandValGen>(RandValGen(seed));
  default_gen_policy.init_from_config();

  //    self_test();

  Program mas(out_dir);
  mas.generate();
  mas.emit_decl();
  mas.emit_func();
  mas.emit_main();

  delete (options);

  return 0;
}
