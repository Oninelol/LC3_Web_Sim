// bindings.cpp — Emscripten glue for the LC-3 core.
// Compiled with emcc, this exposes the CPU class (from your cpu.cpp/.h,
// cpu_state.h, memory.cpp/.h — all UNCHANGED) to JavaScript via embind.
//
// Build: see build_wasm.sh. After building you get lc3.js + lc3.wasm,
// and the page in web/index.html drives YOUR compiled C++ directly.

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "cpu.h"
#include "cpu_state.h"

using namespace emscripten;

void CPUState::print() const {}

// A thin wrapper that adds a few JS-friendly accessors on top of your CPU.
// Your CPU class is used as-is; this only surfaces state for the UI.
struct LC3Wasm {
  CPU cpu;

  void reset() { cpu.reset(); }

  // Load a program from a JS array of 16-bit words at a start address.
  void loadProgram(const val& words, uint16_t start) {
    unsigned len = words["length"].as<unsigned>();
    std::vector<uint16_t> buf(len);
    for (unsigned i = 0; i < len; i++) buf[i] = words[i].as<uint16_t>();
    cpu.load_program(buf.data(), (uint16_t)len, start);
  }

  void step() { cpu.step(); }

  // State accessors
  uint16_t reg(int i) const { return cpu.get_reg(i); }
  uint16_t pc()  const { return cpu.get_pc(); }
  uint16_t ir()  const { return cpu.get_ir(); }
  uint16_t mar() const { return cpu.get_mar(); }
  uint16_t mdr() const { return cpu.get_mdr(); }
  bool n() const { return cpu.getn(); }
  bool z() const { return cpu.getz(); }
  bool p() const { return cpu.getp(); }
  bool running() const { return const_cast<CPU&>(cpu).is_running(); }
  bool halted()  const { return const_cast<CPU&>(cpu).is_halted(); }
  unsigned cycles() const { return (unsigned)cpu.get_cycle_count(); }

  // FSM state — uses your fsm_state_to_string / fsm_state_description directly
  std::string stateName() const { return cpu.get_state_name(); }
  std::string stateDesc() const { return cpu.get_state_desc(); }

  uint16_t readMem(uint16_t a) { return cpu.read_memory(a); }
};

EMSCRIPTEN_BINDINGS(lc3_module) {
  class_<LC3Wasm>("LC3")
    .constructor<>()
    .function("reset", &LC3Wasm::reset)
    .function("loadProgram", &LC3Wasm::loadProgram)
    .function("step", &LC3Wasm::step)
    .function("reg", &LC3Wasm::reg)
    .function("pc", &LC3Wasm::pc)
    .function("ir", &LC3Wasm::ir)
    .function("mar", &LC3Wasm::mar)
    .function("mdr", &LC3Wasm::mdr)
    .function("n", &LC3Wasm::n)
    .function("z", &LC3Wasm::z)
    .function("p", &LC3Wasm::p)
    .function("running", &LC3Wasm::running)
    .function("halted", &LC3Wasm::halted)
    .function("cycles", &LC3Wasm::cycles)
    .function("stateName", &LC3Wasm::stateName)
    .function("stateDesc", &LC3Wasm::stateDesc)
    .function("readMem", &LC3Wasm::readMem);
}