#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <utility>
#include <algorithm>
#include <string>
#include "esp_system.h"

/*
ESP32 Hardware RNG Utilities
=============================================
This header provides reliable and bias-free random number generation
for ESP32 using its hardware RNG (esp_random()).

Includes:
---------
- Uniform int/float randoms
- Gaussian (normal) distribution
- Weighted random selection
- Exclusion-aware selection
- Markov chain-based random walks
*/

// Uniform 32-bit int in [min, max)
inline int32_t rnd(int32_t min, int32_t max) {
  if (min >= max) return min;
  uint32_t range = static_cast<uint32_t>(max - min);
  uint32_t r;
  // Rejection sampling to eliminate modulo bias
  uint32_t limit = UINT32_MAX - (UINT32_MAX % range);
  do {
    r = esp_random();
  } while (r >= limit);
  return min + static_cast<int32_t>(r % range);
}

// Full 64-bit hardware random
inline uint64_t rnd64() {
  return (static_cast<uint64_t>(esp_random()) << 32) | esp_random();
}

// 64-bit range [min, max)
inline uint64_t rnd64(uint64_t min, uint64_t max) {
  if (min >= max) return min;
  uint64_t range = max - min;
  uint64_t r;
  uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
  do { 
    r = rnd64(); 
  } while (r >= limit);
  return min + (r % range);
}

// Arduino-style signed random [min, max)
inline long random(long min, long max) {
  return static_cast<long>(rnd(static_cast<int32_t>(min), static_cast<int32_t>(max)));
}

inline long random(long max) {
  if (max > 0) return random(0, max);
  if (max < 0) return random(max, 0);
  return 0;
}

// NOTE: esp_random() is a hardware RNG and is NOT affected by srandom().
// Keeping this for compatibility with code that expects the function to exist.
inline void randomSeed(unsigned long seed) { 
  ::srandom(seed); 
}

inline float randomFloat(float min, float max) {
  if (min >= max) return min;
  // Use 1.0f / (UINT32_MAX + 1.0f) to get a range of [0, 1)
  return min + (static_cast<float>(esp_random()) / 4294967296.0f) * (max - min);
}

inline bool randomBool() {
  return (esp_random() & 0x01);
}

// Gaussian/Normal distribution using Box-Muller transform
// Returns a float sampled from N(mean, stddev^2)
/*

It’s the famous bell curve: data tends to cluster around a central mean.
- 68% of values lie within 1 standard deviation from the mean.
- Common in natural systems: noise, temperature, human behavior, etc.

1 More natural randomness
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
- Visually: looks more like candlelight

4. Machine learning or statistical modeling

If you’re ever:
- Doing data generation for training (mock inputs)
- Analyzing sensor data variation
- Implementing probabilistic decision trees

Gaussian distribution is the backbone.
*/
inline float randomGaussian(float mean, float stddev) {
  // We need u1 to be in (0, 1] to avoid log(0)
  // esp_random returns [0, UINT32_MAX], so we map to (0, 1]
  float u1 = (static_cast<float>(esp_random()) + 1.0f) / 4294967296.0f;
  float u2 = static_cast<float>(esp_random()) / 4294967296.0f;
  
  float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * M_PI * u2);
  return mean + z0 * stddev;
}

inline int randomGaussianInt(int mean, int stddev) {
  return static_cast<int>(std::round(randomGaussian(static_cast<float>(mean), static_cast<float>(stddev))));
}

// Weighted selection (Pass vector by const reference to avoid expensive copies)
template <typename T>
T weightedRandomFromList(const std::vector<std::pair<T, float>>& options, bool normalized = false) {
  if (options.empty()) return T();

  float totalWeight = 0.0f;
  if (normalized) {
    totalWeight = 1.0f;
  } else {
    for (const auto& opt : options) {
      if (opt.second > 0.0f) totalWeight += opt.second;
    }
  }

  if (totalWeight <= 0.0f) return options.front().first;
  
  float r = randomFloat(0.0f, totalWeight);
  for (const auto& opt : options) {
    if (opt.second <= 0.0f) continue;
    if (r < opt.second) return opt.first;
    r -= opt.second;
  }
  return options.back().first;
}

// Exclusion-aware integer (FIXED: Rejection sampling used instead of heap allocation)
inline int randomWithExclusions(int min, int max, const std::vector<int>& exclude) {
  if (min >= max) return min;
  
  // If the exclusion list is nearly the size of the range, this could loop infinitely.
  // Adding a safety counter just in case.
  int attempts = 0;
  int r;
  bool foundExcluded;
  
  do {
    r = random(min, max);
    foundExcluded = false;
    for (int ex : exclude) {
      if (r == ex) {
        foundExcluded = true;
        break;
      }
    }
    attempts++;
  } while (foundExcluded && attempts < 100); 
  
  return r;
}

/* Markov chain random walk
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
- Text generation (like GPT’s tiny cousin 😄)
- Weather simulation: sunny → cloudy → rainy

Visualization
- Use states to create cyclical dashboards 
(e.g., scrolling weather, prices, messages) that don’t repeat predictably
*/
struct MarkovState {
  std::string name;
  std::vector<std::pair<int, float>> transitions; // target index, probability
};

inline int nextMarkovState(int currentIndex, const std::vector<MarkovState>& states) {
  if (currentIndex < 0 || currentIndex >= static_cast<int>(states.size())) return 0;
  // Transitions are generally normalized (sum to 1.0)
  return weightedRandomFromList(states[currentIndex].transitions, true);
}

inline const char* currentMarkovStateName(int index, const std::vector<MarkovState>& states) {
  if (index < 0 || index >= static_cast<int>(states.size())) return "Unknown";
  return states[index].name.c_str();
}
