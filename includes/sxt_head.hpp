#ifndef SXT_HEAD_HPP
#   define SXT_HEAD_HPP 1
#if (!(defined SXT_ISDIGIT)) || (!(defined SXT_ISALPHA)) || (!(defined SXT_ISSAPCE))

#if (defined SXT_ISDIGIT) || (defined SXT_ISALPHA) || (defined SXT_ISSAPCE)
    static_assert(0, "define SXT_ISDIGIT, SXT_ISALPHA and SXT_ISSAPCE if you defined one of them");
#endif // defined SXT_ISDIGIT || defined SXT_ISALPHA || defined SXT_ISSAPCE
#   include <locale> // for isalpha, isspace
#   define SXT_ISDIGIT(c__) (::std::isdigit(c__))
#   define SXT_ISALPHA(c__) (::std::isalpha(c__))
#   define SXT_ISSAPCE(c__) (::std::isspace(c__))
#endif // !defined SXT_ISDIGIT || !defined SXT_ISALPHA || !defined SXT_ISSAPCE
#if (!(defined SXR_DEFAULT_CHAR_TRAITS))
#   include <type_traits> // for char_traits
#   define SXR_DEFAULT_CHAR_TRAITS ::std::char_traits
#endif // !defined SXR_DEFAULT_CHAR_TRAITS

#if (!(defined SXT_ASSERT))
#   include <cassert>
#   define SXT_ASSERT(expr__) assert(expr__)
#endif // defined defined SXT_ASSERT

#if (!(defined SXR_MOVE))
#   include <utility> // for move
#   define SXR_MOVE ::std::move
#endif // !defined SXR_MOVE

#define SXT__NEXT_CHAR_WITHOUT_LINECHECK(it__, charcol__) ++(charcol__); ++(it__);// <- a bit slower. 
//do { ++(charcol__); ++(it__); } while(0) // but..
#define SXT__NEXT_CHAR_V(it__, itvalue__, charln__, charcol__) if (itvalue__ == '\n') { charcol__ = 0ULL; ++(charln__); } else { ++(charcol__); }  ++(it__);// <- a bit slower.
//do { if (char_traits_type::eq(itvalue__, '\n')) { charcol__ = 0ULL; ++(charln__); } else { ++(charcol__); }  ++(it__); } while(0) // but..

namespace sxt {
    enum token_type {
        STX_TOKEN_TYPE_WORD,            // [a-z]|[A-Z]|_
        STX_TOKEN_TYPE_FLOAT,           // -|[0-9]|.
        STX_TOKEN_TYPE_INTEGER,         // [0-9]
        STX_TOKEN_TYPE_STRING_LETTERAL, // " ... "
        STX_TOKEN_TYPE_PLUS,            // + 
        STX_TOKEN_TYPE_MINUS,           // -
        STX_TOKEN_TYPE_ASSIGN,          // =
        STX_TOKEN_TYPE_LPAREN,          // (
        STX_TOKEN_TYPE_RPAREN,          // )
        STX_TOKEN_TYPE_LCURLY,          // {
        STX_TOKEN_TYPE_RCURLY,          // }
        STX_TOKEN_TYPE_DOT,             // .
        STX_TOKEN_TYPE_COMMA,           // ,
        STX_TOKEN_TYPE_COLON,           // :
        STX_TOKEN_TYPE_SEMICOLON,       // ;
        STX_TOKEN_TYPE_QUOTE,           // '
        STX_TOKEN_TYPE_DOUBLE_QUOTE,    // "
        STX_TOKEN_TYPE_STAR,            // *
        STX_TOKEN_TYPE_TILDA,           // ~
        STX_TOKEN_TYPE_BACKCLASH,       // '\'
        STX_TOKEN_TYPE_MORE,            // >
        STX_TOKEN_TYPE_LESS,            // <
        STX_TOKEN_TYPE_EXCLAMATION,     // !
        STX_TOKEN_TYPE_QUESTION,        // ?
        STX_TOKEN_TYPE_AMPERSAND,       // &
        STX_TOKEN_TYPE_EOF,             // '\0'
        STX_TOKEN_TYPE_INVALID,         // 
        STX_TOKEN_TYPE_MAX_ENUM,        // 
    };

    [[nodiscard]] static bool this_type_is_valid(token_type type) {
        return type < STX_TOKEN_TYPE_EOF; 
    }

    template <class CharT_>
    struct token_symbols_trait {
        public:
        typedef CharT_ char_type;

        public:
        [[nodiscard]] static token_type type_from_char(char_type c) noexcept {
            switch (c) {
                case '+': return    STX_TOKEN_TYPE_PLUS;
                case '-': return    STX_TOKEN_TYPE_MINUS;
                case '=': return    STX_TOKEN_TYPE_ASSIGN;
                case '(': return    STX_TOKEN_TYPE_LPAREN;
                case ')': return    STX_TOKEN_TYPE_RPAREN;
                case '{': return    STX_TOKEN_TYPE_LCURLY;
                case '}': return    STX_TOKEN_TYPE_RCURLY;
                case '.': return    STX_TOKEN_TYPE_DOT;
                case ',': return    STX_TOKEN_TYPE_COMMA;
                case ':': return    STX_TOKEN_TYPE_COLON;
                case ';': return    STX_TOKEN_TYPE_SEMICOLON;
                case '\'':return    STX_TOKEN_TYPE_QUOTE;
                case '\"':return    STX_TOKEN_TYPE_DOUBLE_QUOTE;
                case '*': return    STX_TOKEN_TYPE_STAR;
                case '~': return    STX_TOKEN_TYPE_TILDA;
                case '\\':return    STX_TOKEN_TYPE_BACKCLASH;
                case '>': return    STX_TOKEN_TYPE_MORE;
                case '<': return    STX_TOKEN_TYPE_LESS;
                case '?': return    STX_TOKEN_TYPE_QUESTION;
                case '!': return    STX_TOKEN_TYPE_EXCLAMATION;
                case '&': return    STX_TOKEN_TYPE_AMPERSAND;
                default: {
                    if (SXT_ISALPHA(c) || c == '_'){
                        return STX_TOKEN_TYPE_WORD;
                    } else if (SXT_ISDIGIT(c)){
                        return STX_TOKEN_TYPE_INTEGER;
                    } else {
                        return STX_TOKEN_TYPE_INVALID;
                    }
                }
            }
            return STX_TOKEN_TYPE_INVALID;
        }

    };

    template <>
    struct token_symbols_trait<wchar_t> {
       public:
        typedef wchar_t char_type;

        public:
        [[nodiscard]] static token_type type_from_char(char_type c) noexcept {
            switch (c) {
                case L'+': return    STX_TOKEN_TYPE_PLUS;
                case L'-': return    STX_TOKEN_TYPE_MINUS;
                case L'=': return    STX_TOKEN_TYPE_ASSIGN;
                case L'(': return    STX_TOKEN_TYPE_LPAREN;
                case L')': return    STX_TOKEN_TYPE_RPAREN;
                case L'{': return    STX_TOKEN_TYPE_LCURLY;
                case L'}': return    STX_TOKEN_TYPE_RCURLY;
                case L'.': return    STX_TOKEN_TYPE_DOT;
                case L',': return    STX_TOKEN_TYPE_COMMA;
                case L':': return    STX_TOKEN_TYPE_COLON;
                case L';': return    STX_TOKEN_TYPE_SEMICOLON;
                case L'\'':return    STX_TOKEN_TYPE_QUOTE;
                case L'\"':return    STX_TOKEN_TYPE_DOUBLE_QUOTE;
                case L'*': return    STX_TOKEN_TYPE_STAR;
                case L'~': return    STX_TOKEN_TYPE_TILDA;
                case L'\\':return    STX_TOKEN_TYPE_BACKCLASH;
                case L'>': return    STX_TOKEN_TYPE_MORE;
                case L'<': return    STX_TOKEN_TYPE_LESS;
                case L'?': return    STX_TOKEN_TYPE_QUESTION;
                case L'!': return    STX_TOKEN_TYPE_EXCLAMATION;
                case L'&': return    STX_TOKEN_TYPE_AMPERSAND;
                default: {
                    if (SXT_ISALPHA(c) || c == L'_'){
                        return STX_TOKEN_TYPE_WORD;
                    } else if (SXT_ISDIGIT(c)){
                        return STX_TOKEN_TYPE_INTEGER;
                    } else {
                        return STX_TOKEN_TYPE_INVALID;
                    }
                }
            }
            return STX_TOKEN_TYPE_INVALID;
        }
    };

    enum ext_token_type_flag_bit {
        STX_EXT_TOKEN_TYPE_FLAG_BIT_NONE = (0),
        STX_EXT_TOKEN_TYPE_FLAG_BIT_STRING_LETTERAL = (1 << 0),
        STX_EXT_TOKEN_TYPE_FLAG_BIT_UNKNOWN_AS_WORDS = (1 << 1),
        STX_EXT_TOKEN_TYPE_FLAG_BIT_SIGNED_NUMBERS = (1 << 2),
        STX_EXT_TOKEN_TYPE_FLAG_BIT_MAX_ENUM,
    };
    typedef int ext_token_type_flag_bits;

    template<class StringT_>
    struct value_token {
        public:
        typedef StringT_& reference;
        typedef const StringT_& const_reference;
        typedef typename StringT_::const_iterator const_iterator;

        private:
        StringT_ value_;
        token_type type_;
        
        public:
        value_token() : value_(), type_(STX_TOKEN_TYPE_INVALID) {

        }
        value_token(token_type type__, const StringT_& str__) : value_(str__), type_(type__) {

        }
        value_token(token_type type__, StringT_&& str__) : value_(SXR_MOVE(str__)), type_(type__) {

        }

        public:
        [[nodiscard]] reference value() noexcept {
            return value_;
        }
        [[nodiscard]] const_reference value() const noexcept {
            return value_;
        }
        [[nodiscard]] token_type type() const noexcept {
            return type_;
        }
        [[nodiscard]] bool is_valid() const noexcept {
            return this_type_is_valid(type_);
        }
    };
    /**
     * @brief A tokenizer class that can be configured to split input strings into tokens based on customizable delimiters and traits.
     *
     * @tparam StringT_ type of string to use.
     * @tparam TokenSymbolsTraitsT_ the type of the token symbols traits. Defaults to SXR_DEFAULT_CHAR_TRAITS<typename StringT_::value_type>.
     */
    template<class StringT_, class TokenSymbolsTraitsT_ = token_symbols_trait<typename StringT_::value_type>>
    struct tokenizer {
        public:
        typedef typename StringT_::const_iterator const_iterator;
        typedef value_token<StringT_> value_token_type;
        typedef typename StringT_::value_type char_type;
        typedef TokenSymbolsTraitsT_ symbols_trait_type;

        private:
        const_iterator begin_;  /// The beginning iterator of the input range.
        const_iterator current_;/// The current iterator position in the input range.
        const_iterator end_;    /// The ending iterator of the input range.
        size_t line_{0ULL};     /// The current line number in the input range.
        size_t column_{0ULL};    /// The current column number in the input range.

        public:
        tokenizer() {

        }
        /**
         * @brief Constructor that initializes the tokenizer with a range of iterators.
         *
         * @param beginI The beginning iterator of the input range.
         * @param endI The ending iterator of the input range.
         */
        tokenizer(const_iterator beginI, const_iterator endI) : begin_(beginI), current_(beginI), end_(endI) {

        }

        public:
        /**
         * @brief Retrieves the next token from the input range.
         *
         * @param flags the flags for token type.
         * @return The next token of type value_token_type.
         */
        value_token_type next_new_token(ext_token_type_flag_bits flags) {
            const auto createNumberToken = [](const_iterator& currentr, size_t& columnr, const_iterator end__, const_iterator b) {
                token_type numberType = STX_TOKEN_TYPE_INTEGER;
                
                for (char_type currentValue = *currentr; currentr != end__;  currentValue = *currentr) {
                    token_type curType = symbols_trait_type::type_from_char(currentValue);
                    if ((numberType == STX_TOKEN_TYPE_INTEGER) && (curType == token_type::STX_TOKEN_TYPE_DOT)) {
                        SXT__NEXT_CHAR_WITHOUT_LINECHECK(currentr, columnr);
                        if ((currentr == end__) || (symbols_trait_type::type_from_char(*currentr) != token_type::STX_TOKEN_TYPE_INTEGER)) {
                            --currentr;
                            --columnr;
                            break;
                        }
                        numberType = STX_TOKEN_TYPE_FLOAT;
                    } else if (curType != token_type::STX_TOKEN_TYPE_INTEGER) {
                        break;
                    }
                    SXT__NEXT_CHAR_WITHOUT_LINECHECK(currentr, columnr);
                }
                
                return value_token_type(numberType, StringT_(b, currentr));
            };
            const auto isWordSymbol = [](char_type c) {
                return symbols_trait_type::type_from_char(c) == token_type::STX_TOKEN_TYPE_WORD; // compiler have to optimize this on litteraly O1, so...
            };

            while (current_ != end_) {
                auto currentValue = *current_;
                if (SXT_ISSAPCE(currentValue)) {
                    SXT__NEXT_CHAR_V(current_, currentValue, line_, column_);
                    continue;
                }
                const token_type currentTokenType = symbols_trait_type::type_from_char(currentValue);
                
                if (currentTokenType == STX_TOKEN_TYPE_WORD) {
                    const const_iterator wordStart = current_;
                    while (current_ != end_) {
                        currentValue = *current_;
                        if (!isWordSymbol(currentValue))
                            break;
                        SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                    }
                    return value_token_type(STX_TOKEN_TYPE_WORD, StringT_(wordStart, current_));
                    
                } else if (currentTokenType == STX_TOKEN_TYPE_INTEGER) {
                    return createNumberToken(current_, column_, end_, current_);
                }  else if (currentTokenType == token_type::STX_TOKEN_TYPE_MINUS) {
                    SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                    if (flags & STX_EXT_TOKEN_TYPE_FLAG_BIT_SIGNED_NUMBERS) {
                        if ((current_ != end_) && (symbols_trait_type::type_from_char(*current_) == token_type::STX_TOKEN_TYPE_INTEGER))
                            return createNumberToken(current_, column_, end_, current_ - 1);
                    }
                    return value_token_type(currentTokenType,  StringT_(current_ - 1, current_));
                } else if (currentTokenType == STX_TOKEN_TYPE_DOUBLE_QUOTE) {
                    if (flags & STX_EXT_TOKEN_TYPE_FLAG_BIT_STRING_LETTERAL) {
                        const auto wordStart = current_;
                        SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                        bool slash = false;
                        while (current_ != end_) {
                            currentValue = *current_;
                            if (slash) {
                                slash = false;
                            } else {
                                const token_type tokenType = symbols_trait_type::type_from_char(currentValue);
                                if (tokenType == STX_TOKEN_TYPE_BACKCLASH) {
                                    slash = true;
                                } else if (tokenType == STX_TOKEN_TYPE_DOUBLE_QUOTE) {
                                    SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                                    return value_token_type(STX_TOKEN_TYPE_STRING_LETTERAL, StringT_(wordStart, current_));
                                }
                            }
                            SXT__NEXT_CHAR_V(current_, currentValue, line_, column_);
                        }
                    } else {
                        SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                        return value_token_type(STX_TOKEN_TYPE_DOUBLE_QUOTE, StringT_(current_ - 1, current_));
                    }

                } else if (!this_type_is_valid(currentTokenType)) {
                    if (flags & STX_EXT_TOKEN_TYPE_FLAG_BIT_UNKNOWN_AS_WORDS) {
                        SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                        return value_token_type(STX_TOKEN_TYPE_WORD, StringT_(current_ - 1, current_));
                    } else {
                        SXT_ASSERT(false); // unknown symbol
                        return value_token_type();
                    }
                } else {
                    SXT__NEXT_CHAR_WITHOUT_LINECHECK(current_, column_);
                    return value_token_type(currentTokenType,  StringT_(current_ - 1, current_));
                }
            }
            return value_token_type();
        }
        [[nodiscard]] size_t line() const noexcept {
            return line_;
        }
        [[nodiscard]] size_t column() const noexcept {
            return column_;
        }
        [[nodiscard]] bool eof() const noexcept {
            return (current_ != end_);
        }
    };
    const char* token_type_to_string(token_type tt) {
        switch (tt) {
            case STX_TOKEN_TYPE_WORD:
                return "WORD";
            case STX_TOKEN_TYPE_FLOAT:
                return "FLOAT";
            case STX_TOKEN_TYPE_INTEGER:
                return "INTEGER";
            case STX_TOKEN_TYPE_STRING_LETTERAL:
                return "STRING LETTERAL";
            case STX_TOKEN_TYPE_PLUS:
                return "PLUS";
            case STX_TOKEN_TYPE_MINUS:
                return "MINUS";
            case STX_TOKEN_TYPE_ASSIGN:
                return "ASSIGN";
            case STX_TOKEN_TYPE_LPAREN:
                return "LPAREN";
            case STX_TOKEN_TYPE_RPAREN:
                return "RPAREN";
            case STX_TOKEN_TYPE_LCURLY:
                return "LCURLY";
            case STX_TOKEN_TYPE_RCURLY:
                return "RCURLY";
            case STX_TOKEN_TYPE_DOT:
                return "DOT";
            case STX_TOKEN_TYPE_COMMA:
                return "COMMA";
            case STX_TOKEN_TYPE_COLON:
                return "COLON";
            case STX_TOKEN_TYPE_SEMICOLON:
                return "SEMICOLON";
            case STX_TOKEN_TYPE_QUOTE:
                return "QUOTE";
            case STX_TOKEN_TYPE_DOUBLE_QUOTE:
                return "DOUBLE QUOTE";
            case STX_TOKEN_TYPE_STAR:
                return "STAR";
            case STX_TOKEN_TYPE_TILDA:
                return "TILDA";
            case STX_TOKEN_TYPE_BACKCLASH:
                return "BACKCLASH";
            case STX_TOKEN_TYPE_MORE:
                return "MORE";
            case STX_TOKEN_TYPE_LESS:
                return "LESS";
            case STX_TOKEN_TYPE_EXCLAMATION:
                return "EXCLAMATION";
            case STX_TOKEN_TYPE_QUESTION:
                return "QUESTION";
            case STX_TOKEN_TYPE_INVALID:
                return "INVALID";
            case STX_TOKEN_TYPE_EOF:
                return "EOF";
            case STX_TOKEN_TYPE_MAX_ENUM:
            default:
                SXT_ASSERT(false);
                return "";
        }
        return "";
    }
    const wchar_t* token_type_to_wstring(token_type tt) {
        switch (tt) {
            case STX_TOKEN_TYPE_WORD:
                return L"WORD";
            case STX_TOKEN_TYPE_FLOAT:
                return L"FLOAT";
            case STX_TOKEN_TYPE_INTEGER:
                return L"INTEGER";
            case STX_TOKEN_TYPE_STRING_LETTERAL:
                return L"STRING LETTERAL";
            case STX_TOKEN_TYPE_PLUS:
                return L"PLUS";
            case STX_TOKEN_TYPE_MINUS:
                return L"MINUS";
            case STX_TOKEN_TYPE_ASSIGN:
                return L"ASSIGN";
            case STX_TOKEN_TYPE_LPAREN:
                return L"LPAREN";
            case STX_TOKEN_TYPE_RPAREN:
                return L"RPAREN";
            case STX_TOKEN_TYPE_LCURLY:
                return L"LCURLY";
            case STX_TOKEN_TYPE_RCURLY:
                return L"RCURLY";
            case STX_TOKEN_TYPE_DOT:
                return L"DOT";
            case STX_TOKEN_TYPE_COMMA:
                return L"COMMA";
            case STX_TOKEN_TYPE_COLON:
                return L"COLON";
            case STX_TOKEN_TYPE_SEMICOLON:
                return L"SEMICOLON";
            case STX_TOKEN_TYPE_QUOTE:
                return L"QUOTE";
            case STX_TOKEN_TYPE_DOUBLE_QUOTE:
                return L"DOUBLE QUOTE";
            case STX_TOKEN_TYPE_STAR:
                return L"STAR";
            case STX_TOKEN_TYPE_TILDA:
                return L"TILDA";
            case STX_TOKEN_TYPE_BACKCLASH:
                return L"BACKCLASH";
            case STX_TOKEN_TYPE_MORE:
                return L"MORE";
            case STX_TOKEN_TYPE_LESS:
                return L"LESS";
            case STX_TOKEN_TYPE_EXCLAMATION:
                return L"EXCLAMATION";
            case STX_TOKEN_TYPE_QUESTION:
                return L"QUESTION";
            case STX_TOKEN_TYPE_INVALID:
                return L"INVALID";
            case STX_TOKEN_TYPE_EOF:
                return L"EOF";
            case STX_TOKEN_TYPE_MAX_ENUM:
            default:
                SXT_ASSERT(false);
                return L"";
        }
        return L"";
    }
};

#endif //ifndef SXT_STX_HEAD_