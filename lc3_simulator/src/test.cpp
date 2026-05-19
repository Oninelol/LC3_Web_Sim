// test.cpp
#include "cpu.h"
#include <iostream>
#include <cstdlib>

#define ASSERT_EQ(actual, expected) do { \
    auto _a = (actual); \
    auto _e = (expected); \
    if (_a != _e) { \
        std::cerr << "FAIL " << __func__ << " line " << __LINE__ \
                  << ": expected " << _e << " (0x" << std::hex << _e << std::dec \
                  << "), got " << _a << " (0x" << std::hex << _a << std::dec << ")\n"; \
        std::exit(1); \
    } \
} while(0)

// Sums 1+2+3+4+5 = 15 using a countdown loop.
// R1 = counter (starts at 5, decrements to 0)
// R0 = accumulator (starts at 0, ends at 15)
//
// Hand-assembled:
//   x3000: 5025  AND R0, R0, #0     ; R0 = 0  (clear accumulator)
//   x3001: 5260  AND R1, R1, #0     ; R1 = 0
//   x3002: 1265  ADD R1, R1, #5     ; R1 = 5  (counter)
//   x3003: 0403  BRz  #3            ; LOOP_END if R1 == 0  (skip 3 instrs)
//   x3004: 1001  ADD R0, R0, R1     ; R0 += R1   (note: no setcc check needed)
//   x3005: 127F  ADD R1, R1, #-1    ; R1 -= 1   (sets NZP based on R1)
//   x3006: 0FFC  BRnzp #-4          ; jump back to BRz at x3003
//   x3007: F025  TRAP x25 (HALT)
void test_sum_loop() {
    CPU cpu;
    uint16_t prog[] = {
        0x5020,  // AND R0, R0, #0
        0x5260,  // AND R1, R1, #0
        0x1265,  // ADD R1, R1, #5
        0x0403,  // BRz #3
        0x1001,  // ADD R0, R0, R1
        0x127F,  // ADD R1, R1, #-1
        0x0FFC,  // BRnzp #-4
        0xF025,  // TRAP x25 HALT
    };
    cpu.load_program(prog, 8, 0x3000);
    cpu.run();

    // Final state checks
    ASSERT_EQ(cpu.get_reg(0), 15);          // 1+2+3+4+5
    ASSERT_EQ(cpu.get_reg(1), 0);           // counter ran out
    ASSERT_EQ(cpu.is_halted(), true);       // hit TRAP x25
    ASSERT_EQ(cpu.is_running(), false);     // run() exited cleanly

    // NZP from the final BRz (when R1 hit 0, Z was set)
    ASSERT_EQ(cpu.getz(), true);
    ASSERT_EQ(cpu.getn(), false);
    ASSERT_EQ(cpu.getp(), false);

    std::cout << "test_sum_loop PASSED  (cycles = " << cpu.get_cycle_count() << ")\n";
}

int main() {
    test_sum_loop();
    return 0;
}