
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
        details::parseResult m_data;

    public:
        ParseResult(bool successfull = false, details::parseResult data = { 0 }) : m_successfull(successfull), m_data(data) {}

        void printUsage(std::ostream & out, const char * executableName) {
            if (m_data.m_printUsage != nullptr) {
                out << "Usage: " << executableName << ' ';
                m_data.m_printUsage(m_data.m_object, out);
            }
        }

        operator bool() const noexcept { return m_successfull; }

    };

    namespace details {

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
                    details::invokePrintUsage0<sub_t>
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
    constexpr auto Const(const char * value) -> details::const_::Const_t { return details::const_::Const_t(value); }

    /*
    * Creates a variable object
    *
    * A variable will map any argument given
    */
    template<typename identifier_t>
    constexpr auto Var(identifier_t identifier) -> details::var::Var_t<identifier_t> { return details::var::Var_t<identifier_t>(std::forward<identifier_t>(identifier)); }
    constexpr auto Var() -> details::var::Var_t<void> { return details::var::Var_t<void>(); }

    /*
    * Creates a sequence object
    *
    * A sequence will try to match the arguments with the specified subojects, failing if any is mismatched
    */
    template<typename ... subs_t>
    constexpr auto Sequence(subs_t ... subs) -> details::sequence::Sequence_t<void, false, subs_t...> {
        return details::sequence::Sequence_t<void, false, subs_t...>(Documentation_t<void>{}, std::tuple<subs_t...>{std::forward<subs_t>(subs)...});
    }

    /*
    * Creates an optional object
    *
    * An option will try to match the arguments
    */
    template<typename sub_t>
    constexpr auto Optional(sub_t sub) -> details::optional::Optional_t<sub_t> {
        return details::optional::Optional_t<sub_t>(std::forward<sub_t>(sub));
    }

    /*
    * Creates a loop object
    *
    * A loop will loop over the arguments until the next argument fails, returning a success
    */
    template<typename sub_t>
    constexpr auto Loop(sub_t sub) -> details::loop::Loop_t<sub_t> { return details::loop::Loop_t<sub_t>(std::forward<sub_t>(sub)); }

    /*
    * Creates a branch object with the specified subobjects
    *
    * A branch will attempt to match all the supplied arguments iteratively for each subobject
    * Stops immediately when a suboject successfully parses
    */
    template<typename ... subs_t>
    constexpr auto Branch(subs_t ... subs) -> details::branch::Branch_t<subs_t...> { return details::branch::Branch_t<subs_t...>(std::forward<subs_t>(subs)...); }

    /*
    * Creates command object
    */
    template<typename sub_t>
    constexpr auto Command(sub_t sub) -> details::Command_t<sub_t> { return details::Command_t<sub_t>(std::forward<sub_t>(sub)); }

}

#endif
