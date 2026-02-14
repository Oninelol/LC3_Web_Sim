#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <string>

/* Add Enum for FSM States of LC3 for the FSM RTL to be Visualized on GUI */
enum class FSMState {
    FETCH_0,    /* MAR <- PC, PC <- PC+1 */
    FETCH_1,    /* MDR <- M[MAR] */
    FETCH_2,    /* IR <- MDR */

    DECODE,     /* Decodes OPCODE */

    ADD_0,
    AND_0,
    NOT_0,  /* Artihmetic & Logical Operation */

    TRAP_0,
    TRAP_1,
    TRAP_2, /* Trap Vector */

    LEA_0, /* One state of LEA */

    LD_0,
    LD_1,
    LD_2,    /* 3 states of LD */

    LDR_0,
    LDR_1,
    LDR_2,  /* 3 states of LDR */

    LDI_0,
    LDI_1,
    LDI_2,
    LDI_3,
    LDI_4,  /* 5 states of LDI */

    STI_0,
    STI_1,
    STI_2,
    STI_3,
    STI_4,  /* 5 states of STI */

    STR_0,
    STR_1,
    STR_2,  /* 3 states of STR */

    ST_0,
    ST_1,
    ST_2,   /* 3 states of ST */

    JSR_0,
    JSR_1,  /* 2 states of JSR, second one depending on IR[11] */
    JSR_2,  /* JSR_1 is IR[11]=0, JSR_2 is IR[11]=1 */

    JMP_0,  /* 1 state of JMP */

    BR_0,
    BR_1,   /* 2 states of BR */

    HALT,   /* TRAP x25 */
    ERROR,  /* To 8 instruction at DECODE */
    RESERVED, /* x1101 reserved opcode */


};

/* Conversion of FSM State to string*/
inline std::string fsm_state_to_string(FSMState state){
    switch(state){
        case FSMState::FETCH_0: return "FETCH_0";
        case FSMState::FETCH_1: return "FETCH_1";
        case FSMState::FETCH_2: return "FETCH_2";
        case FSMState::DECODE: return "DECODE";
        case FSMState::ADD_0: return "ADD_0";
        case FSMState::AND_0: return "AND_0";
        case FSMState::NOT_0: return "NOT_0";
        case FSMState::TRAP_0: return "TRAP_0";
        case FSMState::TRAP_1: return "TRAP_1";
        case FSMState::TRAP_2: return "TRAP_2";
        case FSMState::LEA_0: return "LEA_0";
        case FSMState::LD_0: return "LD_0";
        case FSMState::LD_1: return "LD_1";
        case FSMState::LD_2: return "LD_2";
        case FSMState::LDR_0: return "LDR_0";
        case FSMState::LDR_1: return "LDR_1";
        case FSMState::LDR_2: return "LDR_2";
        case FSMState::LDI_0: return "LDI_0";
        case FSMState::LDI_1: return "LDI_1";
        case FSMState::LDI_2: return "LDI_2";
        case FSMState::LDI_3: return "LDI_3";
        case FSMState::LDI_4: return "LDI_4";
        case FSMState::STI_0: return "STI_0";
        case FSMState::STI_1: return "STI_1";
        case FSMState::STI_2: return "STI_2";
        case FSMState::STI_3: return "STI_3";
        case FSMState::STI_4: return "STI_4";
        case FSMState::STR_0: return "STR_0";
        case FSMState::STR_1: return "STR_1";
        case FSMState::STR_2: return "STR_2";
        case FSMState::ST_0: return "ST_0";
        case FSMState::ST_1: return "ST_1";
        case FSMState::ST_2: return "ST_2";
        case FSMState::JSR_0: return "JSR_0";
        case FSMState::JSR_1: return "JSR, IR11=0";
        case FSMState::JSR_2: return "JSR, IR[11]=1";
        case FSMState::JMP_0: return "JMP_0";
        case FSMState::BR_0: return "BR_0";
        case FSMState::BR_1: return "BR_1";
        case FSMState::HALT: return "HALT";
        case FSMState::ERROR: return "ERROR";
        case FSMState::RESERVED: return "RESERVED OPCODE";
        default: return "UNKNOWN";
    }
}

inline std::string fsm_state_description(FSMState state){
    switch(state){
        case FSMState::FETCH_0: return "MAR <- PC, PC <- PC+1";
        case FSMState::FETCH_1: return "MDR <- M[MAR]";
        case FSMState::FETCH_2: return "IR <- MDR";
        case FSMState::DECODE: return "Decode opcode and dispatch";
        case FSMState::ADD_0: return "Execute ADD, SetCC";
        case FSMState::AND_0: return "Execute AND, SetCC";
        case FSMState::NOT_0: return "Execute NOT, setCC";
        case FSMState::TRAP_0: return "MAR <- trapvect";
        case FSMState::TRAP_1: return "MDR <- M[MAR]";
        case FSMState::TRAP_2: return "PC <- MDR";
        case FSMState::LEA_0: return "DR <- PC+offset9, setCC";
        case FSMState::LD_0: return "MAR <- PC+offset9";
        case FSMState::LD_1: return "MDR <- M[MAR]";
        case FSMState::LD_2: return "DR <- MDR, setCC";
        case FSMState::LDR_0: return "MAR <- BaseR + offset6";
        case FSMState::LDR_1: return "MDR <- M[MAR]";
        case FSMState::LDR_2: return "DR <- MDR, setCC";
        case FSMState::LDI_0: return "MAR <- PC + offset9";
        case FSMState::LDI_1: return "MDR <- M[MAR]";
        case FSMState::LDI_2: return "MAR <- MDR";
        case FSMState::LDI_3: return "MDR <- M[MAR]";
        case FSMState::LDI_4: return "DR <- MDR, setCC";
        case FSMState::STI_0: return "MAR <- PC + offset9";
        case FSMState::STI_1: return "MDR <- M[MAR]";
        case FSMState::STI_2: return "MAR <- MDR";
        case FSMState::STI_3: return "MDR <- SR";
        case FSMState::STI_4: return "M[MAR] <- MDR";
        case FSMState::STR_0: return "MAR <- BaseR + offset6";
        case FSMState::STR_1: return "MDR <- SR";
        case FSMState::STR_2: return "M[MAR] <- MDR";
        case FSMState::ST_0: return "MAR <- PC + offset9";
        case FSMState::ST_1: return "MDR <- SR";
        case FSMState::ST_2: return "M[MAR] <- MDR";
        case FSMState::JSR_0: return "R7 <- PC (save return address), [IR[11]]";
        case FSMState::JSR_1: return "PC <- BaseR";
        case FSMState::JSR_2: return "PC <- PC + off11";
        case FSMState::JMP_0: return "PC <- BaseR";
        case FSMState::BR_0: return "[BEN]";
        case FSMState::BR_1: return "PC <- PC + offset9";
        case FSMState::HALT: return "Execution Halted";
        case FSMState::ERROR: return "Invalid Instruction";
        case FSMState::RESERVED: return "RESERVED OPCODE";
        default: return "Unknown Instruction";
    }
}


struct CPUState{
    std::array<uint16_t,8> reg;     /* Initialize R0-R7*/
    uint16_t pc;    /* Initialize Program Counter in CPU */
    bool n,z,p;     /* Condition codes for n,z,p */
    uint16_t IR;
    uint16_t MAR;
    uint16_t MDR;   /* intialize IR, MAR, MDR to be 16 bit integers */

    FSMState current_state; /* Current state of execution */
    FSMState previous_state; /* Previous state of FSM execution, for visualization purposes */
    
    bool running; 
    bool halted; /* Condition checks for whether if the program is still running, or is halted */
    uint64_t cycles; /* Total cycles executed, count */

    CPUState();     /* Constructor */
    void reset();   /* Reset state of LC3 CPU*/
    void update_cond(uint16_t result);
    void print() const;
};
