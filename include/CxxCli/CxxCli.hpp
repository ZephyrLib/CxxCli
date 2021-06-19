
#ifndef HEADER_CXXCLI
#define HEADER_CXXCLI 1

#include <tuple>
#include <utility>

#ifndef CXXCLI_INDENT_CHAR_VALUE
#define CXXCLI_INDENT_CHAR_VALUE ' '
#endif
#ifndef CXXCLI_INDENT_LENGTH
#define CXXCLI_INDENT_LENGTH 4
#endif

namespace CxxCli {

    struct UsageAsList_t {} static constexpr UsageAsList{};

    template<typename docStr_t> struct Documentation_t { docStr_t m_doc; };
    template<> struct Documentation_t<void> {};
    template<typename doc_t>
    static constexpr auto Doc(doc_t doc) -> Documentation_t<doc_t> { return Documentation_t<doc_t>{ std::move(doc) }; }
    static constexpr auto Doc() -> Documentation_t<void> { return Documentation_t<void>{}; }

}

#include "CxxCli.Templates.inl"

namespace CxxCli {

    /*
    * The object is valid for as long as the command object that produced it
    */
    struct ParseResult {
    private:
        bool m_successfull = false;
        templates::parseResult m_data;

    public:
        ParseResult(bool successfull = false, templates::parseResult data = { 0 }) : m_successfull(successfull), m_data(data) {}

        void printUsage(std::ostream & out, const char * executableName) {
            if (m_data.m_printUsage != nullptr) {
                out << "Usage: " << executableName << ' ';
                m_data.m_printUsage(m_data.m_object, out);
            }
        }

        operator bool() const noexcept { return m_successfull; }

    };

    namespace templates {

        template<typename sub_t>
        struct Command_t {
        private:
            sub_t m_sub;

        public:

            constexpr Command_t(sub_t sub) : m_sub(std::move(sub)) {}

            /*
            * Parses input
            *
            * Expects raw arguments without current executable:
            * int main(int argc, char ** argv) {
            *   CxxCli::Command_t cmd = ... ;
            *   auto r = cmd.parse(argc - 1, argv + 1);
            *   ...
            * }
            */
            ParseResult parse(
                // arg count
                int argc,
                // argument values
                const char * const * argv
            ) const {
                parseResult result = {
                    &m_sub,
                    templates::invokePrintUsage0<sub_t>
                };
                int i = 0;
                if (m_sub.parse(&result, i, argc, argv) == ret::ok && i == argc) {
                    return ParseResult(true, result);
                }
                //result.errorMessage = "extra arguments";
                return ParseResult(false, result);
            }

            void printUsage(std::ostream & out, const char * executableName) const {
                out << "Usage: " << executableName << ' ';
                m_sub.printUsage(out, 1);
            }

        };

    }

    /*
    * Creates a const object
    *
    * A const will try compare an argument to the specified value, failing if comparison is not identical
    */
    constexpr auto Const(const char * value) -> templates::Const_t { return templates::Const_t(value); }

    /*
    * Creates a variable object
    *
    * A variable will map any argument given
    */
    constexpr auto Var(const char * identifier = "") -> templates::Var_t { return templates::Var_t(identifier); }

    /*
    * Creates a sequence object
    *
    * A sequence will try to match the arguments with the specified subojects, failing if any is mismatched
    */
    template<typename ... subs_t>
    constexpr auto Sequence(subs_t ... subs) -> templates::Sequence_t<void, false, subs_t...> {
        return templates::Sequence_t<void, false, subs_t...>(Documentation_t<void>{}, std::tuple<subs_t...>{std::forward<subs_t>(subs)...});
    }

    /*
    * Creates an optional object
    *
    * An option will try to match the arguments
    */
    template<typename ... subs_t>
    constexpr auto Optional(subs_t ... subs) -> templates::Optional_t<void, false, subs_t...> {
        return templates::Optional_t<void, false, subs_t...>(Documentation_t<void>{}, std::tuple<subs_t...>{std::forward<subs_t>(subs)...});
    }

    /*
    * Creates a loop object
    *
    * A loop will loop over the arguments until the next argument fails, returning a success
    */
    template<typename sub_t>
    constexpr auto Loop(sub_t sub) -> templates::Loop_t<sub_t> { return templates::Loop_t<sub_t>(std::move(sub)); }

    /*
    * Creates a branch object with the specified subobjects
    *
    * A branch will attempt to match all the supplied arguments iteratively for each subobject
    * Stops immediately when a suboject successfully parses
    */
    template<typename ... subs_t>
    constexpr auto Branch(subs_t ... subs) -> templates::Branch_t<subs_t...> { return templates::Branch_t<subs_t...>(std::move(subs)...); }

    /*
    * Creates command object
    */
    template<typename sub_t>
    constexpr auto Command(sub_t sub) -> templates::Command_t<sub_t> { return templates::Command_t<sub_t>(std::move(sub)); }

}

#endif
