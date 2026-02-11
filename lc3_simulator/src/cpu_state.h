#pragma once
#include <array>
#include <cstdint>
#include <iostream>
struct CPUState{
    std::array<uint16_t,8> reg;     /* Initialize R0-R7*/
    uint16_t pc;    /* Initialize Program Counter in CPU */
    bool n,z,p;     /* Condition codes for n,z,p */
    CPUState();     /* Constructor */
    void reset();   /* Reset state of LC3 CPU*/
    void update_flags(uint16_t result);
    void print() const;
};
