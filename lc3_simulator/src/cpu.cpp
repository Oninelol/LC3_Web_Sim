#include "cpu.h"
#include <iostream>
#include <iomanip>

CPUState::CPUState(){
    reset();
}

void CPUState::reset(){
    reg.fill(0);
    pc = 0x3000;
    n = false;
    z = true;
    p = false;
    cycles = 0;
    current_state = FSMState::FETCH_0;
    previous_state = FSMState::FETCH_0;

}

void CPUState::update_cond(uint16_t result){

}

uint16_t CPU::sign_extend(uint16_t val, int bits){

}

void CPU::transition_to(FSMState newstate){
    state.previous_state = state.current_state;
    state.current_state = newstate;
}

void CPU::execute_fetch0(){
    state.MAR = state.pc;
    state.pc++;
    transition_to(FSMState::FETCH_1);
}

void CPU::execute_fetch1(){
    state.MDR = memory[state.MAR];
    transition_to(FSMState::FETCH_2);
}

void CPU::execute_fetch2(){
    state.IR = state.MDR;
    transition_to(FSMState::FETCH_2);
}

void CPU::execute_decode(){
    uint16_t decode_temp = (state.IR >> 12);
    decode_temp = decode_temp & 0x000F; /* Bitmask to clear all bits to 0 except for the rightmost 4 bits */

    switch(decode_temp)
    {
        case 0x0001: transition_to(FSMState::ADD_0);
        case 0x0002: transition_to(FSMState::LD_0);
        case 0x0003: transition_to(FSMState::ST_0);
        case 0x0004: transition_to(FSMState::JSR_0);
        case 0x0005: transition_to(FSMState::AND_0);
        case 0x0006: transition_to(FSMState::LDR_0);
        case 0x0007: transition_to(FSMState::STR_0);
        case 0x0008: transition_to(FSMState::ERROR);
        case 0x0009: transition_to(FSMState::NOT_0);
        case 0x000A: transition_to(FSMState::LDI_0);
        case 0x000B: transition_to(FSMState::STI_0);
        case 0x000C: transition_to(FSMState::JMP_0);
        case 0x000D: transition_to(FSMState::RESERVED);
        case 0x000E: transition_to(FSMState::LEA_0);
        case 0x000F: transition_to(FSMState::TRAP_0);
        default: transition_to(FSMState::ERROR);    /* Perform operation based on opcode, IR[15:12] */
    }

}

void CPU::execute_add(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007; /* bitmask to only keep the 3 DR bits */
    uint16_t sr1 = state.IR >> 6;
    sr1 = sr1 & 0x0007; /* bitmask to only keep the 3 SR1 bits */
    uint16_t bit5 = state.IR >> 5;
    bit5 = bit5 & 0x0001; /* bitmask to only keep bit[5] of IR */
    uint16_t op2; /* declare op2 to add */
    if(bit5==1){
        op2 = state.IR & 0x001F; /* bitmask to only keep last 5 bits, imm5 */
        op2 = sign_extend(op2,5);
        state.reg[dr] = state.reg[sr1] + op2;   /* ADD DR,SR1,#imm5 */
    }
    else{
        op2 = state.IR & 0x0007; /* bitmask to only keep last 3 bits, SR2 */
        state.reg[dr] = state.reg[sr1] + state.reg[op2]; /* ADD DR,SR1,SR2 */
    }
    state.update_cond(state.reg[dr]);   /* setCC */
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_and(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007; /* keep DR bits */
    uint16_t sr1 = state.IR >> 6;
    sr1 = sr1 & 0x0007; /* bitmask to only keep the 3 sr1 bits */
    uint16_t bit5 = state.IR >> 5;
    bit5 = bit5 & 0x0001; /* IR[5] */
    uint16_t op2;
    if(bit5==1){
        op2 = state.IR & 0x001F; /* keep imm5 */
        op2 = sign_extend(op2,5);
        state.reg[dr] = state.reg[sr1] & op2;
    }
    else{
        op2 = state.IR & 0x0007; /* keep last 3 bits, SR2 */
        state.reg[dr] = state.reg[sr1] & state.reg[op2];
    }
    state.update_cond(state.reg[dr]);   /* setCC */
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_not(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007;   /* keep DR bits */
    uint16_t sr = state.IR >> 6;
    sr = sr & 0x0007; /* keep the 3 sr bits */
    state.reg[dr] = ~state.reg[sr]; /* DR <- NOT(SR) */
    state.update_cond(state.reg[dr]);   /* setCC */
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_trap0(){
    uint16_t trapvect = state.IR & 0x00FF; /* bitmask the 8 bit trap vector */
    state.MAR = trapvect;   /* MAR <- trapvect */
    transition_to(FSMState::TRAP_1);    
}

void CPU::execute_trap1(){
    state.MDR = memory[state.MAR];  /* MDR <- M[MAR] */
    state.reg[7] = state.pc;    /* R7 <- PC */
    transition_to(FSMState::TRAP_2);
}

void CPU::execute_trap2(){
    state.pc = state.MDR;
    if(state.IR & 0x00FF == 0x0025){
        transition_to(FSMState::HALT);
    }
    else{
    transition_to(FSMState::FETCH_0);
    }
}

void CPU::execute_lea0(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007; /* keep 3 bits of DR */
    uint16_t off9 = state.IR & 0x01FF; /* keep imm9 */
    off9 = sign_extend(off9,9);
    state.reg[dr] = state.pc + off9; /* DR <- PC + off9 */
    state.update_cond(state.reg[dr]); /* setCC */
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_ld0(){
    uint16_t off9 = state.IR & 0x01FF; /* keep imm9 */
    off9 = sign_extend(off9,9);
    state.MAR = state.pc + off9;
    transition_to(FSMState::LD_1);
}

void CPU::execute_ld1(){
    state.MDR = memory[state.MAR];
    transition_to(FSMState::LD_2);
}

void CPU::execute_ld2(){
    
}

