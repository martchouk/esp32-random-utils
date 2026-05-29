![ESP32](https://img.shields.io/badge/Target-ESP32-blue?logo=espressif)
![Hardware RNG](https://img.shields.io/badge/Random-Hardware%20RNG-green?logo=entropy)
![Language](https://img.shields.io/badge/Language-Pure%20C-orange.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

ESP32 Random Utilities
=========================

A lightweight **pure C** library for generating random numbers on the ESP32,
using its built-in hardware RNG via `esp_random()`. Includes bias-reduced
integer and float generation, Gaussian (normal) distribution, weighted
selection, exclusion filtering, and Markov chain state transitions.

No STL, no templates, no heap allocation — just `static inline` functions in a
single header. Usable from both C and C++ translation units.

> **⚠️ v2.0 is a breaking change.** The library was rewritten in pure C and the
> entire API was renamed to a `rnd_*` prefix. See the [Migrating from
> 1.x](#migrating-from-1x-c) table below.

---

## Features

- ✅ **Uniform random** integers — signed 32-bit and unsigned 64-bit
- ✅ **Arduino-style** `long` range helpers
- ✅ **Floating point** random numbers (IEEE-754 mantissa trick, no divide)
- ✅ **Gaussian / Normal distribution** (Box–Muller transform)
- ✅ **Weighted random choice** (normalized or auto-normalized)
- ✅ **Exclusion-aware** random range selection (allocation-free)
- ✅ **Markov chain random walks** for behavior/state modeling

All functions are unbiased where it matters: `rnd_i32` / `rnd_u64` use
rejection sampling to eliminate modulo bias.

### Gaussian / Normal distribution (Box–Muller)

Returns a float sampled from `N(mean, stddev²)` — the classic bell curve where
values cluster around the mean (68% within 1σ). Useful when uniform randomness
feels artificial:

- LED flicker that looks like candlelight (`rnd_gaussian(100, 10)` instead of
  `rnd_i32(80, 120)`)
- Particle trails / bursts that fade naturally from a center
- Organic-feeling delays and jitter

### Markov chain random walk

You define a set of states and the probabilities of moving between them; each
update picks the next state from the current state's transitions:

```
State A --> A (40%)  --> B (30%)  --> C (30%)
State B --> A (20%)  --> B (40%)  --> C (40%)
```

Useful for procedural scene navigation, smooth visual transitions, behavior
modeling, and cyclical dashboards that don't repeat predictably. Transition
weights may be normalized (`0.4, 0.3, 0.3`) or not (`40, 30, 30`) — they are
summed automatically.

---

## Installation

### From ZIP
1. Download this repo as `.zip`
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**

### From GitHub
Clone into your `libraries` folder:
```bash
git clone https://github.com/martchouk/esp32-random-utils.git
```

---

## Usage

```c
#include "rnd.h"

// --- Uniform integers (half-open [min, max) intervals)
int32_t  a = rnd_i32(-10, 10);              // signed 32-bit in [-10, 9]
uint64_t b = rnd_u64(1000, 1000000);        // unsigned 64-bit in [1000, 999999]
uint64_t r = rnd_u64_raw();                 // raw full 64-bit value

// --- Arduino-style long helpers
long s1 = rnd_long_range(-10, 10);          // signed long in [-10, 9]
long s2 = rnd_long(50);                      // signed long in [0, 49]

// --- Float and boolean
float f = rnd_float(1.5f, 3.5f);            // float in [1.5, 3.5)
bool  k = rnd_bool();                        // true / false

// --- Gaussian / normal
float g  = rnd_gaussian(100.0f, 15.0f);     // float from N(100, 15²)
int   gi = rnd_gaussian_int(100, 15);       // rounded int from same distribution

// --- API-compatibility no-op (esp_random() is HW-based and not seedable)
rnd_seed(0);
```

### Weighted selection

```c
rnd_weighted_int_t options[] = {
    { .value = 0, .weight = 0.4f },
    { .value = 1, .weight = 0.3f },
    { .value = 2, .weight = 0.3f },
};

int picked = rnd_weighted_int_auto(options, 3);        // auto-sum weights
// int picked = rnd_weighted_int_normalized(options, 3); // if weights sum to 1.0
```

### Exclusion-based selection

```c
int excluded[] = { 2, 4, 7 };
int value = rnd_with_exclusions(0, 10, excluded, 3);   // one of {0,1,3,5,6,8,9}
```

### Markov chain state transitions

```c
static const rnd_markov_transition_t idle_t[]   = { {0, 0.5f}, {1, 0.3f}, {3, 0.2f} };
static const rnd_markov_transition_t work_t[]   = { {1, 0.4f}, {2, 0.4f}, {3, 0.2f} };
static const rnd_markov_transition_t error_t[]  = { {0, 1.0f} };
static const rnd_markov_transition_t paused_t[] = { {0, 0.7f}, {1, 0.3f} };

static const rnd_markov_state_t states[] = {
    { .name = "Idle",   .transitions = idle_t,   .transition_count = 3 },
    { .name = "Working",.transitions = work_t,   .transition_count = 3 },
    { .name = "Error",  .transitions = error_t,  .transition_count = 1 },
    { .name = "Paused", .transitions = paused_t, .transition_count = 2 },
};

int state = 0;
state = rnd_next_markov_state(state, states, 4);
const char *name = rnd_markov_state_name(state, states, 4);
```

> The `name` strings and transition arrays are **not copied** — they must
> outlive the states that reference them (use `static`/global storage, as above).

---

## API reference

| Function | Returns | Description |
|---|---|---|
| `rnd_i32(min, max)` | `int32_t` | Uniform signed int in `[min, max)`, unbiased |
| `rnd_u64(min, max)` | `uint64_t` | Uniform unsigned 64-bit in `[min, max)`, unbiased |
| `rnd_u64_raw()` | `uint64_t` | Raw full 64-bit hardware value |
| `rnd_long_range(min, max)` | `long` | Uniform `long` in `[min, max)` |
| `rnd_long(max)` | `long` | `[0, max)` if `max>0`, `[max, 0)` if `max<0`, else `0` |
| `rnd_float(min, max)` | `float` | Uniform float in `[min, max)` |
| `rnd_bool()` | `bool` | Random `true`/`false` |
| `rnd_gaussian(mean, stddev)` | `float` | Sample from `N(mean, stddev²)` |
| `rnd_gaussian_int(mean, stddev)` | `int` | Gaussian sample rounded to nearest int |
| `rnd_seed(seed)` | `void` | No-op; HW RNG is not seedable (API compat) |
| `rnd_weighted_int(opts, count, normalized)` | `int` | Weighted choice over `rnd_weighted_int_t[]` |
| `rnd_weighted_int_auto(opts, count)` | `int` | Weighted choice, weights summed automatically |
| `rnd_weighted_int_normalized(opts, count)` | `int` | Weighted choice, assumes weights sum to 1.0 |
| `rnd_with_exclusions(min, max, exclude, n)` | `int` | Uniform from `[min, max) \ exclude[]` |
| `rnd_next_markov_state(cur, states, n)` | `int` | Next Markov state index |
| `rnd_markov_state_name(idx, states, n)` | `const char *` | State name (or `""` if out of range) |

---

## Migrating from 1.x (C++)

v1.x was a C++ header (`rnd.hpp`) with STL-based weighted/Markov helpers. v2.0
is pure C (`rnd.h`). Update your `#include` and rename calls:

| 1.x (C++, `rnd.hpp`) | 2.0 (C, `rnd.h`) |
|---|---|
| `#include <rnd.hpp>` | `#include "rnd.h"` |
| `rnd(a, b)` | `rnd_i32(a, b)` |
| `rnd64()` | `rnd_u64_raw()` |
| `rnd64(a, b)` | `rnd_u64(a, b)` |
| `random(a, b)` | `rnd_long_range(a, b)` |
| `random(a)` | `rnd_long(a)` |
| `randomFloat(a, b)` | `rnd_float(a, b)` |
| `randomBool()` | `rnd_bool()` |
| `randomGaussian(m, s)` | `rnd_gaussian(m, s)` |
| `randomGaussianInt(m, s)` | `rnd_gaussian_int(m, s)` |
| `randomSeed(s)` | `rnd_seed(s)` |
| `weightedRandomFromList<int>({{v,w},...}, norm)` | `rnd_weighted_int(opts, count, norm)` |
| `randomWithExclusions(min, max, {…})` | `rnd_with_exclusions(min, max, arr, n)` |
| `struct MarkovState { std::string; std::vector<…> }` | `rnd_markov_state_t` + `rnd_markov_transition_t` |
| `nextMarkovState(i, states)` | `rnd_next_markov_state(i, states, count)` |
| `currentMarkovStateName(i, states)` | `rnd_markov_state_name(i, states, count)` |

The generic `weightedRandomFromList<T>` template is gone; the C version selects
an `int value`. For non-int payloads, store an index and look it up.

---

## Build notes

Because the library uses `sqrtf()`, `logf()`, `cosf()`, and `lroundf()`, some
build systems need the math library linked:

```
-lm
```

Under ESP-IDF / Arduino-ESP32 this is handled for you. On recent ESP-IDF,
`esp_random()` is declared in `esp_random.h`; the header includes
`esp_system.h`, which transitively provides it under Arduino-ESP32.

---

## Example

See [`examples/MarkovWalk/MarkovWalk.ino`](examples/MarkovWalk/MarkovWalk.ino).

---

## License
MIT License. See [LICENSE](LICENSE) for details.

---

## Author
**Andrei Martchouk**
GitHub: [@martchouk](https://github.com/martchouk)
