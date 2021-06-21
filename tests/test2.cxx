
#include <cassert>
#include <string>

#include <CxxCli/CxxCli.hpp>

#include "test_header.hxx"

static int test() {
    DECLARE_ARGS("--output-dir", "./out", "--target", "./target", "--src-name", "./source.cxx", "--header-name", "./header.hxx", "--identifier", "identifier")

    using namespace CxxCli;

    const char * identifier = nullptr;
    const char * target = nullptr;
    const char * src_name = nullptr;
    std::string header_name;
    const char * output_dir = nullptr;

    auto cmd = Command(
        Loop(
            Sequence(
                Optional(Sequence(Const("--identifier"), Var() >> &identifier)),
                Optional(Sequence(Const("--target"), Var() >> &target)),
                Optional(Sequence(Const("--src-name"), Var() >> &src_name)),
                Optional(Sequence(Const("--header-name"), Var() >> &header_name)),
                Optional(Sequence(Const("--output-dir"), Var() >> &output_dir))
            )
        )
    );

    auto result = cmd.parse(argc, argv);

    assert(result);

    assert(identifier == argv[9]);
    assert(target == argv[3]);
    assert(src_name == argv[5]);
    assert(header_name == argv[7]);
    assert(output_dir == argv[1]);

    return 0;
}

int main(int argc, char ** argv) { return test(); }

