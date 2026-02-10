![ESP32](https://img.shields.io/badge/Target-ESP32-blue?logo=espressif)
![Hardware RNG](https://img.shields.io/badge/Random-Hardware%20RNG-green?logo=entropy)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

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

### Gaussian/Normal distribution using Box-Muller transform
Returns a float sampled from N(mean, stddev^2)

It’s the famous bell curve: data tends to cluster around a central mean.
- 68% of values lie within 1 standard deviation from the mean.
- Common in natural systems: noise, temperature, human behavior, etc.

1. More natural randomness
If you’re building something that needs realistic behavior — like:
- Weather simulation
- Random blinking or dimming of lights
- Game AI decisions or enemy spawning
- Sensor simulation (simulate jitter or natural variability)

A uniform distribution feels artificial. Gaussian randomness feels alive.

2. Center-weighted events
Uniform random: random(0, 100) gives all values equally.
Gaussian: most values cluster near the mean, like 50, and far extremes (0, 100) are rare.

Use case:
- Particle trails cluster near origin
- LED bursts that fade naturally from center
- Random delays that feel more organic

3. Noise generation
- For adding smooth visual noise (Perlin/simplex over Gaussian samples)
- For sound/signal simulation
- To create “jitter” around a value that isn’t jumpy

Real-World Example: LED flickering effect
Instead of: int brightness = random(80, 120); // uniform
You could do: int brightness = gaussian(100, 10); // mean=100, std dev=10

Which results in:
- Most flickers close to 100
- Rare flashes above 120 or below 80
- 👀 Visually: looks more like candlelight

4. Machine learning or statistical modeling

If you’re ever:
- Doing data generation for training (mock inputs)
- Analyzing sensor data variation
- Implementing probabilistic decision trees

Gaussian distribution is the backbone.


### Markov chain random walk

A random walk is a special type of Markov process where:
- You randomly move from one state to another based on fixed probabilities.
- Imagine a drunk person walking on a line — each step is randomly left or right.

Use cases:
- Procedural scene navigation
- Smooth visual transitions
- Behavior modeling
- Game character states

You define a set of possible states and how likely you are to transition from one to another.
State A --> State A (40%)
        --> State B (30%)
        --> State C (30%)

State B --> A (20%)
        --> B (40%)
        --> C (40%)

Each time you update, you randomly choose the next state 
based on the current state’s transition probabilities.

Applications for Your Project

LED Panel / Display
- Smooth animation transitions between font styles, effects, or color palettes
- Simulate cellular automata or flocking behavior
- Procedurally evolve visuals: fade, noise, scroll, ripple in a natural pattern

Games or AI
- Define enemy behavior patterns with a Markov model
- Text generation (like GPT’s tiny cousin)
- Weather simulation: sunny → cloudy → rainy

Visualization
- Use states to create cyclical dashboards 
(e.g., scrolling weather, prices, messages) that don’t repeat predictably

---

## Installation

### From ZIP:
1. Download this repo as `.zip`
2. In Arduino IDE: **Sketch > Include Library > Add .ZIP Library**

### From GitHub:
Clone into your `libraries` folder:
```bash
git clone https://github.com/martchouk/esp32-random-utils.git
```

---

## Usage

```cpp
#include <rnd.hpp>

// --- Uniform randoms
uint32_t r1 = rnd(0, 100);                  // Unsigned int in [0, 99]
uint64_t r2 = rnd64(1000000000ULL, 2000000000ULL);  // 64-bit unsigned

// --- Arduino-style signed int
long s1 = random(-10, 10);                  // Signed int in [-10, 9]
long s2 = random(50);                       // Signed int in [0, 49]

// --- Float support
float f1 = randomFloat(1.5f, 3.5f);         // Float in [1.5, 3.5)

// --- Boolean
bool bit = randomBool();                   // true / false

// --- Gaussian distributions
float g1 = randomGaussian(100.0f, 15.0f);   // Float from N(100, 15^2)
int g2 = randomGaussianInt(100, 15);       // Int from same distribution
```

### Weighted selection:
```cpp
// Discrete choice with weights
std::string pet = weightedRandomFromList({
  {"Cat", 0.5f},
  {"Dog", 0.4f},
  {"Turtle", 0.1f}
}, true); // true = normalized weights
```

### Exclusion-based selection:
```cpp
// Random number in range [1000, 1010), excluding 1003 and 1005
int port = randomWithExclusions(1000, 1010, {1003, 1005});
```

### Markov Chain state transitions:
```cpp
std::vector<MarkovState> states = {
  {"Idle",    {{0, 0.6f}, {1, 0.4f}}},
  {"Working", {{1, 0.5f}, {2, 0.5f}}},
  {"Error",   {{0, 1.0f}}}
};

int currentState = 0;
for (int i = 0; i < 10; ++i) {
  Serial.printf("[%d] State: %s
", i, currentMarkovStateName(currentState, states));
  delay(500);
  currentState = nextMarkovState(currentState, states);
}
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
**Andrei Martchouk**  
GitHub: [@martchouk](https://github.com/martchouk)

