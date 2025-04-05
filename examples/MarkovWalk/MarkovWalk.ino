#include "rnd.hpp"

std::vector<MarkovState> states = {
  {"Idle",    {{0, 0.5f}, {1, 0.3f}, {3, 0.2f}}},   // Idle can go to Idle, Working, or Paused
  {"Working", {{1, 0.4f}, {2, 0.4f}, {3, 0.2f}}},   // Working can stay or go to Error/Paused
  {"Error",   {{0, 1.0f}}},                         // Error always restarts to Idle
  {"Paused",  {{0, 0.7f}, {1, 0.3f}}}               // Paused usually resumes Idle
};

int currentState = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("🔁 Starting Markov Chain Walk...\n");
}

void loop() {
  const char* name = currentMarkovStateName(currentState, states);
  Serial.print("→ Current state: ");
  Serial.println(name);

  delay(1000);  // Simulate passage of time

  int next = nextMarkovState(currentState, states);
  if (next != currentState) {
    Serial.print("  Transitioning to: ");
    Serial.println(currentMarkovStateName(next, states));
  } else {
    Serial.println("  Staying in the same state.");
  }

  currentState = next;
  Serial.println();
}
