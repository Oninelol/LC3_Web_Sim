#include "cpu.h"
#include <iostream>
#include <iomanip>

CPUState::CPUState(){
    reset();
}

void CPU::load_program(const uint16_t* prog, uint16_t len, uint16_t start) {
    memory.load_program(prog, len, start);
    state.pc = start;  // PC should point at the loaded program
}

void CPUState::reset(){
    reg.fill(0);
    pc = 0x3000;
    n = false;
    z = true;
    p = false;
    IR = 0;
    MAR = 0;
    MDR = 0;
    running = true;
    halted = false;
    cycles = 0;
    current_state = FSMState::FETCH_0;
    previous_state = FSMState::FETCH_0;
}

void CPU::reset(){
    state.reset();
    memory.clear_memory();
}

void CPU::step() {
    if (!state.running) return;

    if(state.current_state == FSMState::FETCH_0){
        std::cerr << "PC=0x" << std::hex << state.pc
                  << "  IR=0x" << state.IR
                  << "  R0=" << std::dec << state.reg[0]
                  << "  R1=" << state.reg[1]
                  << "  NZP=" << state.n << state.z << state.p
                  << "\n";
    }

    switch (state.current_state) {
        case FSMState::FETCH_0: execute_fetch0(); break;
        case FSMState::FETCH_1: execute_fetch1(); break;
        case FSMState::FETCH_2: execute_fetch2(); break;
        case FSMState::DECODE:  execute_decode();  break;

        case FSMState::ADD_0: execute_add(); break;
        case FSMState::AND_0: execute_and(); break;
        case FSMState::NOT_0: execute_not(); break;

        case FSMState::TRAP_0: execute_trap0(); break;
        case FSMState::TRAP_1: execute_trap1(); break;
        case FSMState::TRAP_2: execute_trap2(); break;

        case FSMState::LEA_0: execute_lea0(); break;

        case FSMState::LD_0: execute_ld0(); break;
        case FSMState::LD_1: execute_ld1(); break;
        case FSMState::LD_2: execute_ld2(); break;

        case FSMState::LDR_0: execute_ldr0(); break;
        case FSMState::LDR_1: execute_ldr1(); break;
        case FSMState::LDR_2: execute_ldr2(); break;

        case FSMState::LDI_0: execute_ldi0(); break;
        case FSMState::LDI_1: execute_ldi1(); break;
        case FSMState::LDI_2: execute_ldi2(); break;
        case FSMState::LDI_3: execute_ldi3(); break;
        case FSMState::LDI_4: execute_ldi4(); break;

        case FSMState::ST_0: execute_st0(); break;
        case FSMState::ST_1: execute_st1(); break;
        case FSMState::ST_2: execute_st2(); break;

        case FSMState::STR_0: execute_str0(); break;
        case FSMState::STR_1: execute_str1(); break;
        case FSMState::STR_2: execute_str2(); break;

        case FSMState::STI_0: execute_sti0(); break;
        case FSMState::STI_1: execute_sti1(); break;
        case FSMState::STI_2: execute_sti2(); break;
        case FSMState::STI_3: execute_sti3(); break;
        case FSMState::STI_4: execute_sti4(); break;

        case FSMState::JSR_0: execute_jsr0(); break;
        case FSMState::JSR_1: execute_jsr1(); break;
        case FSMState::JSR_2: execute_jsr2(); break;

        case FSMState::JMP_0: execute_jmp0(); break;

        case FSMState::BR_0: execute_br0(); break;
        case FSMState::BR_1: execute_br1(); break;  

        case FSMState::HALT:
            state.running = false;
            state.halted = true;
            break;

        case FSMState::ERROR:
        case FSMState::RESERVED:
            state.running = false;
            break;
    }
    state.cycles++;
}

void CPU::run() {
    while (state.running) {
        step();
    }
}

void CPUState::update_cond(uint16_t result){
    n = (result & 0x8000) != 0; 
    z = (result == 0);
    p = !n && !z;
}

uint16_t CPU::sign_extend(uint16_t val, int bits){
    if (val & (1 << (bits - 1))) {
        uint16_t mask = 0xFFFF << bits;
        return val | mask;
    }
    return val;
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
    state.MDR = memory.read(state.MAR);
    transition_to(FSMState::FETCH_2);
}

void CPU::execute_fetch2(){
    state.IR = state.MDR;
    transition_to(FSMState::DECODE);
}

void CPU::execute_decode(){
    uint16_t decode_temp = (state.IR >> 12);
    decode_temp = decode_temp & 0x000F; /* Bitmask to clear all bits to 0 except for the rightmost 4 bits */

    switch(decode_temp)
    {
        case 0x0000: transition_to(FSMState::BR_0); break;
        case 0x0001: transition_to(FSMState::ADD_0); break;
        case 0x0002: transition_to(FSMState::LD_0); break;
        case 0x0003: transition_to(FSMState::ST_0); break;
        case 0x0004: transition_to(FSMState::JSR_0); break;
        case 0x0005: transition_to(FSMState::AND_0); break;
        case 0x0006: transition_to(FSMState::LDR_0); break;
        case 0x0007: transition_to(FSMState::STR_0); break;
        case 0x0008: transition_to(FSMState::ERROR); break;
        case 0x0009: transition_to(FSMState::NOT_0); break;
        case 0x000A: transition_to(FSMState::LDI_0); break;
        case 0x000B: transition_to(FSMState::STI_0); break;
        case 0x000C: transition_to(FSMState::JMP_0); break;
        case 0x000D: transition_to(FSMState::RESERVED); break;
        case 0x000E: transition_to(FSMState::LEA_0); break;
        case 0x000F: transition_to(FSMState::TRAP_0); break;
        default: transition_to(FSMState::ERROR); break;    /* Perform operation based on opcode, IR[15:12] */
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
    state.MDR = memory.read(state.MAR);  /* MDR <- M[MAR] */
    state.reg[7] = state.pc;    /* R7 <- PC */
    transition_to(FSMState::TRAP_2);
}

void CPU::execute_trap2(){
    state.pc = state.MDR;
    if((state.IR & 0x00FF) == 0x0025){
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
    state.MDR = memory.read(state.MAR); /* MDR <- M[MAR] */
    transition_to(FSMState::LD_2);  
}

void CPU::execute_ld2(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007; /* keep 3 bits of DR */
    state.reg[dr] = state.MDR; /* DR <- MDR */
    state.update_cond(state.reg[dr]);
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_ldr0(){
    uint16_t baseR = state.IR >> 6;
    baseR = baseR & 0x0007; /* find 3 bits of baseR */
    uint16_t off6 = state.IR & 0x003F; /* bitmask only off6 bits */
    off6 = sign_extend(off6,6);
    state.MAR = state.reg[baseR] + off6; /* MAR <- BaseR + off6 */
    transition_to(FSMState::LDR_1);
}

void CPU::execute_ldr1(){
    state.MDR = memory.read(state.MAR); /* MDR <- M[MAR] */
    transition_to(FSMState::LDR_2);  
}

void CPU::execute_ldr2(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007; /* keep 3 bits of DR */
    state.reg[dr] = state.MDR; /* DR <- MDR */
    state.update_cond(state.reg[dr]);
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_ldi0(){
    uint16_t off9 = state.IR & 0x01FF; /* Keep off9 */
    off9 = sign_extend(off9,9);
    state.MAR = state.pc + off9;
    transition_to(FSMState::LDI_1);
}

void CPU::execute_ldi1(){
    state.MDR = memory.read(state.MAR); /* MDR <- M[MAR] */
    transition_to(FSMState::LDI_2);  
}

void CPU::execute_ldi2(){
    state.MAR = state.MDR;
    transition_to(FSMState::LDI_3);
}

void CPU::execute_ldi3(){
    state.MDR = memory.read(state.MAR); /* MDR <- M[MAR] */
    transition_to(FSMState::LDI_4);  
}

void CPU::execute_ldi4(){
    uint16_t dr = state.IR >> 9;
    dr = dr & 0x0007; /* keep 3 bits of DR */
    state.reg[dr] = state.MDR; /* DR <- MDR */
    state.update_cond(state.reg[dr]);
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_sti0(){
    uint16_t off9 = state.IR & 0x01FF; /* Keep off9 */
    off9 = sign_extend(off9,9);
    state.MAR = state.pc + off9;
    transition_to(FSMState::STI_1);
}

void CPU::execute_sti1(){
    state.MDR = memory.read(state.MAR); /* MDR <- M[MAR] */
    transition_to(FSMState::STI_2);  
}

void CPU::execute_sti2(){
    state.MAR = state.MDR;  /* MAR <- MDR */
    transition_to(FSMState::STI_3);
}

void CPU::execute_sti3(){
    uint16_t sr = state.IR >> 9;
    sr = sr & 0x0007; /* keep 3 bits of SR */
    state.MDR = state.reg[sr];
    transition_to(FSMState::STI_4);
}

void CPU::execute_sti4(){
    memory.write(state.MAR,state.MDR); /* M[MAR] <- MDR */
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_st0() {
    uint16_t off9 = state.IR & 0x01FF;  // Keep off9
    off9 = sign_extend(off9, 9);
    state.MAR = state.pc + off9;
    transition_to(FSMState::ST_1);
}

void CPU::execute_st1() {
    uint16_t sr = state.IR >> 9;
    sr = sr & 0x0007;  // Keep 3 bits of SR
    state.MDR = state.reg[sr];
    transition_to(FSMState::ST_2);
}

void CPU::execute_st2() {
    memory.write(state.MAR,state.MDR);  // M[MAR] <- MDR
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_str0() {
    uint16_t baseR = state.IR >> 6;
    baseR = baseR & 0x0007;  // Find 3 bits of baseR
    uint16_t off6 = state.IR & 0x003F;  // Bitmask only off6 bits
    off6 = sign_extend(off6, 6);
    state.MAR = state.reg[baseR] + off6;  // MAR <- BaseR + off6
    transition_to(FSMState::STR_1);
}

void CPU::execute_str1() {
    uint16_t sr = state.IR >> 9;
    sr = sr & 0x0007;  // Keep 3 bits of SR
    state.MDR = state.reg[sr];
    transition_to(FSMState::STR_2);
}

void CPU::execute_str2() {
    memory.write(state.MAR,state.MDR);  // M[MAR] <- MDR
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_jsr0(){
    state.reg[7] = state.pc;    /* R7 <- PC */
    uint16_t IR11 = state.IR >> 11;
    IR11 = IR11 & 0x0001; /* only keep the IR[11] bit */
    if(IR11 == 0){
        transition_to(FSMState::JSR_1);
    }
    else{
        transition_to(FSMState::JSR_2);
    }
}

void CPU::execute_jsr1(){
    uint16_t baseR = state.IR >> 6;
    baseR = baseR & 0x0007; /* Mask the baseR bits */
    state.pc = state.reg[baseR];    /* PC <- BaseR*/
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_jsr2(){
    uint16_t off11 = state.IR & 0x07FF; /* save the 11 bits at the end */
    off11 = sign_extend(off11,11);
    state.pc += off11;
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_jmp0(){
    uint16_t baseR = state.IR >> 6;
    baseR = baseR & 0x0007; /* Mask the baseR bits */
    state.pc = state.reg[baseR];
    transition_to(FSMState::FETCH_0);
}

void CPU::execute_br0(){
    uint16_t n_cond = state.IR >> 11;
    n_cond = n_cond & 0x0001; /* only keep IR[11], keeping the n flag */
    uint16_t z_cond = state.IR >> 10;
    z_cond = z_cond & 0x0001; /* only keep IR[10], keeping the z flag */
    uint16_t p_cond = state.IR >> 9;
    p_cond = p_cond & 0x0001; /* only keep IR[9], keeping the p flag*/
    bool BEN = ((n_cond && state.n) || (z_cond && state.z) || (p_cond && state.p)); /* if true, [BEN] activates, branch to desired address */
    if(BEN){
        transition_to(FSMState::BR_1);
    }
    else{
    transition_to(FSMState::FETCH_0);
    }
}

void CPU::execute_br1(){
    uint16_t off9 = state.IR & 0x01FF; /* Bitmask the last 9 bits */
    off9 = sign_extend(off9,9);
    state.pc += off9; /* PC <- PC + off9 */
    transition_to(FSMState::FETCH_0);
}

CPU::CPU() : temp_addr(0), branch_taken(false){}
