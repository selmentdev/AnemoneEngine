#include "AnemoneRuntime/System/CommandLine.hxx"
#include "AnemoneRuntime/Diagnostics/Assert.hxx"
#include "AnemoneRuntime/System/Private/CommandLineStatics.hxx"

#include <algorithm>
#include <atomic>


namespace Anemone
{
    constexpr std::optional<std::string_view> ExtractTokenValue(std::string_view token, std::string_view name)
    {
        std::optional<std::string_view> result{};

        if (token.starts_with('-'))
        {
            // Option '-short' or '--long'
            token.remove_prefix(1);

            if (token.starts_with('-'))
            {
                // Long option '--option'
                token.remove_prefix(1);
            }

            if (token.starts_with(name))
            {
                // Option matched.
                token.remove_prefix(name.size());

                if (token.starts_with(':') or token.starts_with('='))
                {
                    // Matched '--option=value' or '--option:value'.
                    token.remove_prefix(1);
                    result = token;
                }
                else
                {
                    // Override result.
                    result = std::string_view{};
                }
            }
        }

        return result;
    }

    constexpr std::string_view UnquoteToken(std::string_view s)
    {
        if (s.starts_with('\"'))
        {
            // Starts with quote.
            s.remove_prefix(1);

            if (s.size() > 1)
            {
                std::string_view::size_type end = s.size() - 1;

                if ((s[end - 1] != '\\') and (s[end] == '"'))
                {
                    // Ends with non-escaped quote.
                    s.remove_suffix(1);
                }
            }
        }

        return s;
    }

    constexpr std::string_view ParseToken(std::string_view& s)
    {
        // Skip past whitespaces
        if (auto const start = s.find_first_not_of(" \t"); start != std::string_view::npos)
        {
            s.remove_prefix(start);
        }

        if (s.empty())
        {
            return {};
        }

        bool quoted = false;
        bool escaped = false;

        auto it = s.begin();
        auto const end = s.end();

        while (it != end)
        {
            char const ch = *it;

            if (quoted)
            {
                // Inside quoted token. Check for escapes.
                if (ch == '\\')
                {
                    escaped = true;
                }
                else if (ch == '\"')
                {
                    // Found quote character.
                    if (escaped)
                    {
                        // Escaped quote. Continue parsing.
                        escaped = false;
                    }
                    else
                    {
                        // Found closing quote. Break parsing.
                        // This means that we treat `--name="value"` as a single token.
                        // This also means that `--name="value""quoted"` is broken into two separate tokens.
                        ++it;
                        break;
                    }
                }
            }
            else
            {
                // Normal characters.
                if (ch == ' ' or ch == '\t')
                {
                    // Found whitespace. Break parsing.
                    break;
                }

                if (ch == '\"')
                {
                    // Found opening quote.
                    quoted = true;
                }
            }

            ++it;
        }

        std::string_view result{s.begin(), it};
        s.remove_prefix(result.size());
        return result;
    }
}

namespace Anemone
{
    auto CommandLine::GetOption(std::string_view name) -> std::optional<std::string_view>
    {
        std::optional<std::string_view> result{};

        for (int i = 1; i < Private::GCommandLineStatics->ArgC; ++i)
        {
            std::string_view token{Private::GCommandLineStatics->ArgV[i]};

            if (auto value = ExtractTokenValue(token, name); value.has_value())
            {
                result = *value;

                // NOTE: do not break this loop - we explicitly want last value provided by the user.
            }
        }

        return result;
    }

    void CommandLine::GetOption(std::string_view name, std::vector<std::string_view>& values)
    {
        for (int i = 1; i < Private::GCommandLineStatics->ArgC; ++i)
        {
            std::string_view token{Private::GCommandLineStatics->ArgV[i]};

            if (auto value = ExtractTokenValue(token, name))
            {
                values.emplace_back(*value);
            }
        }
    }

    void CommandLine::GetPositional(std::vector<std::string_view>& positional)
    {
        for (int i = 1; i < Private::GCommandLineStatics->ArgC; ++i)
        {
            std::string_view token{Private::GCommandLineStatics->ArgV[i]};

            if (token.starts_with('-'))
            {
                // Not a positional argument.
                continue;
            }

            positional.emplace_back(token);
        }
    }

    void CommandLine::Parse(
        std::string_view commandLine,
        std::vector<std::string_view>& positional,
        std::vector<std::string_view>& switches,
        std::vector<std::pair<std::string_view, std::string_view>>& options)
    {
        while (not commandLine.empty())
        {
            if (auto token = ParseToken(commandLine); not token.empty())
            {
                if (token.starts_with('-'))
                {
                    token.remove_prefix(1);

                    if (token.starts_with('-'))
                    {
                        token.remove_prefix(1);
                    }

                    // Named parameter prefixed with '-' or '--'.
                    auto separator = token.find_first_of("=:");

                    if (separator != std::string_view::npos)
                    {
                        std::string_view const name = token.substr(0, separator);
                        std::string_view value = token.substr(separator + 1);

                        // Remove quote from value.
                        value = UnquoteToken(value);

                        options.emplace_back(name, value);
                    }
                    else
                    {
                        // Token itself is an option.
                        switches.emplace_back(token);
                    }
                }
                else
                {
                    // Positional parameter.
                    positional.emplace_back(UnquoteToken(token));
                }
            }
        }
    }
}
