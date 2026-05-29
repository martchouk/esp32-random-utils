#ifndef RND_H
#define RND_H

/*
ESP32 Hardware RND Utilities - Pure C Version
=============================================

This header provides reliable and bias-reduced random number generation
for ESP32 using its hardware RNG: esp_random().

Features:
---------
- Uniform int32 randoms
- Uniform uint64 randoms
- Uniform float randoms
- Gaussian / normal distribution
- Weighted integer selection
- Exclusion-aware integer selection
- Markov chain-based random walks

Notes:
------
- All functions are based on esp_random().
- esp_random() is hardware-based and is not seedable in the usual PRNG sense.
- rnd_seed() is provided only as an API-compatibility no-op.
- This is pure C: no std::vector, no std::pair, no std::string, no templates.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#include "esp_system.h"

#ifndef RND_PI_F
#define RND_PI_F 3.14159265358979323846f
#endif


/* ============================================================
   Core random integer functions
   ============================================================ */

/*
Uniform signed 32-bit int in [min, max).

Uses unsigned arithmetic to avoid signed integer overflow when the
requested interval spans almost the entire int32_t range.

The rejection-sampling threshold avoids modulo bias.
*/
static inline int32_t rnd_i32(int32_t min, int32_t max) {
    if (min >= max) {
        return min;
    }

    const uint32_t range = (uint32_t)max - (uint32_t)min;

    /*
      This threshold method is commonly used for unbiased bounded RNG.

      Since uint32_t arithmetic wraps modulo 2^32:
          -range == 2^32 - range

      threshold = (2^32 - range) % range

      Values below threshold are rejected so that the remaining domain
      size is an exact multiple of range.
    */
    const uint32_t threshold = (uint32_t)(-range) % range;

    uint32_t r;

    do {
        r = esp_random();
    } while (r < threshold);

    return min + (int32_t)(r % range);
}


/*
Raw full 64-bit hardware random value.

Internally combines two 32-bit esp_random() calls.
*/
static inline uint64_t rnd_u64_raw(void) {
    return ((uint64_t)esp_random() << 32) | (uint64_t)esp_random();
}


/*
Uniform unsigned 64-bit int in [min, max).

Uses rejection sampling to avoid modulo bias.
*/
static inline uint64_t rnd_u64(uint64_t min, uint64_t max) {
    if (min >= max) {
        return min;
    }

    const uint64_t range = max - min;
    const uint64_t threshold = (uint64_t)(-range) % range;

    uint64_t r;

    do {
        r = rnd_u64_raw();
    } while (r < threshold);

    return min + (r % range);
}


/*
Compatibility-style seed function.

esp_random() is a hardware RNG and is not seeded by user code.
This function intentionally does nothing.
*/
static inline void rnd_seed(unsigned long seed) {
    (void)seed;
}


/* ============================================================
   Long integer convenience wrappers
   ============================================================ */

/*
Uniform long in [min, max).

C has no function overloading, so this replaces the C++/Arduino-style:

    random(min, max)
*/
static inline long rnd_long_range(long min, long max) {
    if (min >= max) {
        return min;
    }

    /*
      This wrapper intentionally maps through int32_t because the original
      C++ library also used int32_t internally.

      On ESP32, long is normally 32-bit.
    */
    return (long)rnd_i32((int32_t)min, (int32_t)max);
}


/*
Uniform long in [0, max) if max > 0.
Uniform long in [max, 0) if max < 0.
Returns 0 if max == 0.

This replaces the C++/Arduino-style:

    random(max)
*/
static inline long rnd_long(long max) {
    if (max > 0) {
        return rnd_long_range(0, max);
    }

    if (max < 0) {
        return rnd_long_range(max, 0);
    }

    return 0;
}


/* ============================================================
   Float and boolean randoms
   ============================================================ */

/*
Uniform float in [min, max).

Uses an IEEE-754 mantissa trick:
- Take 23 random bits.
- Insert them into the mantissa of 1.0f.
- This creates a float in [1.0f, 2.0f).
- Subtract 1.0f to obtain [0.0f, 1.0f).

This avoids a float division.
*/
static inline float rnd_float(float min, float max) {
    if (min >= max) {
        return min;
    }

    union {
        uint32_t u;
        float f;
    } x;

    x.u = (esp_random() >> 9) | 0x3F800000u;

    return min + (max - min) * (x.f - 1.0f);
}


/*
Random boolean.
*/
static inline bool rnd_bool(void) {
    return (esp_random() & 1u) != 0u;
}


/* ============================================================
   Gaussian / normal distribution
   ============================================================ */

/*
Gaussian / normal distribution using Box-Muller transform.

Returns a float sampled from:

    N(mean, stddev^2)

The function uses the same IEEE-754 float trick as rnd_float().
*/
static inline float rnd_gaussian(float mean, float stddev) {
    union {
        uint32_t u;
        float f;
    } x1, x2;

    x1.u = (esp_random() >> 9) | 0x3F800000u;
    x2.u = (esp_random() >> 9) | 0x3F800000u;

    /*
      x1.f is in [1.0f, 2.0f), therefore:
          u1 = 2.0f - x1.f
      gives (0.0f, 1.0f], avoiding log(0).

      x2.f - 1.0f gives [0.0f, 1.0f).
    */
    const float u1 = 2.0f - x1.f;
    const float u2 = x2.f - 1.0f;

    const float z0 =
        sqrtf(-2.0f * logf(u1)) *
        cosf(2.0f * RND_PI_F * u2);

    return mean + z0 * stddev;
}


/*
Gaussian integer.

Samples from N(mean, stddev^2), then rounds to nearest int.
*/
static inline int rnd_gaussian_int(int mean, int stddev) {
    return (int)lroundf(
        rnd_gaussian((float)mean, (float)stddev)
    );
}


/* ============================================================
   Weighted integer selection
   ============================================================ */

/*
Weighted integer option.

Example:

    rnd_weighted_int_t options[] = {
        { .value = 0, .weight = 0.4f },
        { .value = 1, .weight = 0.3f },
        { .value = 2, .weight = 0.3f }
    };

    int result = rnd_weighted_int(options, 3, false);

The weights do not need to be normalized unless normalized == true.
*/
typedef struct {
    int value;
    float weight;
} rnd_weighted_int_t;


/*
Weighted random integer selection.

Parameters:
-----------
options:
    Pointer to array of rng_weighted_int_t.

count:
    Number of elements in options.

normalized:
    If false:
        total weight is calculated automatically.

    If true:
        total weight is assumed to be 1.0f.
        This is slightly faster, but only correct if your positive weights
        sum to approximately 1.0.

Returns:
--------
- selected option.value
- 0 if options == NULL or count == 0
- options[0].value if all weights are <= 0
*/
static inline int rnd_weighted_int(const rnd_weighted_int_t *options,
                                   size_t count,
                                   bool normalized) {
    if (options == NULL || count == 0) {
        return 0;
    }

    float total_weight = 0.0f;

    if (normalized) {
        total_weight = 1.0f;
    } else {
        for (size_t i = 0; i < count; ++i) {
            if (options[i].weight > 0.0f) {
                total_weight += options[i].weight;
            }
        }
    }

    if (total_weight <= 0.0f) {
        return options[0].value;
    }

    float r = rnd_float(0.0f, total_weight);

    for (size_t i = 0; i < count; ++i) {
        const float weight = options[i].weight;

        if (weight <= 0.0f) {
            continue;
        }

        if (r < weight) {
            return options[i].value;
        }

        r -= weight;
    }

    /*
      Fallback for rare floating-point edge cases.
    */
    return options[count - 1].value;
}


/*
Convenience wrapper for non-normalized weights.
*/
static inline int rnd_weighted_int_auto(const rnd_weighted_int_t *options,
                                        size_t count) {
    return rnd_weighted_int(options, count, false);
}


/*
Convenience wrapper for already-normalized weights.
*/
static inline int rnd_weighted_int_normalized(const rnd_weighted_int_t *options,
                                              size_t count) {
    return rnd_weighted_int(options, count, true);
}


/* ============================================================
   Exclusion-aware integer selection
   ============================================================ */

/*
Internal helper: checks whether an integer exists in an array.
*/
static inline bool rnd_contains_int(const int *values,
                                    size_t count,
                                    int value) {
    if (values == NULL) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        if (values[i] == value) {
            return true;
        }
    }

    return false;
}


/*
Uniform random integer from:

    [min, max) \ exclude

That means:
- sample from [min, max)
- but never return values contained in exclude[]

Parameters:
-----------
min, max:
    Range boundaries. max is exclusive.

exclude:
    Pointer to an array of excluded integers.
    May be NULL if exclude_count == 0.

exclude_count:
    Number of elements in exclude[].

Behavior:
---------
1. Fast path:
   Tries simple rejection sampling up to 16 times.

2. Slow path:
   If rejection sampling fails, counts all valid candidates and selects
   the k-th valid candidate.

Returns:
--------
- valid random candidate if one exists
- min if no valid candidate exists
*/
static inline int rnd_with_exclusions(int min,
                                      int max,
                                      const int *exclude,
                                      size_t exclude_count) {
    if (min >= max) {
        return min;
    }

    if (exclude == NULL || exclude_count == 0) {
        return (int)rnd_i32((int32_t)min, (int32_t)max);
    }

    /*
      Fast path: efficient when the exclusion set is small compared
      with the whole range.
    */
    for (int attempt = 0; attempt < 16; ++attempt) {
        const int candidate = (int)rnd_i32((int32_t)min, (int32_t)max);

        if (!rnd_contains_int(exclude, exclude_count, candidate)) {
            return candidate;
        }
    }

    /*
      Slow path: allocation-free deterministic selection.
    */
    int valid_count = 0;

    for (int i = min; i < max; ++i) {
        if (!rnd_contains_int(exclude, exclude_count, i)) {
            ++valid_count;
        }
    }

    if (valid_count == 0) {
        return min;
    }

    int target = (int)rnd_i32(0, valid_count);

    for (int i = min; i < max; ++i) {
        if (!rnd_contains_int(exclude, exclude_count, i)) {
            if (target == 0) {
                return i;
            }

            --target;
        }
    }

    /*
      Should be unreachable.
    */
    return min;
}


/* ============================================================
   Markov chain random walk
   ============================================================ */

/*
A Markov transition.

target:
    Index of the next state.

probability:
    Weight/probability of this transition.
    It may be normalized, e.g. 0.4f, 0.3f, 0.3f.
    It may also be non-normalized, e.g. 40.0f, 30.0f, 30.0f.
*/
typedef struct {
    int target;
    float probability;
} rnd_markov_transition_t;


/*
A Markov state.

name:
    Human-readable state name.
    The string is not copied. It must remain valid for as long as the
    Markov state is used.

transitions:
    Pointer to transition array.

transition_count:
    Number of transitions.
*/
typedef struct {
    const char *name;
    const rnd_markov_transition_t *transitions;
    size_t transition_count;
} rnd_markov_state_t;


/*
Returns the next Markov state index.

This function automatically normalizes transition probabilities by summing
all positive transition weights.

Therefore, both of the following are valid:

    0.4f, 0.3f, 0.3f

and:

    40.0f, 30.0f, 30.0f

Invalid target indices are ignored by returning the current state.
*/
static inline int rnd_next_markov_state(int current_index,
                                        const rnd_markov_state_t *states,
                                        size_t state_count) {
    if (states == NULL || state_count == 0) {
        return 0;
    }

    if (current_index < 0 || (size_t)current_index >= state_count) {
        return 0;
    }

    const rnd_markov_state_t *state = &states[current_index];

    if (state->transitions == NULL || state->transition_count == 0) {
        return current_index;
    }

    float total_weight = 0.0f;

    for (size_t i = 0; i < state->transition_count; ++i) {
        if (state->transitions[i].probability > 0.0f) {
            total_weight += state->transitions[i].probability;
        }
    }

    if (total_weight <= 0.0f) {
        return current_index;
    }

    float r = rnd_float(0.0f, total_weight);

    for (size_t i = 0; i < state->transition_count; ++i) {
        const float probability = state->transitions[i].probability;

        if (probability <= 0.0f) {
            continue;
        }

        if (r < probability) {
            const int target = state->transitions[i].target;

            if (target >= 0 && (size_t)target < state_count) {
                return target;
            }

            return current_index;
        }

        r -= probability;
    }

    /*
      Fallback for rare floating-point edge cases.
    */
    return current_index;
}


/*
Returns the name of the current Markov state.

Returns an empty string if:
- states == NULL
- index is out of range
- state name is NULL
*/
static inline const char *rnd_markov_state_name(int index,
                                                const rnd_markov_state_t *states,
                                                size_t state_count) {
    if (states == NULL) {
        return "";
    }

    if (index < 0 || (size_t)index >= state_count) {
        return "";
    }

    if (states[index].name == NULL) {
        return "";
    }

    return states[index].name;
}


#endif /* RND_H */
