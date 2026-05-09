#pragma once

template <typename T>
constexpr T align_up(T value, size_t alignment) noexcept {
    return (value + (alignment - 1)) & ~(alignment - 1);
}
