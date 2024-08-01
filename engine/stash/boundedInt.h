#include <type_traits>
#include <cstdint>
#include <cassert>

#ifdef NDEBUG
#define BOUNDS_CHECKING_ENABLED 0
#else
#define BOUNDS_CHECKING_ENABLED 1
#endif

template <intmax_t Lower, intmax_t Upper>
struct SmallestType {
    static_assert(Lower <= Upper, "Lower bound must be less than or equal to Upper bound");

    using type = std::conditional_t<
        (Lower == 0 && Upper <= UINT8_MAX), uint8_t,
        std::conditional_t<
            (Lower == 0 && Upper <= UINT16_MAX), uint16_t,
            std::conditional_t<
                (Lower == 0 && Upper <= UINT32_MAX), uint32_t,
                std::conditional_t<
                    (Lower >= INT8_MIN && Upper <= INT8_MAX), int8_t,
                    std::conditional_t<
                        (Lower >= INT16_MIN && Upper <= INT16_MAX), int16_t,
                        std::conditional_t<
                            (Lower >= INT32_MIN && Upper <= INT32_MAX), int32_t,
                            int64_t
                        >
                    >
                >
            >
        >
    >;
};

template <intmax_t Lower, intmax_t Upper>
using SmallestType_t = typename SmallestType<Lower, Upper>::type;

template <intmax_t Lower, intmax_t Upper, typename T = SmallestType_t<Lower, Upper>>
class BoundedValue {
private:
    T value;
    static constexpr T lowerBound = Lower;
    static constexpr T upperBound = Upper;

public:
    BoundedValue(T initValue = Lower) {
        setValue(initValue);
    }

    void setValue(T newValue) {
#if BOUNDS_CHECKING_ENABLED
        assert(newValue >= lowerBound && newValue <= upperBound);
#endif
        value = newValue;
    }

    T getValue() const {
        return value;
    }

    // Addition
    BoundedValue operator+(const BoundedValue& other) const {
        return BoundedValue(value + other.value);
    }

    // Subtraction
    BoundedValue operator-(const BoundedValue& other) const {
        return BoundedValue(value - other.value);
    }

    // Multiplication
    BoundedValue operator*(const BoundedValue& other) const {
        return BoundedValue(value * other.value);
    }

    // Division
    BoundedValue operator/(const BoundedValue& other) const {
        assert(other.value != 0);
        return BoundedValue(value / other.value);
    }
};
