
#include <iostream>

#include <CxxCli/CxxCli.hpp>

#include "test_header.hxx"

static int test() {
    DECLARE_ARGS("--output-dir", "./out", "--target", "./target", "--src-name", "./source.cxx", "--header-name", "./header.hxx", "--identifier", "identifier")

    using namespace CxxCli;

    auto cmd = Command(
        Loop(
            Optional(Const("--identifier"), Var() >> [&] (const char * v) { std::cout << "identifier=" << v << std::endl; }),
            Optional(Const("--target"), Var() >> [&] (const char * v) { std::cout << "target=" << v << std::endl; }),
            Optional(Const("--src-name"), Var() >> [&] (const char * v) { std::cout << "src-name=" << v << std::endl; }),
            Optional(Const("--header-name"), Var() >> [&] (const char * v) { std::cout << "header-name=" << v << std::endl; }),
            Optional(Const("--output-dir"), Var() >> [&] (const char * v) { std::cout << "output-dir=" << v << std::endl; })
        )
    );

    auto result = cmd.parse(argc, argv);

    if (result) { return 0; }
    std::cout << "errorMessage=[" << result.errorMessage << "]" << std::endl;
    std::cout << "usageMessage=[" << result.usageMessage << "]" << std::endl;
    return 1;
}

int main(int argc, char ** argv) { return test(); }

