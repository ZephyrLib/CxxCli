
#include <utility>
#include <tuple>
//#include <type_traits>
#include <cstring>

namespace CxxCli {

    struct ParseResult {
        bool successfull = false;
        const char * errorMessage = "";
        const char * usageMessage = "";
        operator bool() const noexcept { return successfull; }
    };

    namespace templates {

        enum struct ret { ok = 1, mismatch, error };

        namespace callback {

            template<typename fn_t, typename sub_t>
            struct Callback_t {
                fn_t m_fn;
                sub_t m_sub;
                constexpr Callback_t(fn_t && fn, sub_t sub) : m_fn(std::move(fn)), m_sub(std::move(sub)) {}

                ret parse(ParseResult * r, int & i, int argc, const char ** argv) const {
                    return m_sub.parse(r, i, argc, argv, m_fn);
                }
            };

            struct nullcb {
                void operator()() const noexcept {}
                void operator()(const char *) const noexcept {}
            };

        }

        namespace const0 {

            struct Const_t {
                const char * m_value;
                constexpr Const_t(const char * value) : m_value(value) {}

                template<typename fn_t = callback::nullcb>
                ret parse(ParseResult * r, int & i, int argc, const char ** argv, const fn_t & cb = callback::nullcb()) const {
                    if (i >= argc) {
                        r->errorMessage = "index out of bounds";
                        r->usageMessage = "";
                        return ret::mismatch;
                    }
                    if (std::strcmp(m_value, argv[i]) != 0) { return ret::mismatch; }
                    cb();
                    ++i;
                    return ret::ok;
                }

                template<typename fn_t>
                friend callback::Callback_t<fn_t, Const_t> operator>>(Const_t && c, fn_t && fn) {
                    return callback::Callback_t<fn_t, Const_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace var {

            struct Var_t {
                const char * m_identifier;
                constexpr Var_t(const char * id) : m_identifier(id) {}

                template<typename fn_t = callback::nullcb>
                ret parse(ParseResult * r, int & i, int argc, const char ** argv, const fn_t & cb = callback::nullcb{}) const {
                    if (i >= argc) {
                        r->errorMessage = "index out of bounds";
                        r->usageMessage = "";
                        return ret::mismatch;
                    }
                    cb(argv[i]);
                    ++i;
                    return ret::ok;
                }

                template<typename fn_t>
                friend callback::Callback_t<fn_t, Var_t> operator>>(Var_t && v, fn_t && fn) {
                    return callback::Callback_t<fn_t, Var_t>(std::move(fn), std::move(v));
                }
            };

        }

        namespace sequence {

            template<int I, int L, typename ... sub_t>
            struct parse_t {
                static ret parse(const std::tuple<sub_t...> & subs, ParseResult * r, int & i, int argc, const char ** argv) {
                    auto retVal = std::get<I>(subs).parse(r, i, argc, argv);
                    if (retVal != ret::ok) { return I == 0 ? ret::mismatch : (/*strictMatch*/true ? ret::error : retVal); }
                    if (I + 1 < L) {
                        return parse_t<I + 1, L, sub_t...>::parse(subs, r, i, argc, argv);
                    } else {
                        return ret::ok;
                    }
                }
            };

            template<int L, typename ... sub_t>
            struct parse_t<L, L, sub_t...> {
                static ret parse(const std::tuple<sub_t...> &, ParseResult *, int, int, const char **) {
                    return ret::ok;
                }
            };

            template<typename ... subs_t>
            struct Sequence_t {
                std::tuple<subs_t...> m_subs;
                constexpr Sequence_t(subs_t && ... subs) : m_subs(std::move(subs)...) {}

                template<typename fn_t = callback::nullcb>
                ret parse(ParseResult * r, int & i, int argc, const char ** argv, const fn_t & cb = callback::nullcb{}) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), subs_t...>::parse(m_subs, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        i = j;
                        cb();
                    }
                    return retVal;
                }

                template<typename fn_t>
                friend callback::Callback_t<fn_t, Sequence_t> operator>>(Sequence_t && c, fn_t && fn) {
                    return callback::Callback_t<fn_t, Sequence_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace optional {

            template<int I, int L, typename ... sub_t>
            struct parse_t {
                static ret parse(const std::tuple<sub_t...> & subs, ParseResult * r, int & i, int argc, const char ** argv) {
                    auto retVal = std::get<I>(subs).parse(r, i, argc, argv);
                    if (retVal != ret::ok) { return ret::mismatch; }
                    if (I + 1 < L) {
                        return parse_t<I + 1, L, sub_t...>::parse(subs, r, i, argc, argv);
                    } else {
                        return ret::ok;
                    }
                }
            };

            template<int L, typename ... sub_t>
            struct parse_t<L, L, sub_t...> {
                static ret parse(const std::tuple<sub_t...> &, ParseResult *, int, int, const char **) {
                    return ret::ok;
                }
            };

            template<typename ... subs_t>
            struct Optional_t {
                std::tuple<subs_t...> m_subs;
                constexpr Optional_t(subs_t && ... subs) : m_subs(std::move(subs)...) {}

                template<typename fn_t = callback::nullcb>
                ret parse(ParseResult * r, int & i, int argc, const char ** argv, const fn_t & cb = callback::nullcb{}) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), subs_t...>::parse(m_subs, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        i = j;
                        cb();
                    }
                    return ret::ok;
                }

                template<typename fn_t>
                friend callback::Callback_t<fn_t, Optional_t> operator>>(Optional_t && c, fn_t && fn) {
                    return callback::Callback_t<fn_t, Optional_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace loop {

            template<typename ... subs_t>
            struct Loop_t {
                sequence::Sequence_t<subs_t...> m_sub;
                constexpr Loop_t(subs_t && ... subs) : m_sub(std::move(subs)...) {}

                template<typename fn_t = callback::nullcb>
                ret parse(ParseResult * r, int & i, int argc, const char ** argv, const fn_t & cb = callback::nullcb{}) const {
                    while (true) {
                        auto j = i;
                        auto retVal = m_sub.parse(r, j, argc, argv);
                        if (retVal != ret::ok || i == j) { return ret::ok; }
                        i = j;
                        cb();
                    }
                }

                template<typename fn_t>
                friend callback::Callback_t<fn_t, Loop_t> operator>>(Loop_t && c, fn_t && fn) {
                    return callback::Callback_t<fn_t, Loop_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace branch {

            template<int I, int L, typename ... sub_t>
            struct parse_t {
                static ret parse(const std::tuple<sub_t...> & subs, ParseResult * r, int & i, int argc, const char ** argv) {
                    auto retVal = std::get<I>(subs).parse(r, i, argc, argv);
                    if (retVal == ret::mismatch) {
                        return parse_t<I + 1, L, sub_t...>::parse(subs, r, i, argc, argv);
                    }
                    return retVal;
                }
            };

            template<int L, typename ... subs_t>
            struct parse_t<L, L, subs_t...> {
                static ret parse(const std::tuple<subs_t...> &, ParseResult * r, int, int, const char **) {
                    r->usageMessage = "";
                    r->errorMessage = "illegal branch state";
                    return ret::error;
                }
            };

            template<typename ... subs_t>
            struct Branch_t {
                std::tuple<subs_t...> m_subs;
                constexpr Branch_t(subs_t && ... subs) : m_subs(std::move(subs)...) {}

                template<typename fn_t = callback::nullcb>
                ret parse(ParseResult * r, int & i, int argc, const char ** argv, const fn_t & cb = callback::nullcb{}) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), subs_t...>::parse(m_subs, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        i = j;
                        cb();
                    }
                    return retVal;
                }

                template<typename fn_t>
                friend callback::Callback_t<fn_t, Branch_t> operator>>(Branch_t && c, fn_t && fn) {
                    return callback::Callback_t<fn_t, Branch_t>(std::move(fn), std::move(c));
                }

            };

        }

        template<typename sub_t>
        struct Command_t {
        private:
            sub_t m_sub;

        public:

            constexpr Command_t(sub_t && sub) : m_sub(std::move(sub)) {}

            ParseResult parse(int argc, const char ** argv) const {
                ParseResult result;
                int i = 0;
                result.successfull = m_sub.parse(&result, i, argc, argv) == ret::ok;
                if (result.successfull && i != argc) {
                    result.successfull = false;
                    result.errorMessage = "extra arguments";
                }
                return result;
            }
        };

    }

}

namespace CxxCli {

    constexpr auto Const(const char * value) -> templates::const0::Const_t { return templates::const0::Const_t(value); }

    constexpr auto Var(const char * identifier = "") -> templates::var::Var_t { return templates::var::Var_t(identifier); }

    template<typename ... subs_t>
    constexpr auto Sequence(subs_t && ... subs) -> templates::sequence::Sequence_t<subs_t...> { return templates::sequence::Sequence_t<subs_t...>(std::move(subs)...); }

    template<typename ... subs_t>
    constexpr auto Optional(subs_t && ... subs) -> templates::optional::Optional_t<subs_t...> { return templates::optional::Optional_t<subs_t...>(std::move(subs)...); }

    template<typename ... subs_t>
    constexpr auto Loop(subs_t && ... subs) -> templates::loop::Loop_t<subs_t...> { return templates::loop::Loop_t<subs_t...>(std::move(subs)...); }

    template<typename ... subs_t>
    constexpr auto Branch(subs_t && ... subs) -> templates::branch::Branch_t<subs_t...> { return templates::branch::Branch_t<subs_t...>(std::move(subs)...); }

    template<typename sub_t>
    constexpr auto Command(sub_t && sub) -> templates::Command_t<sub_t> { return templates::Command_t<sub_t>(std::move(sub)); }

}
