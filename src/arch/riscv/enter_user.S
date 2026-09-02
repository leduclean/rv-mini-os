// This file is used as a stub entry to the user mode.
.text
.globl enter_user

// void enter_user(pt_regs_t *regs);
enter_user:
    // Fake move the stack pointer to the 
    // stub structure to reset it state
    mv sp, a0
    j trap_return

    
