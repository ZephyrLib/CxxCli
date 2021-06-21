
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

        template<typename, typename T>
        struct is_callable {
            static_assert(std::integral_constant<T, false>::value, "Second template parameter needs to be of function type.");
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
            constexpr bool operator()() const noexcept { return true; }
            template<typename x> constexpr bool operator()(const x &) const noexcept { return true; }
        };

        template<typename fn_t, bool hasBoolRet, typename ... args_t>
        struct callbackInvoker_t;
        template<typename fn_t, typename ... args_t>
        struct callbackInvoker_t<fn_t, true, args_t...> { static constexpr bool invoke(const fn_t & fn, const args_t & ... args) { return fn(args...); } };
        template<typename fn_t, typename ... args_t>
        struct callbackInvoker_t<fn_t, false, args_t...> { static constexpr bool invoke(const fn_t & fn, const args_t & ... args) { fn(args...); return true; } };

        template<typename extraCb_t, typename fn_t, bool hasBoolRet, typename ... args_t>
        struct Callaback0_t : extraCb_t {
            static constexpr bool validCallback = true;
            fn_t m_fn;
            constexpr Callaback0_t(extraCb_t efn, fn_t fn) : extraCb_t(std::move(efn)), m_fn(std::move(fn)) {}
            constexpr bool callback(const args_t & ... args) const noexcept(noexcept(std::declval<const fn_t &>()(args...))) {
                return extraCb_t::callback(args...) && callbackInvoker_t<fn_t, hasBoolRet, args_t...>::invoke(m_fn, args...);
            }
        };

        template<typename fn_t, bool hasCorrectArgs, typename ... args_t>
        struct getCallaback1_t {
            template<typename extraCb_t>
            using b = Callaback0_t<extraCb_t, fn_t, is_callable_v<fn_t, bool(args_t...)>, args_t...>;
        };
        template<typename fn_t, typename ... args_t>
        struct getCallaback1_t<fn_t, false, args_t...> {
            template<typename>
            struct b {
                static constexpr bool validCallback = false;
            };
        };

        template<typename fn_t>
        struct getCallaback0_t {
            template<typename ... args_t>
            using a = getCallaback1_t<fn_t, is_callable_v<fn_t, decltype(nullptr)(args_t...)>, args_t...>;
        };
        template<>
        struct getCallaback0_t<void> {
            template<typename ... args_t>
            struct a {
                template<typename>
                struct b {
                    constexpr static bool callback(const args_t & ...) noexcept { return true; }
                };
            };
        };

        template<typename extraCb_t, typename fn_t, typename ... args_t>
        struct getCallaback_t {
            using t = typename getCallaback0_t<fn_t>::template a<args_t...>::template b<extraCb_t>;
        };

        template<typename extraCb_t, typename fn_t, typename ... args_t>
        using callback_t = typename getCallaback_t<extraCb_t, fn_t, args_t...>::t;

    }

    namespace details {

        namespace const_ {

            using defaultCb_t = callback_t<void, void>;

            template<typename value_t, typename cb_t>
            struct Const_t : cb_t {
                static constexpr bool usageAsList = false;
                static constexpr bool createsScope = false;
                static constexpr int containedObjectCount = 0;

                value_t m_value;

                constexpr Const_t(value_t value, cb_t fn) : cb_t(std::move(fn)), m_value(std::move(value)) {}

                ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                    if (i >= argc) {
                        //r->errorMessage = "index out of bounds";
                        //r->usageMessage = "";
                        r->m_object = this;
                        r->m_printUsage = invokePrintUsage0<Const_t>;
                        return ret::mismatch;
                    }
                    if (std::strcmp(m_value, argv[i]) != 0) { return ret::mismatch; }
                    if (!cb_t::callback()) { return ret::mismatch; }
                    ++i;
                    return ret::ok;
                }

                void printUsage(std::ostream & out, int) const { out << m_value; }

                template<typename fn_t, typename ncb_t = callback_t<cb_t, fn_t>, typename r = typename std::enable_if<ncb_t::validCallback, Const_t<value_t, ncb_t>>::type>
                constexpr friend auto operator>>(Const_t c, fn_t fn) -> r {
                    return r(std::move(c.m_value), ncb_t(std::move(static_cast<cb_t &>(c)), std::move(fn)));
                }

            };

        }

        namespace var {

            template<bool isLegal, typename x>
            struct identifierContainer_t {
                void printUsage(std::ostream & out, int) const { out << "<>"; }
            };
            template<typename x>
            struct identifierContainer_t<true, x> {
                x m_identifier;
                constexpr identifierContainer_t(x data) : m_identifier(std::move(data)) {}
                void printUsage(std::ostream & out, int) const { out << '<' << m_identifier << '>'; }
            };

            template<typename id_t>
            using identifierContainer = identifierContainer_t<!std::is_empty<id_t>::value && !std::is_same<id_t, void>::value, id_t>;

            using defaultCb_t = callback_t<void, void, const char *>;

            template<typename id_t, typename cb_t>
            struct Var_t : identifierContainer<id_t>, cb_t {
                static constexpr bool usageAsList = false;
                static constexpr bool createsScope = false;
                static constexpr int containedObjectCount = 0;

                constexpr Var_t(identifierContainer<id_t> idc, cb_t fn) : identifierContainer<id_t>(std::move(idc)), cb_t(std::move(fn)) {}

                ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                    if (i >= argc) {
                        //r->errorMessage = "index out of bounds";
                        //r->usageMessage = "";
                        r->m_object = this;
                        r->m_printUsage = invokePrintUsage0<Var_t>;
                        return ret::mismatch;
                    }
                    if (!cb_t::callback(argv[i])) { return ret::mismatch; }
                    ++i;
                    return ret::ok;
                }

                template<typename fn_t, typename ncb_t = callback_t<cb_t, fn_t, const char *>, typename r = typename std::enable_if<ncb_t::validCallback, Var_t<id_t, ncb_t>>::type>
                constexpr friend auto operator>>(Var_t v, fn_t fn) -> r {
                    return r(std::move(static_cast<identifierContainer<id_t> &>(v)), ncb_t(std::move(static_cast<cb_t &>(v)), std::move(fn)));
                }

                template<typename scalar_t, typename ncb_t = callback_t<cb_t, lambdaScalarParser<scalar_t>, const char *>, typename r = typename std::enable_if<std::is_scalar<scalar_t>::value, Var_t<id_t, ncb_t>>::type>
                constexpr friend auto operator>>(Var_t v, scalar_t * target) -> r {
                    return r(std::move(static_cast<identifierContainer<id_t> &>(v)), ncb_t(std::move(static_cast<cb_t &>(v)), lambdaScalarParser<scalar_t>{target}));
                }

                constexpr friend auto operator>>(Var_t v, const char ** target) -> Var_t<id_t, callback_t<cb_t, lambdaStringParser<const char *>, const char *>> {
                    using ncb_t = callback_t<cb_t, lambdaStringParser<const char *>, const char *>;
                    using r = Var_t<id_t, ncb_t>;
                    return r(std::move(static_cast<identifierContainer<id_t> &>(v)), ncb_t(std::move(static_cast<cb_t &>(v)), lambdaStringParser<const char *>{target}));
                }

                template<typename char_t, typename traits_t, typename allocator_t>
                constexpr friend auto operator>>(Var_t v, std::basic_string<char_t, traits_t, allocator_t> * target) -> Var_t<id_t, callback_t<cb_t, lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>, const char *>> {
                    using sp = lambdaStringParser<std::basic_string<char_t, traits_t, allocator_t>>;
                    using ncb_t = callback_t<cb_t, sp, const char *>;
                    using r = Var_t<id_t, ncb_t>;
                    return r(std::move(static_cast<identifierContainer<id_t> &>(v)), ncb_t(std::move(static_cast<cb_t &>(v)), sp{target}));
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

            using defaultCb_t = callback_t<void, void>;

            template<typename doc_t, typename cb_t, bool usageAsList0, typename ... subs_t>
            struct Sequence_t : Documentable_t<doc_t>, cb_t {
                static constexpr bool usageAsList = usageAsList0 || or_parameters(subs_t::usageAsList...);
                static constexpr bool createsScope = false;
                static constexpr int containedObjectCount = sizeof...(subs_t);

                std::tuple<subs_t...> m_subs;

                constexpr Sequence_t(Documentation_t<doc_t> doc, cb_t cb, decltype(m_subs) subs) : Documentable_t<doc_t>(std::move(doc)), cb_t(std::move(cb)), m_subs(std::move(subs)) {}

                ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), subs_t...>::parse(m_subs, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        if (cb_t::callback()) {
                            i = j;
                        }
                    }
                    return retVal;
                }

                void printUsage(std::ostream & out, int indent) const { printUsage_t<0, sizeof...(subs_t), usageAsList, doc_t, subs_t...>::print(m_subs, Documentable_t<doc_t>::getDoc(), out, indent); }

                // attach lambda
                template<typename fn_t, typename ncb_t = callback_t<cb_t, fn_t>, typename r = typename std::enable_if<ncb_t::validCallback, Sequence_t<doc_t, ncb_t, usageAsList, subs_t...>>::type>
                constexpr friend auto operator>>(Sequence_t s, fn_t fn) -> r {
                    return r(std::move(s.getDoc()), ncb_t(std::move(static_cast<cb_t &>(s)), std::move(fn)), std::move(s.m_subs));
                }

                // set print as list
                constexpr friend auto operator&(Sequence_t s, UsageAsList_t) -> Sequence_t<doc_t, cb_t, true, subs_t...> {
                    return Sequence_t<doc_t, cb_t, true, subs_t...>(std::move(s.getDoc()), std::move(static_cast<cb_t &>(s)), std::move(s.m_subs));
                }

                // set doc
                template<typename newDoc_t>
                constexpr friend auto operator&(Sequence_t s, Documentation_t<newDoc_t> doc) -> Sequence_t<newDoc_t, cb_t, usageAsList, subs_t...> {
                    return Sequence_t<newDoc_t, cb_t, usageAsList, subs_t...>(std::move(doc), std::move(static_cast<cb_t &>(s)), std::move(s.m_subs));
                }

            };

        }

        namespace optional {

            using defaultCb_t = callback_t<void, void>;

            template<typename sub_t, typename cb_t>
            struct Optional_t : cb_t {
                static constexpr bool usageAsList = sub_t::usageAsList;
                static constexpr bool createsScope = true;
                static constexpr int containedObjectCount = 1;

                sub_t m_sub;

                constexpr Optional_t(sub_t sub, cb_t cb) : cb_t(std::move(cb)), m_sub(std::move(sub)) {}

                ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                    auto j = i;
                    auto retVal = m_sub.parse(r, j, argc, argv);
                    if (retVal == ret::ok) {
                        if (cb_t::callback()) {
                            i = j;
                        }
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

                template<typename fn_t, typename ncb_t = callback_t<cb_t, fn_t>, typename r = typename std::enable_if<ncb_t::validCallback, Optional_t<sub_t, ncb_t>>::type>
                constexpr friend auto operator>>(Optional_t o, fn_t fn) -> r {
                    return r(std::move(o.m_sub), ncb_t(std::move(static_cast<cb_t &>(o)), std::move(fn)));
                }

            };

        }

        namespace loop {

            using defaultCb_t = callback_t<void, void>;

            template<typename sub_t, typename cb_t>
            struct Loop_t : cb_t {
                static constexpr bool usageAsList = sub_t::usageAsList;
                static constexpr bool createsScope = true;
                static constexpr int containedObjectCount = 1;

                sub_t m_sub;

                constexpr Loop_t(sub_t sub, cb_t cb) : cb_t(std::move(cb)), m_sub(std::move(sub)) {}

                template<typename fn_t = NullCallbackFn_t>
                ret parse(parseResult * r, int & i, int argc, const char * const * argv, const fn_t & cb = NullCallbackFn_t{}) const {
                    while (true) {
                        auto j = i;
                        auto retVal = m_sub.parse(r, j, argc, argv);
                        if (retVal != ret::ok || i == j) { return ret::ok; }
                        if (!cb_t::callback()) { return ret::ok; }
                        i = j;
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

                template<typename fn_t, typename ncb_t = callback_t<cb_t, fn_t>, typename r = typename std::enable_if<ncb_t::validCallback, Loop_t<sub_t, ncb_t>>::type>
                constexpr friend auto operator>>(Loop_t l, fn_t fn) -> r {
                    return r(std::move(l.m_sub), ncb_t(std::move(static_cast<cb_t &>(l)), std::move(fn)));
                }

            };

        }

        namespace branch {

            template<int I, int L, typename branch_t>
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

            using defaultCb_t = callback_t<void, void>;

            template<typename cb_t, typename ... subs_t>
            struct Branch_t : cb_t {
                static constexpr bool usageAsList = or_parameters(subs_t::usageAsList...);
                static constexpr bool createsScope = true;
                static constexpr int containedObjectCount = sizeof...(subs_t);

                std::tuple<subs_t...> m_subs;

                constexpr Branch_t(cb_t cb, decltype(m_subs) subs) : cb_t(std::move(cb)), m_subs(std::move(subs)) {}

                ret parse(parseResult * r, int & i, int argc, const char * const * argv) const {
                    auto j = i;
                    auto retVal = parse_t<0, sizeof...(subs_t), Branch_t>::parse(*this, r, j, argc, argv);
                    if (retVal == ret::ok) {
                        if (cb_t::callback()) {
                            i = j;
                        }
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

                template<typename fn_t, typename ncb_t = callback_t<cb_t, fn_t>, typename r = typename std::enable_if<ncb_t::validCallback, Branch_t<ncb_t, subs_t...>>::type>
                constexpr friend auto operator>>(Branch_t b, fn_t fn) -> r {
                    return r(ncb_t(std::move(static_cast<cb_t &>(b)), std::move(fn)), std::move(b.m_subs));
                }

            };

            template<int I, int L, typename branch_t>
            struct parse_t {
                static ret parse(const branch_t & b, parseResult * r, int & i, int argc, const char * const * argv) {
                    auto retVal = std::get<I>(b.m_subs).parse(r, i, argc, argv);
                    if (retVal == ret::mismatch) { return parse_t<I + 1, L, branch_t>::parse(b, r, i, argc, argv); }
                    return retVal;
                }
            };
            template<int L, typename branch_t>
            struct parse_t<L, L, typename branch_t> {
                static ret parse(const branch_t & b, parseResult * r, int &, int, const char * const *) {
                    r->m_object = &b;
                    r->m_printUsage = invokePrintUsage0<branch_t>;
                    return ret::error;
                }
            };

        }

    }

}
