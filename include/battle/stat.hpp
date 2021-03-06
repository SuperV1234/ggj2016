#pragma once

#include "base.hpp"
#include "game.hpp"

GGJ16_NAMESPACE
{
    enum class stat_type
    {
        health = 0,
        shield = 1,
        power = 2,
        maxhealth = 3,
        maxshield = 4,
        mana = 5,
        maxmana = 6
    };

    constexpr sz_t stat_count{7};

    using stat_value = float;

    using stat = stat_value;

    using stat_array = std::array<stat, stat_count>;

    template <typename TArray>
    decltype(auto) get_stat(stat_type type, TArray && array) noexcept
    {
        return array[vrmc::from_enum(type)];
    }
}
GGJ16_NAMESPACE_END
