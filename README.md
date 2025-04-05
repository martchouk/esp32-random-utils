=========================
ESP32 Random Utilities
=========================

A lightweight and powerful C++ library for generating random numbers on the ESP32, using its built-in hardware RNG via `esp_random()`. Includes bias-free integer and float generation, Gaussian (normal) distribution, weighted selection, exclusion filtering, and Markov chain state transitions.

---

## Features

- ✅ **Uniform random** integers (32-bit, 64-bit)
- ✅ **Signed** and **Arduino-style** `random()` support
- ✅ **Floating point** random numbers
- ✅ **Gaussian/Normal distribution** (Box-Muller transform)
- ✅ **Weighted random choice** (with normalized probabilities)
- ✅ **Exclusion-aware random range selection**
- ✅ **Markov chain random walks** for behavior/state modeling

---

## Installation

### From ZIP:
1. Download this repo as `.zip`
2. In Arduino IDE: **Sketch > Include Library > Add .ZIP Library**

### From GitHub (advanced):
Clone into your `libraries` folder:
```bash
git clone https://github.com/yourusername/esp32-random-utils.git
```

---

## Usage

```cpp
#include <rnd.hpp>

long r = random(-10, 10);                  // Uniform signed integer
float f = randomFloat(1.5f, 3.5f);         // Uniform float
int gauss = randomGaussianInt(100, 15);   // Int sampled from N(100, 15^2)
bool bit = randomBool();                  // True/false
```

### Weighted selection:
```cpp
String pet = weightedRandomFromList({
  {"Cat", 0.5f},
  {"Dog", 0.4f},
  {"Turtle", 0.1f}
}, true); // true = normalized weights
```

### Exclusion:
```cpp
int port = randomWithExclusions(1000, 1010, {1003, 1005});
```

### Markov Chains:
```cpp
std::vector<MarkovState> states = {
  {"Idle", {{0, 0.6f}, {1, 0.4f}}},
  {"Busy", {{1, 0.5f}, {0, 0.5f}}},
};
int state = 0;
state = nextMarkovState(state, states);
Serial.println(currentMarkovStateName(state, states));
```

---

## Example

See `examples/MarkovWalk/MarkovWalk.ino`

---

## License
MIT License. See LICENSE file for details.

---

## Author
**Your Name**  
GitHub: [@yourusername](https://github.com/yourusername)

---

## library.properties
```ini
name=ESP32 Random Utilities
version=1.0.0
author=Your Name
maintainer=your@email.com
sentence=Hardware-based random number tools for ESP32
paragraph=Includes uniform, Gaussian, weighted, and Markov chain random utilities using esp_random(). Designed for simulations, games, visuals, and procedural logic on ESP32.
category=Signal Input/Output
url=https://github.com/yourusername/esp32-random-utils
architectures=esp32
includes=rnd.hpp
```

---

## LICENSE (MIT)
```text
MIT License

Copyright (c) 2024 Your Name

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

