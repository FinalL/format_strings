#include <iostream>
#include <type_traits>
#include <array>
#include <utility>

namespace format_strings
{
template <class...>
struct Container {};

template <class, class, class, class>
struct Subparser;

template <class, class...>
struct StringContainer;
/******************************************************/
template <char... chars>
struct String
{};

template <>
struct String<>
{
    static constexpr std::size_t length = 0u;   
};


template <char c0, char... chars>
struct String<c0, chars...>
{
    using type = String<c0,chars...>;
    static constexpr char first = c0;
    static constexpr std::size_t length = 1 + sizeof...(chars);
    static constexpr std::array<char,length> value = {c0, chars...}; // returns the array without null terminator
    static constexpr char string[] = {c0,chars...,'\0'}; // returns the array with null terminator
    constexpr const char* c_str() const
    {
        return type::string;
    }
    constexpr operator const char* () const
    {
        return type::string;
    }
    template <class... Args>
    constexpr auto format(Args... args) const; // for reusable String use as format string
};

namespace detail 
{
template <class, class...>
struct StringContainer
{};

template <template <char...> class Accumulator, char... chars>
struct StringContainer<Accumulator<chars...>>
{
    static constexpr std::size_t length = Accumulator<chars...>::length;
    static constexpr std::array<char, length> merge = {chars...};
    using merged_string = String<chars...>;
};

template <template <char...> class Accumulator, 
          template <char...> class String0, 
          class... OtherStrings,
          char... accum_chars, char f, char... chars>
struct StringContainer<Accumulator<accum_chars...>,String0<f,chars...>,OtherStrings...> :
            StringContainer<Accumulator<accum_chars...,f>,String0<chars...>, OtherStrings...>
{};

template <template <char...> class Accumulator,
          template <char...> class String0,
          class... OtherStrings,
          char... accum_chars>
struct StringContainer<Accumulator<accum_chars...>,String0<>,OtherStrings...> :
            StringContainer<Accumulator<accum_chars...>, OtherStrings...>
{};
/*****************Subparser*************************************/
template <class,class,class,class>
struct Subparser
{};

template <template <class... strings> class Container, 
          template <char...> class LastString,
          template <char...> class EmptyString, // empty string of remaining chars of the original formatting string
          template <class...> class SubstContainer, // container holding Strings to replace chars
          class... strings, char... remaining_chars, class... remaining_substrings>
struct Subparser<Container<strings...>, LastString<remaining_chars...>, EmptyString<>, SubstContainer<remaining_substrings...>>
{
    static constexpr std::size_t length = Container<strings...>::length + LastString<remaining_chars...>::length;
    static constexpr auto array = Container<strings...,String<remaining_chars...,'\0'>>::merge; // std::array<char,length>
    static constexpr auto string = typename Container<strings...,String<remaining_chars...>>::merged_string{};
};

template <template <class...> class Container, 
          template <char...> class CurrentString,
          template <char...> class UnprocessedString,
          template <class...> class SubstContainer,
          class... strings, char... current_chars, char first_char, char... remainder, class... remaining_substrings>
struct Subparser<Container<strings...>, CurrentString<current_chars...>, UnprocessedString<first_char, remainder...>, SubstContainer<remaining_substrings...>> :
            Subparser<Container<strings...>, CurrentString<current_chars...,first_char>, UnprocessedString<remainder...>, SubstContainer<remaining_substrings...>>
{};

template <template <class...> class Container,
          template <char...> class CurrentString,
          template <char...> class UnprocessedString,
          template <class...> class SubstContainer,
          class... strings, char... current_chars, char... remainder, class first_subststring, class... remaining_strings>
struct Subparser<Container<strings...>, CurrentString<current_chars...>, UnprocessedString<'{', '}', remainder...>, SubstContainer<first_subststring, remaining_strings...>> :
            Subparser<Container<strings...,CurrentString<current_chars...>,first_subststring>, String<>, UnprocessedString<remainder...>, SubstContainer<remaining_strings...>>
{};

/*****************Parser*************************************/
template <char... chars>
struct Parser
{
    using type = Parser<chars...>;
    static constexpr char input[] = {chars...};
    template <class... Args>
    constexpr auto operator()(Args... args)
    {
        return Subparser<
                    StringContainer<String<>>, // substrings holder
                    String<>,                  // temporary string used as buffer
                    String<chars...>,          // input chars to be parsed
                    Container<Args...>         // substitution string holder, Container<String<char...>,etc.>
                >::string;
    }
};
} // namespace detail

// String<char...> format for reuse
template <char c0, char... chars>
template <class... Args>
constexpr auto String<c0,chars...>::format(Args... args) const
{
    return detail::Parser<c0,chars...>()(args...);
}

namespace literals
{
    template <char... chars>
    constexpr auto operator"" _s()
    {
        return String<chars...>();
    }

    template <typename Char, Char... chars>
    constexpr auto operator"" _s()
    {
        return String<chars...>();
    }

    template <class Char, Char... chars>
    constexpr auto operator"" _format()
    {
        static_assert(std::is_same_v<Char, char>,"Only char supported");
        return detail::Parser<chars...>();
    }
} // namespace literals
} // namespace format_strings

using namespace format_strings::literals;

constexpr auto frequent_string = "Prefix string."_s; // string used frequently that can be inserted into other strings at compile time
constexpr auto formatted = "{} This is a {} formatting text, with some number at the end {}."_format(frequent_string, "freaking cool"_s, 42_s);
constexpr const char* m = formatted;
constexpr auto fmt_str = ">{}< format string used multiple times {}"_s;
constexpr auto another = fmt_str.format(frequent_string, 42_s);
constexpr auto another1 = fmt_str.format("one-time-use string"_s);
constexpr auto test_string = "Hi this is a String<char...> but it can be converted too"_s;
constexpr char literal[] = "Basic string literal that is used everywhere.";
// just tests
// constexpr auto ac = String<'H','i'>::value;
// using testexp = detail::StringContainer<String<'H','i'>,String<',',' ','m','y'>,String<' '>,String<'g','u','y','s'>>;
int main(int, char**)
{
    std::cout << formatted << std::endl;
    std::cout << test_string << std::endl;
    std::cout << fmt_str << std::endl;
    std::cout << another << std::endl << another1 << std::endl;
    std::cout << literal << std::endl;
    return 0;
}