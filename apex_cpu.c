/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;   
        }
        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LOADP:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_JALR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STORE:
        case OPCODE_STOREP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BNN:
        case OPCODE_BN:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_CML:
        {
            printf("%s,R%d,#%d", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    printf("Z: %-3d P: %-3d N: %-3d ", cpu->zero_flag, cpu->positive_flag, cpu->negative_flag);

    printf("\n\n");

    for(i = 0; i < DATA_MEMORY_SIZE; i++)
    {
        if(cpu->data_memory[i] != 0)
        {
            printf("MEM[%-3d]: %-3d", i, cpu->data_memory[i]);
        }
    }

    printf("\n\n");

    // printf("BTB: ");
    // for(i = 0; i < BTB_SIZE; i++)
    // {
    //     if(cpu->BTB_Entries[i].ins_addr != 0)
    //     {
    //         printf("pc(%-4d) ", cpu->BTB_Entries[i].ins_addr);
    //     }
    // }

    printf("\n");
}


/*
Akash, edit this function according to your requirement
*/
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        if(cpu->decode_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = TRUE;
        }
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;
            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("Fetch", &cpu->fetch);
            }
            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        cpu->pc += 4;

        if(cpu->decode_rename.has_insn == FALSE)
        {
            /* Copy data from fetch latch to decode latch*/
            cpu->decode_rename = cpu->fetch;
        }
        else
        {
            cpu->fetch_from_next_cycle = TRUE;            
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
}

static void
APEX_Decode(APEX_CPU *cpu)
{
    if(cpu->decode_rename.has_insn)
    {
        if(cpu->dispatch_from_next_cycle == TRUE)
        {
            cpu->decode_from_next_cycle = TRUE;
        }
        if (cpu->decode_from_next_cycle == TRUE)
        {
            cpu->decode_from_next_cycle = FALSE;
            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("Fetch", &cpu->decode_rename);
            }
            /* Skip this cycle*/
            return;
        }

        // rd renaming

        switch (cpu->decode_rename.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_ADDL:
            case OPCODE_SUB:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_LOAD:
            case OPCODE_MOVC:
            case OPCODE_JALR:
            case OPCODE_LOADP:
            {
                cpu->overwritten_pd = cpu->rename_table[cpu->decode_rename.rd];
                cpu->decode_rename.pd = cpu->free_reg_list[cpu->free_reg_head];
                cpu->rename_table[cpu->decode_rename.rd] = cpu->decode_rename.pd;
                cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                break;
            }
        }

        // rs1 & rs2 renaming
        switch (cpu->decode_rename.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_STORE:
            case OPCODE_STOREP:
            case OPCODE_CMP:
            {
                if(cpu->rename_table[cpu->decode_rename.rs1] == -1)
                {
                    cpu->decode_rename.ps1 = cpu->free_reg_list[cpu->free_reg_head];
                    cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                    cpu->rename_table[cpu->decode_rename.rs1] = cpu->decode_rename.ps1;
                    
                }
                
                if(cpu->rename_table[cpu->decode_rename.rs2] == -1)
                {
                    cpu->decode_rename.ps2 = cpu->free_reg_list[cpu->free_reg_head];
                    cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                    cpu->rename_table[cpu->decode_rename.rs2] = cpu->decode_rename.ps2;
                }
                cpu->cpu_prf[cpu->decode_rename.rs2].dependency_count += 1;
                break;
            }

            case OPCODE_LOAD:
            case OPCODE_LOADP:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_JALR:
            case OPCODE_JUMP:
            case OPCODE_CML:
            {
                if(cpu->rename_table[cpu->decode_rename.rs1] == -1)
                {
                    cpu->decode_rename.ps1 = cpu->free_reg_list[cpu->free_reg_head];
                    cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                    cpu->rename_table[cpu->decode_rename.rs1] = cpu->decode_rename.ps1;
                }
                break;
            }

            case OPCODE_MOVC:
            case OPCODE_NOP:
            {
                break;
            }

            case OPCODE_HALT:
            {
                cpu->fetch.has_insn = FALSE;
                break;
            }

        
            default:
                break;
        }

        cpu->rename_dispatch = cpu->decode_rename;
        cpu->rename_dispatch.has_insn = TRUE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode_rename);
        }        
        cpu->decode_rename.has_insn = FALSE;
        
    }
}

static int
APEX_Dispatch(APEX_CPU *cpu)
{
    if(cpu->rename_dispatch.has_insn)
    {
        if(cpu->godzilla.enter_godzilla == FALSE)
        {
            cpu->dispatch_from_next_cycle = TRUE;
        }
        if(cpu->dispatch_from_next_cycle == TRUE)
        {
            cpu->dispatch_from_next_cycle = FALSE;
            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("Fetch", &cpu->rename_dispatch);
            }
            /* Skip this cycle*/
            return FALSE;
        }

        cpu->godzilla.imm = cpu->rename_dispatch.imm;
        cpu->godzilla.opcode = cpu->rename_dispatch.opcode;
        strcpy(cpu->godzilla.opcode_str, cpu->rename_dispatch.opcode_str);
        cpu->godzilla.overwritten_pd = cpu->overwritten_pd;
        cpu->godzilla.pc = cpu->decode_rename.pc;
        cpu->godzilla.pd = cpu->decode_rename.pd;
        cpu->godzilla.ps1 = cpu->decode_rename.ps1;
        cpu->godzilla.ps2 = cpu->godzilla.ps2;
        cpu->godzilla.rd = cpu->decode_rename.rd;
        cpu->godzilla.rs1 = cpu->decode_rename.rs1;
        cpu->godzilla.rs2 = cpu->decode_rename.rs2;

        // if(cpu->rename_dispatch.opcode == OPCODE_HALT)
        // {
        //     return TRUE;
        // }
    }

    return FALSE;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    for (int i = 0; i < PRF_SIZE; i++)
    {
        cpu->cpu_prf[i].isValid = 1;
        cpu->cpu_prf[i].dependency_count = 0;
        for (int j = 0; j < IQ_SIZE; j++)
        {
            cpu->cpu_prf[i].dependency_list[j].isValid = 0;
        }
        cpu->cpu_prf[i].value = -1;
    }

    cpu->free_reg_head = 0;
    cpu->free_reg_tail = 0;

    for (int i = 0; i < IQ_SIZE; i++) {
        cpu->cpu_iq[i].isValid = 0;
    }

    for (int i = 0; i < LSQ_SIZE; i++) {
        cpu->cpu_lsq[i].isValid = 0;
    }

    for (int i = 0; i < ROB_SIZE; i++) {
        cpu->cpu_rob[i].isValid = 0;
    }

    for (int i = 0; i < REG_FILE_SIZE; i++) {
        cpu->rename_table[i] = i;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;
    // int memory_address = 0;

    while (TRUE)
    {
        if (cpu->clock >= cpu->cycles_limit)
        {
            printf("\nYou've exhausted all the clock cycles!\n");
            break;
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }

        if (APEX_Dispatch(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            print_reg_file(cpu);
            break;
        }
        APEX_Decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }
        else
        {
            if(cpu->clock == cpu->sim_n - 1)
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}

void ns_print_stage_content(const char *name, const CPU_Stage *stage)
{
    print_stage_content(name, stage);
}

void ns_print_reg_file(const APEX_CPU *cpu)
{
    print_reg_file(cpu);
}