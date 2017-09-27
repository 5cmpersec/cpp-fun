#include <iostream>
#include <cassert>

enum class State {
  S_START_MACHINE,
  S_END_MACHINE,

  S_SAMPLE_0,
  S_SAMPLE_1,

  S_FAILURE,
 };

std::ostream& operator<<(std::ostream& out, State const state) {
  switch (state) {
  case State::S_START_MACHINE:
    return out << "S_START_MACHINE";
  case State::S_END_MACHINE:
    return out << "S_END_MACHINE";
  case State::S_SAMPLE_0:
    return out << "S_SAMPLE_0";
  case State::S_SAMPLE_1:
    return out << "S_SAMPLE_1";
  case State::S_FAILURE:
    return out << "S_FAILURE";
  default:
    assert(false);
  }
  return out << "S_INVALID";
}

enum class Transition {
  T_DEFAULT,

  T_ERROR,
};

std::ostream& operator<<(std::ostream& out, Transition const transition) {
  switch (transition) {
  case Transition::T_ERROR:
    return out << "T_ERROR";
  case Transition::T_DEFAULT:
    return out << "T_DEFAULT";
  default:
    assert(false);
  }
  return out << "T_INVALID";
}

struct StateMachineEntry {
  State source;
  Transition transition;
  State destination;
};

static const StateMachineEntry g_transitions[] {
  {State::S_START_MACHINE, Transition::T_DEFAULT, State::S_SAMPLE_0},

  {State::S_SAMPLE_0, Transition::T_DEFAULT, State::S_SAMPLE_1},
  {State::S_SAMPLE_0, Transition::T_ERROR, State::S_FAILURE},

  {State::S_SAMPLE_1, Transition::T_DEFAULT, State::S_SAMPLE_0},

  {State::S_FAILURE, Transition::T_DEFAULT, State::S_END_MACHINE},
};

State next(State state, Transition transition) {
  for (auto& entry : g_transitions) {
    if (entry.source == state && entry.transition == transition) {
      std::cout << "transition " << entry.source << " -> " << entry.transition \
      << " -> " << entry.destination << std::endl;
      return entry.destination;
    }
  }
  assert(false);
  return State::S_FAILURE;
}

struct Machine {
  State state = State::S_START_MACHINE;
  Transition transition = Transition::T_DEFAULT;

  int loop = 100;
};

Transition executeActionSample0(Machine& machine) {
  if (machine.loop > 0)
    return Transition::T_DEFAULT;
  return Transition::T_ERROR;
}

Transition executeActionSample1(Machine& machine) {
  machine.loop--;

  return Transition::T_DEFAULT;
}

Transition executeActionFailure(Machine& machine) {
  std::cout << "Failure!" << std::endl;

  return Transition::T_DEFAULT;
}

int main() {
  Machine machine = {};
  while (true) {
    machine.state = next(machine.state, machine.transition);

    switch (machine.state) {
    case State::S_END_MACHINE:
      return -1;
    case State::S_START_MACHINE:
    default:
      assert(false);
      return -1;

    case State::S_SAMPLE_0:
      machine.transition = executeActionSample0(machine);
      break;
    case State::S_SAMPLE_1:
      machine.transition = executeActionSample1(machine);
      break;
    case State::S_FAILURE:
      machine.transition = executeActionFailure(machine);
      break;
    }
  }
  return 0;
}
