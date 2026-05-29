#include "rnd.h"

// Transition tables. Each {target, probability} pair points at a state index.
// Probabilities need not be normalized (rnd_next_markov_state sums them).
static const rnd_markov_transition_t idle_transitions[] = {
  { .target = 0, .probability = 0.5f },  // Idle  -> Idle
  { .target = 1, .probability = 0.3f },  //       -> Working
  { .target = 3, .probability = 0.2f },  //       -> Paused
};

static const rnd_markov_transition_t working_transitions[] = {
  { .target = 1, .probability = 0.4f },  // Working -> Working
  { .target = 2, .probability = 0.4f },  //         -> Error
  { .target = 3, .probability = 0.2f },  //         -> Paused
};

static const rnd_markov_transition_t error_transitions[] = {
  { .target = 0, .probability = 1.0f },  // Error always restarts to Idle
};

static const rnd_markov_transition_t paused_transitions[] = {
  { .target = 0, .probability = 0.7f },  // Paused usually resumes Idle
  { .target = 1, .probability = 0.3f },  //        sometimes jumps to Working
};

static const rnd_markov_state_t states[] = {
  { .name = "Idle",    .transitions = idle_transitions,    .transition_count = 3 },
  { .name = "Working", .transitions = working_transitions, .transition_count = 3 },
  { .name = "Error",   .transitions = error_transitions,   .transition_count = 1 },
  { .name = "Paused",  .transitions = paused_transitions,  .transition_count = 2 },
};

static const size_t STATE_COUNT = sizeof(states) / sizeof(states[0]);

int currentState = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\xF0\x9F\x94\x81 Starting Markov Chain Walk...\n");
}

void loop() {
  const char* name = rnd_markov_state_name(currentState, states, STATE_COUNT);
  Serial.print("-> Current state: ");
  Serial.println(name);

  delay(1000);  // Simulate passage of time

  int next = rnd_next_markov_state(currentState, states, STATE_COUNT);
  if (next != currentState) {
    Serial.print("  Transitioning to: ");
    Serial.println(rnd_markov_state_name(next, states, STATE_COUNT));
  } else {
    Serial.println("  Staying in the same state.");
  }

  currentState = next;
  Serial.println();
}
