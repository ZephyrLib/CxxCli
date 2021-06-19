
#include <tuple>
#include <cstdlib>
#include <utility>
#include <cstring>
#include <ostream>
#include <type_traits>

namespace CxxCli {

    namespace templates {

        template<typename char_t, char_t... values>
        struct string000 {
            static constexpr const char_t chars[sizeof...(values) + 1] = { values..., 0 };
        };

        template<typename char_t, char_t value, unsigned count, unsigned... indices>
        struct generate_param_pack_t {
            using result = typename generate_param_pack_t<char_t, value, count - 1, value, indices...>::result;
        };
        template<typename char_t, char_t value, unsigned... indices>
        struct generate_param_pack_t<char_t, value, 0, indices...> {
            using result = string000<char_t, indices...>;
        };

        template<typename char_t>
        using indent_t = typename generate_param_pack_t<char_t, (CXXCLI_INDENT_CHAR_VALUE), (CXXCLI_INDENT_LENGTH)>::result;
        using indent_char_t = indent_t<char>;

    }

    namespace templates {
        enum struct ret { ok = 1, mismatch, error };

        struct parseResult {
            const void * m_object;
            void (*m_printUsage)(const void *, std::ostream &);
        };

        template<typename x>
        static void invokePrintUsage0(const void * v, std::ostream & out) { static_cast<const x *>(v)->printUsage(out, 0); }

        /*template<typename, typename T>
        struct is_callable {
            static_assert(std::integral_constant<T, false>::value, "Second template parameter needs to be of function type.");
        };

        template<typename Ret, typename... Args>
        struct is_callable_checkImpl {
            template<typename T>
            static constexpr auto check(T *) -> typename std::is_same<decltype(std::declval<T>()(std::declval<Args>()...)), Ret>::type;
            template<typename>
            static constexpr std::false_type check(...);
        };
        template<typename... Args>
        struct is_callable_checkImpl<decltype(nullptr), Args...> {
            template<typename T>
            static constexpr auto check(T *) -> typename std::enable_if<true, decltype(std::declval<T>()(std::declval<Args>()...))>::type;
            template<typename>
            static constexpr std::false_type check(...);
        };

        template<typename C, typename Ret, typename... Args>
        struct is_callable<C, Ret(Args...)> {
            struct err_t {};
            template<typename T>
            static constexpr auto check(T *) -> decltype(std::declval<T>()(std::declval<Args>()...));
            template<typename>
            static constexpr err_t check(...);
            using ret_t = decltype(check<C>(0));
            static constexpr bool value = std::conditional<std::is_same<Ret, decltype(nullptr)>::value, typename std::conditional<std::is_same<ret_t, err_t>::value, std::false_type, std::true_type>::type, std::is_same<ret_t, Ret>>::type::value;
        };

        template<typename T, typename Fn_t>
        static constexpr bool is_callable_v = is_callable<T, Fn_t>::value;

        template<typename fn_t, typename externFn_t, typename sub_t>
        struct CallbackImpl_t {
            fn_t m_fn;
            sub_t m_sub;
            constexpr CallbackImpl_t(externFn_t fn, sub_t sub) : m_fn(std::move(fn)), m_sub(std::move(sub)) {}

            ret parse(parseResult * r, int & i, int argc, const char * const * argv) const { return m_sub.parse(r, i, argc, argv, m_fn); }
            void printUsage(std::ostream & out, int indent) const { m_sub.printUsage(out, indent); }
        };
        template<typename fn_t, typename sub_t>
        struct CallbackImpl_t<fn_t, void, sub_t>;

        template<typename fn_t, typename sub_t>
        using Callback_t = CallbackImpl_t<
            fn_t,
            typename std::conditional<is_callable_v<fn_t, decltype(nullptr)()>, fn_t,
                typename std::conditional<is_callable_v<fn_t, decltype(nullptr)(const char *)>, fn_t, void>::type
            >::type,
            sub_t
        >;*/

        template<typename scalar_t>
        struct lambdaScalarParser {
            scalar_t * m_target;

            bool operator()(const char * value) const {
                char * str_end;
                auto v = parse(value, &str_end);
                if ((str_end - value) != std::strlen(value)) { return false; }
                *m_target = v;
                return true;
            }

            template<typename x = scalar_t>
            static auto parse(char const * value, char ** end_ptr) -> typename std::enable_if<std::is_floating_point<x>::value, x>::type {
                return static_cast<x>(std::strtold(value, end_ptr));
            }

            template<typename x = scalar_t>
            static auto parse(char const * value, char ** end_ptr) -> typename std::enable_if<std::is_integral<x>::value && std::is_signed<x>::value, x>::type {
                return static_cast<x>(std::strtoll(value, end_ptr, 10));
            }

            template<typename x = scalar_t>
            static auto parse(char const * value, char ** end_ptr) -> typename std::enable_if<std::is_integral<x>::value && std::is_unsigned<x>::value, x>::type {
                return static_cast<x>(std::strtoull(value, end_ptr, 10));
            }

        };

        template<typename string_t>
        struct lambdaStringParser {
            string_t * m_target;
            bool operator()(const char * value) const { *m_target = value; return true; }
        };

        static void printIndent(std::ostream & out, int indent) { for (int i = 0; i < indent; ++i) { out << indent_char_t::chars; } }

        template<typename doc_t>
        struct printDoc2_t { static void print(std::ostream & out, const doc_t & doc, int) { out << doc; } };
        template<>
        struct printDoc2_t<const char *> {
            static void print(std::ostream & out, const char * doc, int indent) {
                bool nl = false;
                for (auto i = doc, j = doc;; ++i) {
                    if (*i == '\n') {
                        if (nl) {
                            out << '\n';
                            printIndent(out, indent);
                        }
                        out.write(j, i - j);
                        j = i + 1;
                        nl = true;
                    } else if (*i == 0) {
                        if (nl) {
                            out << '\n';
                            printIndent(out, indent);
                        }
                        out.write(j, i - j);
                        break;
                    }
                }
            }
        };
        template<typename char_t, typename traits_t, typename allocator_t>
        struct printDoc2_t<std::basic_string<char_t, traits_t, allocator_t>> {
            static void print(std::ostream & out, const std::basic_string<char_t, traits_t, allocator_t> & doc, int indent) {
                printDoc2_t<const char_t *>::print(out, doc.c_str(), indent);
            }
        };

        template<typename doc_t>
        static void printDoc0(const Documentation_t<doc_t> & doc, std::ostream & out, int indent) { printDoc2_t<std::decay<doc_t>::type>::print(out, doc.m_doc, indent); }
        template<>
        static void printDoc0<void>(const Documentation_t<void> &, std::ostream &, int) {}

        template<typename doc_t>
        struct Documentable_t {
            Documentation_t<doc_t> m_doc;
            constexpr Documentable_t() {}
            constexpr Documentable_t(Documentation_t<doc_t> doc) : m_doc(std::move(doc)) {}
            void printDoc(std::ostream & out, int indent) { out << m_doc.m_doc; }
            const Documentation_t<doc_t> & getDoc() const noexcept { return m_doc; }
        };
        template<>
        struct Documentable_t<void> {
            constexpr Documentable_t() {}
            constexpr Documentable_t(Documentation_t<void> doc) {}
            void printDoc(std::ostream &, int) {}
            static Documentation_t<void> getDoc() noexcept { return Documentation_t<void>{}; }
        };

        template<typename fn_t, typename sub_t>
        struct Callback_t {
            fn_t m_fn;
            sub_t m_sub;
            constexpr Callback_t(fn_t fn, sub_t sub) : m_fn(std::move(fn)), m_sub(std::move(sub)) {}

            ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                return m_sub.parse(r, i, argc, argv, m_fn);
            }

            void printUsage(std::ostream & out, int indent) const { m_sub.printUsage(out, indent); }
        };

        struct NullCallbackFn_t {
            void operator()() const noexcept {}
            template<typename x> void operator()(const x &) const noexcept {}
        };

        struct Const_t {
            const char * m_value;
            constexpr Const_t(const char * value) : m_value(value) {}

            template<typename fn_t = NullCallbackFn_t>
            ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t()) const {
                if (i >= argc) {
                    //r->errorMessage = "index out of bounds";
                    //r->usageMessage = "";
                    r->m_object = this;
                    r->m_printUsage = invokePrintUsage0<Const_t>;
                    return ret::mismatch;
                }
                if (std::strcmp(m_value, argv[i]) != 0) { return ret::mismatch; }
                cb();
                ++i;
                return ret::ok;
            }

            void printUsage(std::ostream & out, int) const { out << m_value; }

            template<typename fn_t>
            friend Callback_t<fn_t, Const_t> operator>>(Const_t c, fn_t fn) {
                return Callback_t<fn_t, Const_t>(std::move(fn), std::move(c));
            }

        };

        struct Var_t {
            const char * m_identifier;
            constexpr Var_t(const char * id) : m_identifier(id) {}

            template<typename fn_t = NullCallbackFn_t>
            ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                if (i >= argc) {
                    //r->errorMessage = "index out of bounds";
                    //r->usageMessage = "";
                    r->m_object = this;
                    r->m_printUsage = invokePrintUsage0<Var_t>;
                    return ret::mismatch;
                }
                cb(argv[i]);
                ++i;
                return ret::ok;
            }

            void printUsage(std::ostream & out, int) const { out << '<' << m_identifier << '>'; }

            template<typename fn_t>
            friend Callback_t<fn_t, Var_t> operator>>(Var_t v, fn_t fn) {
                return Callback_t<fn_t, Var_t>(std::move(fn), std::move(v));
            }

            template<typename scalar_t>
            friend typename std::enable_if<std::is_scalar<scalar_t>::value, Callback_t<lambdaScalarParser<scalar_t>, Var_t>>::type
                operator>>(Var_t v, scalar_t * target) {
                return Callback_t<lambdaScalarParser<scalar_t>, Var_t>(lambdaScalarParser<scalar_t>{target}, std::move(v));
            }

            friend Callback_t<lambdaStringParser<const char *>, Var_t> operator>>(Var_t v, const char ** target) {
                return Callback_t<lambdaStringParser<const char *>, Var_t>(lambdaStringParser<const char *>{target}, std::move(v));
            }

            template<typename char_t, typename traits_t, typename allocator_t>
            friend Callback_t<lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>, Var_t> operator>>(Var_t v, std::basic_string<char_t, traits_t, allocator_t> * target) {
                return Callback_t<lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>, Var_t>(lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>{target}, std::move(v));
            }

        };

        template<typename doc_t, bool printAsList, typename ... subs_t>
        struct Sequence_t : Documentable_t<doc_t> {
            std::tuple<subs_t...> m_subs;
            constexpr Sequence_t(Documentation_t<doc_t> doc, decltype(m_subs) subs) : Documentable_t<doc_t>(std::move(doc)), m_subs(std::move(subs)) {}

            template<typename fn_t = NullCallbackFn_t>
            ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                auto j = i;
                auto retVal = parse<0>(m_subs, r, j, argc, argv);
                if (retVal == ret::ok) {
                    i = j;
                    cb();
                }
                return retVal;
            }

            template<int I>
            static ret parse(const std::tuple<subs_t...> & subs, parseResult * r, int & i, int argc, const char * const * argv) {
                auto retVal = std::get<I>(subs).parse(r, i, argc, argv);
                if (retVal != ret::ok) { return I == 0 ? ret::mismatch : (/*strictMatch*/true ? ret::error : retVal); }
                return parse<I + 1>(subs, r, i, argc, argv);
            }

            template<>
            static ret parse<sizeof...(subs_t)>(const std::tuple<subs_t...> &, parseResult *, int &, int, const char * const *) { return ret::ok; }

            void printUsage(std::ostream & out, int indent) const { printUsage0<0>(m_subs, getDoc(), out, indent); }

            template<int I>
            static void printUsage0(const std::tuple<subs_t...> & subs, const Documentation_t<doc_t> & doc, std::ostream & out, int indent) {
                if (printAsList) {
                    out << '\n';
                    printIndent(out, indent);
                }
                std::get<I>(subs).printUsage(out, indent + (printAsList ? 1 : 0));
                if (!printAsList && (sizeof...(subs_t) - 1 != I)) { out << ' '; }
                printUsage0<I + 1>(subs, doc, out, indent);
            }

            template<>
            static void printUsage0<sizeof...(subs_t)>(const std::tuple<subs_t...> &, const Documentation_t<doc_t> & doc, std::ostream & out, int indent) {
                if (!std::is_same<void, doc_t>::value) { out << ' '; }
                printDoc0(doc, out, indent);
            }

            // attach lambda
            template<typename fn_t>
            friend Callback_t<fn_t, Sequence_t> operator>>(Sequence_t c, fn_t fn) {
                return Callback_t<fn_t, Sequence_t>(std::move(fn), std::move(c));
            }

            // set print as list
            template<typename x = typename std::enable_if<!printAsList, Sequence_t<doc_t, true, subs_t...>>::type>
            friend x operator&(Sequence_t c, UsageAsList_t) {
                return x(std::move(c.getDoc()), std::move(c.m_subs));
            }

            // set doc
            template<typename newDoc_t>
            friend Sequence_t<newDoc_t, printAsList, subs_t...> operator&(Sequence_t c, Documentation_t<newDoc_t> doc) {
                return Sequence_t<newDoc_t, printAsList, subs_t...>(std::move(doc), std::move(c.m_subs));
            }

        };

        template<typename doc_t, bool printAsList, typename ... subs_t>
        struct Optional_t : Documentable_t<doc_t> {
            std::tuple<subs_t...> m_subs;
            constexpr Optional_t(Documentation_t<doc_t> doc, decltype(m_subs) subs) : Documentable_t<doc_t>(std::move(doc)), m_subs(std::move(subs)) {}

            template<typename fn_t = NullCallbackFn_t>
            ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                auto j = i;
                auto retVal = parse<0>(m_subs, r, j, argc, argv);
                if (retVal == ret::ok) {
                    i = j;
                    cb();
                }
                return ret::ok;
            }

            template<int I>
            static ret parse(const std::tuple<subs_t...> & subs, parseResult * r, int & i, int argc, const char * const * argv) {
                auto retVal = std::get<I>(subs).parse(r, i, argc, argv);
                if (retVal != ret::ok) { return ret::mismatch; }
                return parse<I + 1>(subs, r, i, argc, argv);
            }

            template<>
            static ret parse<sizeof...(subs_t)>(const std::tuple<subs_t...> &, parseResult *, int &, int, const char * const *) { return ret::ok; }

            void printUsage(std::ostream & out, int indent) const {
                out << '[';
                Sequence_t<void, printAsList, subs_t...>::printUsage0<0>(m_subs, Documentation_t<void>{}, out, indent);
                out << ']';
                if (!std::is_same<void, doc_t>::value) {
                    out << ' ';
                    printDoc0(getDoc(), out, indent);
                }
            }

            template<typename fn_t>
            friend Callback_t<fn_t, Optional_t> operator>>(Optional_t c, fn_t fn) {
                return Callback_t<fn_t, Optional_t>(std::move(fn), std::move(c));
            }

            // set print as list
            template<typename x = typename std::enable_if<!printAsList, Optional_t<doc_t, true, subs_t...>>::type>
            friend x operator&(Optional_t c, UsageAsList_t) {
                return x(std::move(c.getDoc()), std::move(c.m_subs));
            }

            // set doc
            template<typename newDoc_t, typename x = Optional_t<newDoc_t, printAsList, subs_t...>>
            friend x operator&(Optional_t c, Documentation_t<newDoc_t> doc) {
                return x(std::move(doc), std::move(c.m_subs));
            }

        };

        template<typename sub_t>
        struct Loop_t {
            sub_t m_sub;
            constexpr Loop_t(sub_t sub) : m_sub(std::move(sub)) {}

            template<typename fn_t = NullCallbackFn_t>
            ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                while (true) {
                    auto j = i;
                    auto retVal = m_sub.parse(r, j, argc, argv);
                    if (retVal != ret::ok || i == j) { return ret::ok; }
                    i = j;
                    cb();
                }
            }

            void printUsage(std::ostream & out, int indent) const { m_sub.printUsage(out, indent); }

            template<typename fn_t>
            friend Callback_t<fn_t, Loop_t> operator>>(Loop_t c, fn_t fn) {
                return Callback_t<fn_t, Loop_t>(std::move(fn), std::move(c));
            }

        };

        template<typename ... subs_t>
        struct Branch_t {
            std::tuple<subs_t...> m_subs;
            constexpr Branch_t(subs_t ... subs) : m_subs(std::forward<subs_t>(subs)...) {}

            template<typename fn_t = NullCallbackFn_t>
            ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                auto j = i;
                auto retVal = parse<0>(*this, r, j, argc, argv);
                if (retVal == ret::ok) {
                    i = j;
                    cb();
                }
                return retVal;
            }

            template<int I>
            static ret parse(const Branch_t & b, parseResult * r, int & i, int argc, const char * const * argv) {
                auto retVal = std::get<I>(b.m_subs).parse(r, i, argc, argv);
                if (retVal == ret::mismatch) { return parse<I + 1>(b, r, i, argc, argv); }
                return retVal;
            }

            template<>
            static ret parse<sizeof...(subs_t)>(const Branch_t & b, parseResult * r, int &, int, const char * const *) {
                r->m_object = &b;
                r->m_printUsage = invokePrintUsage0<Branch_t>;
                return ret::error;
            }

            void printUsage(std::ostream & out, int indent) const {
                out << "{";
                Sequence_t<void, false, subs_t...>::printUsage0<0>(m_subs, Documentation_t<void>{}, out, indent);
                out << '}';
            }

            template<typename fn_t>
            friend Callback_t<fn_t, Branch_t> operator>>(Branch_t c, fn_t fn) {
                return Callback_t<fn_t, Branch_t>(std::move(fn), std::move(c));
            }

        };

    }

}
