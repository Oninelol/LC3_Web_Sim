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

    CPUState& get_state()
    {
        return state;
    }
    const CPUState& get_state() const{
        return state;
    }      /* Access States */

    uint16_t get_reg(int r) const {
        return state.reg[r];
    }

    void set_reg(int r,uint16_t val)
    {
        state.reg[r] = val;
    }   /* Access registers */

    uint16_t get_pc() const{
        return state.pc;
    }

    void set_pc(uint16_t val)
    {
        state.pc = val;
    }   /* Access PC, program counter of the CPU */

    uint16_t get_mdr() const{
        return state.MDR;
    }

    uint16_t get_mar() const{
        return state.MAR;
    }

    uint16_t get_ir() const{
        return state.IR;
    }   /* Get the values of IR, MAR, and MDR when called */

    bool getn() const{
        return state.n;
    }

    bool getz() const{
        return state.z;
    }

    bool getp() const{
        return state.p;
    }   /* Access control signals nzp of the CPU */

    uint16_t read_memory(uint16_t addr)
    {
        return memory[addr];
    }

    uint16_t write_memory(uint16_t addr,uint16_t write){
        memory[addr] = write;
    }   /* Build the memory R/W system in the LC3 CPU (Datapath) */

    void load_program(const uint16_t* prog,uint16_t len,uint16_t start = 0x3000);   /* Load the program into LC3 data, default start at x3000 */

    bool is_running(){
        return state.running;
    }   /* check whether if the program is running */

    bool is_halted(){
        return state.halted;
    }   /* check whether if the program is halted */

    uint16_t get_cycle_count() const{
        return state.cycles;
    }   /* get the total cycles that program has executed for */

    FSMState get_current_state() const{
        return state.current_state;
    }

    FSMState get_previous_state() const{
        return state.previous_state;
    }   /* Get current and previous state */

    std::string get_state_name() const{
        return fsm_state_to_string(state.current_state);
    }

    std::string get_state_desc() const{
        return fsm_state_description(state.current_state);
    }
    /* Get state name and description */


    private:   
     /*  functions to perform LC3 FSM in CPU */
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
    void execute_jsr1();
    void execute_jsr2(); /* functions for JSR operation */

    void execute_jmp0(); /* function for JMP operation */

    void execute_br0();
    void execute_br1(); /* functions for BR operation */

    uint16_t temp_addr; /* Temporary address for multi-cycle operations */
    bool branch_taken;
    
};