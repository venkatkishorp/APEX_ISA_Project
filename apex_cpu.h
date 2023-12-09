/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int ps1;
    int ps2;
    int rd;
    int pd;
    int imm;
    int rs1_value;
    int rs2_value;
    int ps1_value;
    int ps2_value;
    int result_buffer;
    int memory_address;
    int has_insn;
} CPU_Stage;

typedef struct CPU_PRF_Dependency
{
    int isValid;        // if 1, this dependency is valid; if 0, this dependency is not valid
    int dep_type;       // If 1, it is an entry in IQ; if 0, it is an entry in LSQ
    int index;          // Index of the entry
} CPU_PRF_Dependency;

typedef struct CPU_PRF
{
    int isValid;
    int value;
    int dependency_count;
    CPU_PRF_Dependency dependency_list[IQ_SIZE];
    int broadcast_valid;
    int broadcast_value;
} CPU_PRF;

typedef struct CPU_IQ
{
    int isValid;
    int FU;             // if 1, the function unit is IntFU; if 2, the function unit is MulFU; if 3, the function unit is AFU
    int function_type;  // if 1, the fucntion type is addition; if 2, the function type is subraction => Applies only to IntFU
    int literal;
    int ps1_valid;
    int ps1_tag;
    int ps2_valid;
    int ps2_tag;
    int dest_type;      // if 0, destination is PRF; if 1, destination is LSQ
    int dest;
} CPU_IQ;

typedef struct CPU_LSQ
{
    int isValid;
    int lORs;           // if 0, it is store; if 1, it is load
    int mem_valid;      // if 0, memory hasn't been calculated and updated yet; if 1, memory address is calculated and is present in the memory field
    int memory;
    int dest;
    int ps1_valid;
    int ps1_tag;
} CPU_LSQ;

typedef struct CPU_ROB
{
    int isValid;
    int insn_type;      // if 1, it is load; if 2, it is store; if 3, it is Reg-Reg
    int pc;
    int pd;
    int overwritten_pd;
    int rd;
    int lsq_index;
    int mem_error_codes;
} CPU_ROB;

typedef struct CPU_Godzilla
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int ps1;
    int ps2;
    int rd;
    int pd;
    int overwritten_pd;
    int imm;
    int rs1_value;
    int rs2_value;
    int ps1_value;
    int ps2_value;
    int ps1_valid;
    int ps2_valid;
    int result_buffer;
    int memory_address;
    int has_insn;
    int enter_godzilla; // This specifies if dispatch should happen or not
    int lsq_index;      // This is the index allocated for the load/store instruction arrived at the Godzilla at the current cycle
} CPU_Godzilla;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int cycles_limit;              /* Sets the maximum number of cycles the CPU is initialized to run for */
    int enable_forwarding;         /* Sets the user choice of using forwarding */
    int insn_completed;            /* Instructions retired */
    int has_stalled;               /* Indicates whether instruction has been stalled */
    int regs[REG_FILE_SIZE];       /* Integer register file */
    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int positive_flag;              /* {TRUE, FALSE} Used by BP and BNP to branch */
    int negative_flag;             /* {TRUE, FALSE} Used by BN and BNN to branch */
    int fetch_from_next_cycle;
    int decode_from_next_cycle;
    int dispatch_from_next_cycle;
    int overwritten_pd;

    int sim_n;

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode_rename;
    CPU_Stage rename_dispatch;
    CPU_Godzilla godzilla;
    CPU_Stage execute;
    
    CPU_IQ cpu_iq[IQ_SIZE];
    CPU_LSQ cpu_lsq[LSQ_SIZE];
    CPU_ROB cpu_rob[ROB_SIZE];
    CPU_PRF cpu_prf[PRF_SIZE];

    int rename_table[REG_FILE_SIZE];    /* Index: Regs || Value: Physical Regs */
    int free_reg_list[PRF_SIZE];
    int free_reg_head;
    int free_reg_tail;

    /* entry index of BTB */
    int btb_insert_at;

} APEX_CPU;




APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
void ns_print_stage_content(const char *name, const CPU_Stage *stage);
void ns_print_reg_file(const APEX_CPU *cpu);
#endif