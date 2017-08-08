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

#include "program.h"

///////////////////////////////////////////////////////////////////////////////

using namespace yarpgen;

Program::Program (std::string _out_folder) {
    out_folder = _out_folder;
    extern_inp_sym_table = std::make_shared<SymbolTable> ();
    extern_mix_sym_table = std::make_shared<SymbolTable> ();
    extern_out_sym_table = std::make_shared<SymbolTable> ();
}

void Program::generate () {
    Context ctx (gen_policy, nullptr, Node::NodeID::MAX_STMT_ID, true);
    ctx.set_extern_inp_sym_table (extern_inp_sym_table);
    ctx.set_extern_mix_sym_table (extern_mix_sym_table);
    ctx.set_extern_out_sym_table (extern_out_sym_table);
    std::shared_ptr<Context> ctx_ptr = std::make_shared<Context>(ctx);
    form_extern_sym_table(ctx_ptr);

    function = ScopeStmt::generate(ctx_ptr);
}

// This function initially fills extern symbol table with inp and mix variables. It also creates type structs definitions.
void Program::form_extern_sym_table(std::shared_ptr<Context> ctx) {
    auto p = ctx->get_gen_policy();
    // Allow const cv-qualifier in gen_policy, pass it to new Context
    std::shared_ptr<Context> const_ctx = std::make_shared<Context>(*(ctx));
    GenPolicy const_gen_policy = *(const_ctx->get_gen_policy());
    const_gen_policy.set_allow_const(true);
    const_ctx->set_gen_policy(const_gen_policy);
    // Generate random number of random input variables
    uint32_t inp_var_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_inp_var_count(), p->get_max_inp_var_count());
    for (uint32_t i = 0; i < inp_var_count; ++i) {
        ctx->get_extern_inp_sym_table()->add_variable(ScalarVariable::generate(const_ctx));
    }
    //TODO: add to gen_policy
    // Same for mixed variables
    uint32_t mix_var_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_mix_var_count(), p->get_max_mix_var_count());
    for (uint32_t i = 0; i < mix_var_count; ++i) {
        ctx->get_extern_mix_sym_table()->add_variable(ScalarVariable::generate(ctx));
    }

    uint32_t struct_type_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_struct_type_count(), p->get_max_struct_type_count());
    if (struct_type_count == 0)
        return;

    // Create random number of struct definition
    for (uint32_t i = 0; i < struct_type_count; ++i) {
        //TODO: Maybe we should create one container for all struct types? And should they all be equal?
        std::shared_ptr<StructType> struct_type = StructType::generate(ctx, ctx->get_extern_inp_sym_table()->get_struct_types());
        ctx->get_extern_inp_sym_table()->add_struct_type(struct_type);
        ctx->get_extern_out_sym_table()->add_struct_type(struct_type);
        ctx->get_extern_mix_sym_table()->add_struct_type(struct_type);
    }

    // Create random number of input structures
    uint32_t inp_struct_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_inp_struct_count(), p->get_max_inp_struct_count());
    for (uint32_t i = 0; i < inp_struct_count; ++i) {
        uint32_t struct_type_indx = rand_val_gen->get_rand_value<uint32_t>(0, struct_type_count - 1);
        ctx->get_extern_inp_sym_table()->add_struct(Struct::generate(const_ctx, ctx->get_extern_inp_sym_table()->get_struct_types().at(struct_type_indx)));
    }
    // Same for mixed structures
    uint32_t mix_struct_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_mix_struct_count(), p->get_max_mix_struct_count());
    for (uint32_t i = 0; i < mix_struct_count; ++i) {
        uint32_t struct_type_indx = rand_val_gen->get_rand_value<uint32_t>(0, struct_type_count - 1);
        ctx->get_extern_mix_sym_table()->add_struct(Struct::generate(ctx, ctx->get_extern_mix_sym_table()->get_struct_types().at(struct_type_indx)));
    }
    // Same for output structures
    uint32_t out_struct_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_out_struct_count(), p->get_max_out_struct_count());
    for (uint32_t i = 0; i < out_struct_count; ++i) {
        uint32_t struct_type_indx = rand_val_gen->get_rand_value<uint32_t>(0, struct_type_count - 1);
        ctx->get_extern_out_sym_table()->add_struct(Struct::generate(ctx, ctx->get_extern_out_sym_table()->get_struct_types().at(struct_type_indx)));
    }
}

void Program::write_file (std::string of_name, std::string data) {
    std::ofstream out_file;
    out_file.open (out_folder + "/" + of_name);
    out_file << data;
    out_file.close ();
}

static std::string get_file_ext () {
    if (options->is_c())
        return "c";
    else if (options->is_cxx())
        return "cpp";
    std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can't detect language subset" << std::endl;
    exit(-1);
}

void Program::emit_init () {
    std::ofstream out_file;
    out_file.open(out_folder + "/" + "init." + get_file_ext());
    out_file << "#include \"init.h\"\n\n";

    extern_inp_sym_table->emit_variable_def(out_file);
    out_file << "\n\n";
    extern_mix_sym_table->emit_variable_def(out_file);
    out_file << "\n\n";
    extern_out_sym_table->emit_variable_def(out_file);
    out_file << "\n\n";
    extern_inp_sym_table->emit_struct_def(out_file);
    out_file << "\n\n";
    extern_mix_sym_table->emit_struct_def(out_file);
    out_file << "\n\n";
    extern_out_sym_table->emit_struct_def(out_file);
    out_file << "\n\n";
    //TODO: what if we extand struct types in mix_sym_table and out_sym_table
    extern_inp_sym_table->emit_struct_type_static_memb_def(out_file);
    out_file << "\n\n";

    out_file << "void init () {\n";
    extern_inp_sym_table->emit_struct_init (out_file, "    ");
    extern_mix_sym_table->emit_struct_init (out_file, "    ");
    extern_out_sym_table->emit_struct_init (out_file, "    ");
    out_file << "}";

    out_file.close();
}

void Program::emit_decl () {
    std::ofstream out_file;
    out_file.open(out_folder + "/" + "init.h");
    /* TODO: none of it is used currently.
     * All these headers must be added only when they are really needed.
     * Parsing these headers is costly for compile time
    stream << "#include <cstdint>\n";
    out_file << "#include <array>\n";
    out_file << "#include <vector>\n";
    out_file << "#include <valarray>\n\n";
    */

    out_file << "void hash(unsigned long long int *seed, unsigned long long int const v);\n\n";

    extern_inp_sym_table->emit_variable_extern_decl(out_file);
    out_file << "\n\n";
    extern_mix_sym_table->emit_variable_extern_decl(out_file);
    out_file << "\n\n";
    extern_out_sym_table->emit_variable_extern_decl(out_file);
    out_file << "\n\n";
    //TODO: what if we extand struct types in mix_sym_tabl
    extern_inp_sym_table->emit_struct_type_def(out_file);
    out_file << "\n\n";
    extern_inp_sym_table->emit_struct_extern_decl(out_file);
    out_file << "\n\n";
    extern_mix_sym_table->emit_struct_extern_decl(out_file);
    out_file << "\n\n";
    extern_out_sym_table->emit_struct_extern_decl(out_file);
    out_file << "\n\n";

    out_file.close();
}

void Program::emit_func () {
    std::ofstream out_file;
    out_file.open(out_folder + "/" + "func." + get_file_ext());
    out_file << "#include \"init.h\"\n\n";
    out_file << "void foo ()\n";
    function->emit(out_file);

    out_file.close();
}

void Program::emit_hash () {
    std::string ret = "void hash(unsigned long long int *seed, unsigned long long int const v) {\n";
    ret += "    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);\n";
    ret += "}\n";
    write_file("hash." + get_file_ext(), ret);
}

void Program::emit_check () { // TODO: rewrite with IR
    std::ofstream out_file;
    out_file.open(out_folder + "/" + "check." + get_file_ext());
    out_file << "#include \"init.h\"\n\n";

    out_file << "unsigned long long int checksum () {\n";

    std::shared_ptr<ScalarVariable> seed = std::make_shared<ScalarVariable>("seed", IntegerType::init(Type::IntegerTypeID::ULLINT));
    std::shared_ptr<VarUseExpr> seed_use = std::make_shared<VarUseExpr>(seed);

    BuiltinType::ScalarTypedVal zero_init (IntegerType::IntegerTypeID::ULLINT);
    zero_init.val.ullint_val = 0;
    std::shared_ptr<ConstExpr> const_init = std::make_shared<ConstExpr> (zero_init);

    std::shared_ptr<DeclStmt> seed_decl = std::make_shared<DeclStmt>(seed, const_init);

    seed_decl->emit(out_file, "    ");
    out_file << "\n";

    extern_mix_sym_table->emit_variable_check (out_file, "    ");
    extern_out_sym_table->emit_variable_check (out_file, "    ");

    extern_mix_sym_table->emit_struct_check (out_file, "    ");
    extern_out_sym_table->emit_struct_check (out_file, "    ");

    out_file << "    return seed;\n";
    out_file << "}";

    out_file.close();
}

void Program::emit_main () {
    std::string ret;
    ret += "#include <stdio.h>\n";
    ret += "#include \"init.h\"\n\n";
    ret += "extern void init ();\n";
    ret += "extern void foo ();\n";
    ret += "extern unsigned long long int checksum ();\n\n";
    ret += "int main () {\n";
    ret += "    init ();\n";
    ret += "    foo ();\n";
    ret += "    printf(\"%llu\\n\", checksum ());\n";
    ret += "    return 0;\n";
    ret += "}";
    write_file("driver." + get_file_ext(), ret);
}
