#include "cpu_state.h"    /* Include the previously defined cpu structure into this file*/
#include <iostream>
CPUState::CPUState()
{
    reset();
}

void CPUState::reset() {
    reg.fill(0);
    pc = 0x3000;
    n = false;
    z = true;
    p = false;  /* Reset nzp to 010, pc to x3000, and reset all registers to 0 */
}

void CPUState::update_flags(uint16_t result) {  /* Updates the control values nzp in the CPU */

    if(result==0)
    {  
        n = false;
        z = true;
        p = false;
    }
    else if(result & 0x8000)
    {
        n = true;
        z = false;
        p = false;
    }
    else 
    {
        n = false;
        z = false;
        p = true;
    }
}

void CPUState::print() const{
    std::cout << "PC: 0x" << std::hex << pc << std::endl;    /* Display the PC value to the screen */
    for(int i=0;i<8;i++)
    {
        std::cout << "R" << i << ": " << std::hex << reg[i] << std::endl;     /* Display R1 to R7 on the screen */
    }

    std::cout << "Flags: " << std::endl;
    std::cout << "N=" << n << std::endl;
    std::cout << "Z=" << z << std::endl;
    std::cout << "P=" << p << std::endl;
    std::cout << std::endl;

}