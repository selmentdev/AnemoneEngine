#pragma once
#include "AnemoneRuntime/Platform/Base/BaseHeaders.hxx"

#include <cstdint>
#include <span>
#include <compare>
#include <string_view>
#include <bit>
#include <optional>
#include <expected>
#include <charconv>
#include <array>

#include <fmt/format.h>

namespace Anemone::Math
{
    class Random;
}

namespace Anemone
{
    struct DateTime;
}

namespace Anemone
{
    struct Uuid final
    {
        uint8_t Elements[16];

        constexpr auto operator<=>(Uuid const&) const = default;
    };

    struct UuidParser final
    {
        static constexpr int ParseDigit(char c) noexcept
        {
            if (c >= '0' && c <= '9')
            {
                return static_cast<uint8_t>(c - '0');
            }

            if (c >= 'A' && c <= 'F')
            {
                return static_cast<uint8_t>(10 + c - 'A');
            }

            if (c >= 'a' && c <= 'f')
            {
                return static_cast<uint8_t>(10 + c - 'a');
            }

            return -1;
        }

        static constexpr auto Parse(std::string_view value) -> std::optional<Uuid>
        {
            //
            // Parse Uuid in format "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
            // X must be a hex digit, otherwise the function returns nullopt.
            //

            Uuid result;

            if (value.size() != 36)
            {
                return std::nullopt;
            }

            size_t current = 0;

            for (size_t i = 0; i < 16; ++i)
            {
                if ((i == 4) or (i == 6) or (i == 8) or (i == 10))
                {
                    if (value[current] != '-')
                    {
                        return std::nullopt;
                    }

                    ++current;
                }

                auto const hi = ParseDigit(value[current++]);
                auto const lo = ParseDigit(value[current++]);

                if ((hi < 0) || (lo < 0))
                {
                    return std::nullopt;
                }

                result.Elements[i] = (static_cast<uint8_t>(hi) << 4) | static_cast<uint8_t>(lo);
            }

            return result;
        }
    };

    struct UuidGenerator final
    {
        RUNTIME_API static Uuid CreateRandom(Math::Random& generator);

        RUNTIME_API static Uuid CreateSortable(Math::Random& generator);

        RUNTIME_API static Uuid CreateSortable(Math::Random& generator, DateTime dateTime);

        RUNTIME_API static Uuid CreateNamed(std::string_view name, uint64_t seed = 0);

        RUNTIME_API static Uuid CreateNamed(Uuid const& base, std::string_view name, uint64_t seed = 0);
    };

    inline constexpr Uuid NAMESPACE_DNS{
        {0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8},
    };
    inline constexpr Uuid NAMESPACE_OID{
        {0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8},
    };
    inline constexpr Uuid NAMESPACE_URL{
        {0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8},
    };
    inline constexpr Uuid NAMESPACE_X500{
        {0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8},
    };
}

template <>
struct fmt::formatter<Anemone::Uuid>
{
public:
    constexpr auto parse(auto& context) noexcept
    {
        return context.begin();
    }

    auto format(Anemone::Uuid const& value, auto& context) const noexcept
    {
        auto out = context.out();

        for (size_t i = 0; i < 16; ++i)
        {
            constexpr const char* digits = "0123456789abcdef";

            if (i == 4 || i == 6 || i == 8 || i == 10)
            {
                (*out++) = '-';
            }

            uint8_t const element{value.Elements[i]};
            (*out++) = digits[(element >> 4) & 0x0F];
            (*out++) = digits[element & 0x0F];
        }

        return out;
    }
};
