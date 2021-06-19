
#include <iostream>

#include <CxxCli/CxxCli.hpp>

#include "test_header.hxx"

static int test() {
    DECLARE_ARGS("dataset-manager", "dataset", "create", "-l", "EN", "-i", "i0", "-i", "i1", "1")

    using namespace CxxCli;

    auto cmd = Command(
        Sequence(
            Const("dataset-manager"),
            Sequence(
                Branch(
                    Sequence(
                        Const("dataset"),
                        Branch(
                            Const("list") >> [&] { std::cout << "list" << std::endl; },
                            Sequence(Const("delete"), Var() >> [&] (const char * value) { std::cout << "delete " << value << std::endl; }),
                            Sequence(
                                Const("create"),
                                Optional(Const("-l"), Var() >> [&] (const char * value) { std::cout << "create -l=" << value << std::endl; }) >> [&] { std::cout << "use language" << std::endl; },
                                Loop(Sequence(Const("-i"), Var() >> [&] (const char * value) { std::cout << "create -i=" << value << std::endl; })),
                                Var() >> [&] (const char * value) { std::cout << "create " << value << std::endl; }
                            )
                        )
                    ),
                    Sequence(
                        Const("annotator"),
                        Branch(
                            Const("list"),
                            Sequence(Const("register"), Var("fname"), Var("lname")),
                            Sequence(Const("activate"), Var("uuid") >> [&] (const char * value) { std::cout << "annotator activate " << value << std::endl; }),
                            Sequence(Const("deactivate"), Var("uuid"))
                        )
                    )
                )
            )
        )
    );

    auto result = cmd.parse(argc, argv);

    result.printUsage(std::cout, "test1");

    if (result) { return 0; }

    return 1;
}

int main(int argc, char ** argv) { return test(); }
