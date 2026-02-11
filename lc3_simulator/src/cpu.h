#pragma once
#include <stdio.h>
#include "cpu_state.h"
#include "memory.h"
#include <string>
#include <vector>

class CPU{
    public:
    CPU(); /* Constructor */
    ~CPU() = default;

    void step(); /* Execute one FSM state transition */
    void run(); /* Execute instructions until halt */
    void reset(); /* Reset CPU to initial state */


    /* To be added */

    private:    /*  functions to perform LC3 FSM in CPU */
    CPUState state; /* define variable state using CPUState in cpu_state.h */
    std::array<uint16_t, 65536> memory; /* Memory capped at xFFFF */

    void transition_to(FSMState new_state); /* Helper method to transition to different FSM states */
    uint16_t sign_extend(uint16_t val, int bits);

    void execute_fetch0();
    void execute_fetch1();
    void execute_fetch2();
    void execute_decode();  /* Functions for fetch & decode state */

    void execute_add();
    void execute_and();
    void execute_not(); /* functions for arithmetic & logical operations */

    void execute_trap0();
    void execute_trap1();
    void execute_trap2(); /* functions for TRAP operation */

    void execute_lea0(); /* function for LEA operation */

    void execute_ld0();
    void execute_ld1();
    void execute_ld2(); 

    void execute_ldr0();
    void execute_ldr1();
    void execute_ldr2();

    void execute_ldi0();
    void execute_ldi1();
    void execute_ldi2();
    void execute_ldi3();
    void execute_ldi4(); /* functions for LD, LDR, and LDI */

    void execute_sti0();
    void execute_sti1();
    void execute_sti2();
    void execute_sti3();
    void execute_sti4();

    void execute_str0();
    void execute_str1();
    void execute_str2();

    void execute_st0();
    void execute_st1();
    void execute_st2(); /* functions for ST, STR, and STI */

    void execute_jsr0();
    void execute_jsr1(); /* functions for JSR operation */

    void execute_jmp0(); /* function for JMP operation */

    void execute_br0();
    void execute_br1(); /* functions for BR operation */

    uint16_t temp_addr; /* Temporary address for multi-cycle operations */
    bool branch_taken;
    
};