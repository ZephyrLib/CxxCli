
#ifndef HEADER_CXXCLI
#error "illegal include"
#endif

#include <tuple>
#include <cstdlib>
#include <utility>
#include <cstring>
#include <ostream>
#include <type_traits>

namespace CxxCli {

    namespace details {

        template<typename char_t, char_t... values>
        struct static_string {
            static const char_t * chars() noexcept {
                static const char_t value[sizeof...(values) + 1] = { values..., 0 };
                return value;
            }
        };

        template<typename char_t, char_t value, unsigned count, unsigned... indices>
        struct generate_param_pack_t {
            using result = typename generate_param_pack_t<char_t, value, count - 1, value, indices...>::result;
        };
        template<typename char_t, char_t value, unsigned... indices>
        struct generate_param_pack_t<char_t, value, 0, indices...> {
            using result = static_string<char_t, indices...>;
        };

        template<typename char_t>
        using indent_t = typename generate_param_pack_t<char_t, (CXXCLI_INDENT_CHAR_VALUE), (CXXCLI_INDENT_LENGTH)>::result;
        using indent_char_t = indent_t<char>;

        template<typename x>
        static constexpr bool or_parameters(x a) { return a; }
        template<typename x, typename... args_t>
        static constexpr bool or_parameters(x a, args_t... args) { return a || or_parameters(args...); }

    }

    namespace details {

        enum struct ret {
            // its ok
            ok = 1,

            // not ok but recoverable
            mismatch,

            // no
            error
        };

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

        static void printIndent(std::ostream & out, int indent) {
            for (int i = 0; i < indent; ++i) { out << indent_char_t::chars(); }
        }

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
                        out << "# ";
                        out.write(j, i - j);
                        j = i + 1;
                        nl = true;
                        continue;
                    }
                    if (*i == 0) {
                        if (nl) {
                            out << '\n';
                            printIndent(out, indent);
                        }
                        out << "# ";
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
        static inline void printDoc0(const Documentation_t<doc_t> & doc, std::ostream & out, int indent) { printDoc2_t<typename std::decay<doc_t>::type>::print(out, doc.m_doc, indent); }
        template<>
        inline void printDoc0<void>(const Documentation_t<void> &, std::ostream &, int) {}

        template<typename doc_t>
        struct Documentable_t {
            Documentation_t<doc_t> m_doc;
            constexpr Documentable_t(Documentation_t<doc_t> doc) : m_doc(std::move(doc)) {}
            void printDoc(std::ostream & out, int indent) { out << m_doc.m_doc; }
            const constexpr Documentation_t<doc_t> & getDoc() const noexcept { return m_doc; }
        };
        template<>
        struct Documentable_t<void> {
            constexpr Documentable_t(Documentation_t<void>) {}
            void printDoc(std::ostream &, int) {}
            static constexpr Documentation_t<void> getDoc() noexcept { return Documentation_t<void>{}; }
        };

        template<typename fn_t, typename sub_t>
        struct Callback_t {
            static constexpr bool usageAsList = sub_t::usageAsList;
            static constexpr bool createsScope = sub_t::createsScope;
            static constexpr int containedObjectCount = sub_t::containedObjectCount;

            fn_t m_fn;
            sub_t m_sub;

            constexpr Callback_t(fn_t fn, sub_t sub) : m_fn(std::move(fn)), m_sub(std::move(sub)) {}

            ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                return m_sub.parse(r, i, argc, argv, m_fn);
            }

            void printUsage(std::ostream & out, int indent) const { m_sub.printUsage(out, indent); }

            template<typename x, typename r = decltype(std::declval<sub_t>() & std::declval<x>())>
            friend constexpr auto operator&(Callback_t c, x o) -> Callback_t<fn_t, r> {
                return Callback_t<fn_t, r>(std::move(c.m_fn), (std::move(c.m_sub) & std::move(o)));
            }

        };

        struct NullCallbackFn_t {
            constexpr void operator()() const noexcept {}
            template<typename x> constexpr void operator()(const x &) const noexcept {}
        };

    }

    namespace details {

        namespace const_ {

            struct Const_t {
                static constexpr bool usageAsList = false;
                static constexpr bool createsScope = false;
                static constexpr int containedObjectCount = 0;

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
                constexpr friend Callback_t<fn_t, Const_t> operator>>(Const_t c, fn_t fn) {
                    return Callback_t<fn_t, Const_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace var {

            template<bool isLegal, typename x>
            struct dataContainer_t {
                void printUsage(std::ostream & out, int) const { out << "<>"; }
            };

            template<typename x>
            struct dataContainer_t<true, x> {
                x m_data;
                constexpr dataContainer_t(x data) : m_data(std::move(data)) {}
                void printUsage(std::ostream & out, int) const { out << '<' << m_data << '>'; }
            };

            template<typename id_t>
            using dataContainer = dataContainer_t<!std::is_empty<id_t>::value && !std::is_same<id_t, void>::value, id_t>;

            template<typename id_t>
            struct Var_t : dataContainer<id_t> {
                static constexpr bool usageAsList = false;
                static constexpr bool createsScope = false;
                static constexpr int containedObjectCount = 0;

                template<typename x = id_t, typename = typename std::enable_if<!std::is_same<x, void>::value>::type>
                constexpr Var_t(x id) : dataContainer<id_t>(std::move(id)) {}

                template<typename = typename std::enable_if<std::is_same<id_t, void>::value>::type>
                constexpr Var_t() {}

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

                template<typename fn_t>
                constexpr friend Callback_t<fn_t, Var_t> operator>>(Var_t v, fn_t fn) {
                    return Callback_t<fn_t, Var_t>(std::move(fn), std::move(v));
                }

                template<typename scalar_t>
                constexpr friend typename std::enable_if<std::is_scalar<scalar_t>::value, Callback_t<lambdaScalarParser<scalar_t>, Var_t>>::type
                    operator>>(Var_t v, scalar_t * target) {
                    return Callback_t<lambdaScalarParser<scalar_t>, Var_t>(lambdaScalarParser<scalar_t>{target}, std::move(v));
                }

                constexpr friend Callback_t<lambdaStringParser<const char *>, Var_t> operator>>(Var_t v, const char ** target) {
                    return Callback_t<lambdaStringParser<const char *>, Var_t>(lambdaStringParser<const char *>{target}, std::move(v));
                }

                template<typename char_t, typename traits_t, typename allocator_t>
                constexpr friend Callback_t<lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>, Var_t> operator>>(Var_t v, std::basic_string<char_t, traits_t, allocator_t> * target) {
                    return Callback_t<lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>, Var_t>(lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>{target}, std::move(v));
                }

            };

        }

        namespace sequence {

            template<int I, int L, typename ... subs_t>
            struct parse_t {
                static ret parse(const std::tuple<subs_t...> & subs, parseResult * r, int & i, int argc, const char * const * argv) {
                    auto retVal = std::get<I>(subs).parse(r, i, argc, argv);
                    if (retVal != ret::ok) { return I == 0 ? ret::mismatch : (/*strictMatch*/true ? ret::error : retVal); }
                    return parse_t<I + 1, L, subs_t...>::parse(subs, r, i, argc, argv);
                }
            };
            template<int L, typename ... subs_t>
            struct parse_t<L, L, subs_t...> {
                static ret parse(const std::tuple<subs_t...> &, parseResult *, int &, int, const char * const *) { return ret::ok; }
            };

            template<int I, int L, bool usageAsList, typename doc_t, typename ... subs_t>
            struct printUsage_t {
                static void print(const std::tuple<subs_t...> & subs, const Documentation_t<doc_t> & doc, std::ostream & out, int indent) {
                    if (usageAsList) {
                        out << '\n';
                        printIndent(out, indent);
                    }
                    std::get<I>(subs).printUsage(out, indent);
                    if (sizeof...(subs_t) - 1 != I) {
                        if (!usageAsList) {
                            out << ' ';
                        }
                    }
                    printUsage_t<I + 1, L, usageAsList, doc_t, subs_t...>::print(subs, doc, out, indent);
                }
            };
            template<int L, bool usageAsList, typename doc_t, typename ... subs_t>
            struct printUsage_t<L, L, usageAsList, doc_t, subs_t...> {
                static void print(const std::tuple<subs_t...> &, const Documentation_t<doc_t> & doc, std::ostream & out, int indent) {
                    if (usageAsList) {
                        out << '\n';
                        printIndent(out, indent + 1);
                    } else {
                        out << ' ';
                    }
                    printDoc0(doc, out, indent + 1);
                }
            };
            template<int L, bool usageAsList, typename ... subs_t>
            struct printUsage_t<L, L, usageAsList, void, subs_t...> {
                static void print(const std::tuple<subs_t...> &, Documentation_t<void>, std::ostream &, int) {}
            };

            template<typename doc_t, bool usageAsList0, typename ... subs_t>
            struct Sequence_t : Documentable_t<doc_t> {
                static constexpr bool usageAsList = usageAsList0 || or_parameters(subs_t::usageAsList...);
                static constexpr bool createsScope = false;
                static constexpr int containedObjectCount = sizeof...(subs_t);

                std::tuple<subs_t...> m_subs;

                constexpr Sequence_t(Documentation_t<doc_t> doc, decltype(m_subs) subs) : Documentable_t<doc_t>(std::move(doc)), m_subs(std::move(subs)) {}

                template<typename fn_t = NullCallbackFn_t>
                ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), subs_t...>::parse(m_subs, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        i = j;
                        cb();
                    }
                    return retVal;
                }

                void printUsage(std::ostream & out, int indent) const { printUsage_t<0, sizeof...(subs_t), usageAsList, doc_t, subs_t...>::print(m_subs, Documentable_t<doc_t>::getDoc(), out, indent); }

                // attach lambda
                template<typename fn_t>
                constexpr friend Callback_t<fn_t, Sequence_t> operator>>(Sequence_t c, fn_t fn) {
                    return Callback_t<fn_t, Sequence_t>(std::move(fn), std::move(c));
                }

                // set print as list
                template<typename x = typename std::enable_if<true, Sequence_t<doc_t, true, subs_t...>>::type>
                constexpr friend x operator&(Sequence_t c, UsageAsList_t) {
                    return x(std::move(c.getDoc()), std::move(c.m_subs));
                }

                // set doc
                template<typename newDoc_t>
                constexpr friend Sequence_t<newDoc_t, usageAsList, subs_t...> operator&(Sequence_t c, Documentation_t<newDoc_t> doc) {
                    return Sequence_t<newDoc_t, usageAsList, subs_t...>(std::move(doc), std::move(c.m_subs));
                }

            };

        }

        namespace optional {

            template<typename sub_t>
            struct Optional_t {
                static constexpr bool usageAsList = sub_t::usageAsList;
                static constexpr bool createsScope = true;
                static constexpr int containedObjectCount = 1;

                sub_t m_sub;

                constexpr Optional_t(sub_t sub) : m_sub(std::move(sub)) {}

                template<typename fn_t = NullCallbackFn_t>
                ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                    auto j = i;
                    auto retVal = m_sub.parse(r, j, argc, argv);
                    if (retVal == ret::ok) {
                        i = j;
                        cb();
                    }
                    return ret::ok;
                }

                void printUsage(std::ostream & out, int indent) const {
                    out << '[';
                    if (usageAsList && sub_t::createsScope) {
                        out << '\n';
                        printIndent(out, indent + 1);
                    }
                    m_sub.printUsage(out, indent + (usageAsList ? 1 : 0));
                    if (usageAsList) {
                        out << '\n';
                        printIndent(out, indent);
                    }
                    out << ']';
                }

                template<typename fn_t>
                constexpr friend Callback_t<fn_t, Optional_t> operator>>(Optional_t c, fn_t fn) {
                    return Callback_t<fn_t, Optional_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace loop {

            template<typename sub_t>
            struct Loop_t {
                static constexpr bool usageAsList = sub_t::usageAsList;
                static constexpr bool createsScope = true;
                static constexpr int containedObjectCount = 1;

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

                void printUsage(std::ostream & out, int indent) const {
                    out << '[';
                    if (usageAsList && sub_t::createsScope) {
                        out << '\n';
                        printIndent(out, indent + 1);
                    }
                    m_sub.printUsage(out, usageAsList ? (indent + 1) : indent);
                    if (usageAsList) {
                        out << '\n';
                        printIndent(out, indent);
                    }
                    out << "]...";
                }

                template<typename fn_t>
                constexpr friend Callback_t<fn_t, Loop_t> operator>>(Loop_t c, fn_t fn) {
                    return Callback_t<fn_t, Loop_t>(std::move(fn), std::move(c));
                }

            };

        }

        namespace branch {

            template<int I, int L, typename ... subs_t>
            struct parse_t;

            template<int I, int L, bool usageAsList, typename doc_t, typename ... subs_t>
            struct printUsage_t {
                static void print(const std::tuple<subs_t...> & subs, const Documentation_t<doc_t> & doc, std::ostream & out, int indent) {
                    if (usageAsList && std::tuple_element<I, std::tuple<subs_t...>>::type::createsScope) {
                        out << '\n';
                        printIndent(out, indent + 1);
                    }
                    std::get<I>(subs).printUsage(out, indent);
                    if (sizeof...(subs_t) - 1 != I) {
                        if (usageAsList) {
                            out << "\n";
                            printIndent(out, indent);
                            out << "| ";
                        } else {
                            out << " | ";
                        }
                    }
                    printUsage_t<I + 1, L, usageAsList, doc_t, subs_t...>::print(subs, doc, out, indent);
                }
            };
            template<int L, bool usageAsList, typename doc_t, typename ... subs_t>
            struct printUsage_t<L, L, usageAsList, doc_t, subs_t...> {
                static void print(const std::tuple<subs_t...> &, const Documentation_t<doc_t> & doc, std::ostream & out, int indent) {
                    if (usageAsList) {
                        out << '\n';
                        printIndent(out, indent + 1);
                    } else {
                        out << ' ';
                    }
                    printDoc0(doc, out, indent + 1);
                }
            };
            template<int L, bool usageAsList, typename ... subs_t>
            struct printUsage_t<L, L, usageAsList, void, subs_t...> {
                static void print(const std::tuple<subs_t...> &, Documentation_t<void>, std::ostream &, int) {}
            };

            template<typename ... subs_t>
            struct Branch_t {
                static constexpr bool usageAsList = or_parameters(subs_t::usageAsList...);
                static constexpr bool createsScope = true;
                static constexpr int containedObjectCount = sizeof...(subs_t);

                std::tuple<subs_t...> m_subs;

                constexpr Branch_t(subs_t ... subs) : m_subs(std::forward<subs_t>(subs)...) {}

                template<typename fn_t = NullCallbackFn_t>
                ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), subs_t...>::parse(*this, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        i = j;
                        cb();
                    }
                    return retVal;
                }

                void printUsage(std::ostream & out, int indent) const {
                    if (usageAsList) {
                        out << '{';
                    } else {
                        out << "{ ";
                    }
                    printUsage_t<0, sizeof...(subs_t), usageAsList, void, subs_t...>::print(m_subs, Documentation_t<void>{}, out, indent + (usageAsList ? 1 : 0));
                    if (usageAsList) {
                        out << '\n';
                        printIndent(out, indent);
                        out << '}';
                    } else {
                        out << " }";
                    }
                }

                template<typename fn_t>
                constexpr friend Callback_t<fn_t, Branch_t> operator>>(Branch_t c, fn_t fn) {
                    return Callback_t<fn_t, Branch_t>(std::move(fn), std::move(c));
                }

            };

            template<int I, int L, typename ... subs_t>
            struct parse_t {
                static ret parse(const Branch_t<subs_t...> & b, parseResult * r, int & i, int argc, const char * const * argv) {
                    auto retVal = std::get<I>(b.m_subs).parse(r, i, argc, argv);
                    if (retVal == ret::mismatch) { return parse_t<I + 1, L, subs_t...>::parse(b, r, i, argc, argv); }
                    return retVal;
                }
            };
            template<int L, typename ... subs_t>
            struct parse_t<L, L, subs_t...> {
                static ret parse(const Branch_t<subs_t...> & b, parseResult * r, int &, int, const char * const *) {
                    r->m_object = &b;
                    r->m_printUsage = invokePrintUsage0<Branch_t<subs_t...>>;
                    return ret::error;
                }
            };

        }

    }

}
