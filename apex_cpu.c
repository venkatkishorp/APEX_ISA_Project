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

/*
This method is used to print the contents of the godzilla stage - the IQ, ROB and LSQ
*/
void print_godzilla (APEX_CPU *cpu) {
    printf("\nContents of godzilla are:\npd: %d, ps1: %d, ps2: %d, imm: %d, rd: %d, opcode: %d\n\n", cpu->godzilla.pd, cpu->godzilla.ps1, cpu->godzilla.ps2, cpu->godzilla.imm, cpu->godzilla.rd, cpu->godzilla.opcode);

    printf("\nPrinting the IQ:\n");
    printf("\nisValid  |  FU       |  literal  |  ps1-valid|  ps1-tag  |  ps2-valid|  ps2-tag  |  dest-type|  dest     |\n");
    for (int i = 0; i < IQ_SIZE; i++) {
        printf("%-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|\n", cpu->cpu_iq[i].isValid, cpu->cpu_iq[i].FU, cpu->cpu_iq[i].literal, cpu->cpu_iq[i].ps1_valid, cpu->cpu_iq[i].ps1_tag, cpu->cpu_iq[i].ps2_valid, cpu->cpu_iq[i].ps2_tag, cpu->cpu_iq[i].dest_type, cpu->cpu_iq[i].dest);
    }

    printf("\nPrinting the ROB:\n");
    printf("\n    |  isValid  |  insn_type|  pc       |  pd       |  ovwrtn_pd|  rd       |  lsq_index |\n");
    for (int i = 0; i < ROB_SIZE; i++) {
        printf("%-4c|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|\n", i == cpu->rob_head ? '--' : ' ', cpu->cpu_rob[i].isValid, cpu->cpu_rob[i].insn_type, cpu->cpu_rob[i].pc, cpu->cpu_rob[i].pd, cpu->cpu_rob[i].overwritten_pd, cpu->cpu_rob[i].rd, cpu->cpu_rob[i].lsq_index);
    }

    printf("\nPrinting the LSQ:\n");
    printf("\nisValid  |  ld/store |  mem_valid|  memory   |  dest     |  ps1_valid|  ps1_tag  |  pd       |\n");
    for (int i = 0; i < LSQ_SIZE; i++) {
        printf("%-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|  %-9d|\n", cpu->cpu_lsq[i].isValid, cpu->cpu_lsq[i].lORs, cpu->cpu_lsq[i].mem_valid, cpu->cpu_lsq[i].memory, cpu->cpu_lsq[i].dest, cpu->cpu_lsq[i].ps1_valid, cpu->cpu_lsq[i].ps1_tag, cpu->cpu_lsq[i].pd);
    }
}

/*
This method is used to print the stage contents of the execute stage
*/
void print_execute (APEX_CPU *cpu) {
    printf("\nPrinting the contents of INTFU\n");
    printf("\nhas_insn = %d, pd = %d, ps1 = %d, ps2 = %d, imm = %d\n", cpu->execute.intFU.has_insn, cpu->execute.intFU.pd, cpu->execute.intFU.ps1, cpu->execute.intFU.ps2, cpu->execute.intFU.imm);

    printf("\nPrinting the contents of ADDFU\n");
    printf("\nhas_insn = %d, pd = %d, ps1 = %d, imm = %d\n", cpu->execute.intFU.has_insn, cpu->execute.addFU.pd, cpu->execute.addFU.ps1, cpu->execute.addFU.imm);

    printf("\nPrinting the contents of MULFU\n");
    printf("\nhas_insn = %d, pd = %d, ps1 = %d, ps2 = %d\n", cpu->execute.intFU.has_insn, cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
}

/*
This method is used to print the PRF
*/
void print_prf (APEX_CPU *cpu) {
    printf("\nPrinting the PRF\n");
    printf("\nReg      |  isValid  |  Value    \n");
    for (int i = 0; i < PRF_SIZE; i++) {
        printf("%-9d|  %-9d|  %-9d|\n", i, cpu->cpu_prf[i].isValid, cpu->cpu_prf[i].value);
    }
    printf("\n\n");
}

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

/*
This method is used to print the physical register file
*/
void print_phy_reg_file (APEX_CPU *cpu) {
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < PRF_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->cpu_prf[i].value);
    }

    printf("\n");

    for (i = (PRF_SIZE / 2); i < PRF_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->cpu_prf[i].value);
    }

    printf("\n");

    // printf("Z: %-3d P: %-3d N: %-3d ", cpu->zero_flag, cpu->positive_flag, cpu->negative_flag);

    printf("\n\n");
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
                // print_stage_content("Fetch", &cpu->fetch);
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
                // print_stage_content("Fetch", &cpu->decode_rename);
            }
            /* Skip this cycle*/
            return;
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
                else {
                    cpu->decode_rename.ps1 = cpu->rename_table[cpu->decode_rename.rs1];
                }
                
                if(cpu->rename_table[cpu->decode_rename.rs2] == -1)
                {
                    cpu->decode_rename.ps2 = cpu->free_reg_list[cpu->free_reg_head];
                    cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                    cpu->rename_table[cpu->decode_rename.rs2] = cpu->decode_rename.ps2;
                }
                else {
                    cpu->decode_rename.ps2 = cpu->rename_table[cpu->decode_rename.rs2];
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
                else {
                    cpu->decode_rename.ps1 = cpu->rename_table[cpu->decode_rename.rs1];
                }
                break;
            }

            case OPCODE_MOVC:
            case OPCODE_NOP:
            {
                cpu->decode_rename.ps1 = -2;
                cpu->decode_rename.ps2 = -2;
                break;
            }

            case OPCODE_HALT:
            {
                cpu->decode_rename.ps1 = -2;
                cpu->decode_rename.ps2 = -2;
                cpu->fetch.has_insn = FALSE;
                break;
            }

        
            default:
                break;
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
            {
                cpu->overwritten_pd = cpu->rename_table[cpu->decode_rename.rd];
                cpu->decode_rename.pd = cpu->free_reg_list[cpu->free_reg_head];
                printf("\n%d is being overwritten to %d\n", cpu->rename_table[cpu->decode_rename.rd], cpu->decode_rename.pd);
                cpu->rename_table[cpu->decode_rename.rd] = cpu->decode_rename.pd;
                cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                cpu->cpu_prf[cpu->decode_rename.pd].isValid = FALSE;

                printf("\nRename Table: \n");
                for (int i = 0; i < REG_FILE_SIZE; i++) {
                    printf("%d => %d\n", i, cpu->rename_table[i]);
                }
                printf("\n\n");
                printf("\nFree list of registers: \n");
                for (int i = cpu->free_reg_head + 1; i != cpu->free_reg_tail; i = (i + 1)%PRF_SIZE) {
                    printf("%d\t", cpu->free_reg_list[i-1]);
                }
                printf("\n\n");

                break;
            }

            case OPCODE_LOADP:
            {
                cpu->overwritten_pd = cpu->rename_table[cpu->decode_rename.rd];
                cpu->decode_rename.pd = cpu->free_reg_list[cpu->free_reg_head];
                cpu->rename_table[cpu->decode_rename.rd] = cpu->decode_rename.pd;
                cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                cpu->cpu_prf[cpu->decode_rename.pd].isValid = FALSE;

                cpu->decode_rename.lpsp_inc_dest = cpu->free_reg_list[cpu->free_reg_head];
                cpu->rename_table[cpu->decode_rename.rs1] = cpu->decode_rename.lpsp_inc_dest;
                cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                cpu->cpu_prf[cpu->decode_rename.lpsp_inc_dest].isValid = FALSE;

                printf("\nRename Table: \n");
                for (int i = 0; i < REG_FILE_SIZE; i++) {
                    printf("%d => %d\n", i, cpu->rename_table[i]);
                }
                printf("\n\n");
                printf("\nFree list of registers: \n");
                for (int i = cpu->free_reg_head + 1; i != cpu->free_reg_tail; i = (i + 1)%PRF_SIZE) {
                    printf("%d\t", cpu->free_reg_list[i-1]);
                }
                printf("\n\n");

                break;
            }

            case OPCODE_STOREP:
            {
                cpu->decode_rename.lpsp_inc_dest = cpu->free_reg_list[cpu->free_reg_head];
                cpu->rename_table[cpu->decode_rename.rs2] = cpu->decode_rename.lpsp_inc_dest;
                cpu->free_reg_head = (cpu->free_reg_head + 1) % PRF_SIZE;
                cpu->cpu_prf[cpu->decode_rename.lpsp_inc_dest].isValid = FALSE;

                printf("\nlpsp_inc_dest = %d, %d\n", cpu->decode_rename.lpsp_inc_dest);

                break;
            }
        }

        cpu->rename_dispatch = cpu->decode_rename;
        cpu->rename_dispatch.has_insn = TRUE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode1", &cpu->decode_rename);
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
                // print_stage_content("Fetch", &cpu->rename_dispatch);
            }
            /* Skip this cycle*/
            return FALSE;
        }

        cpu->godzilla.imm = cpu->rename_dispatch.imm;
        cpu->godzilla.opcode = cpu->rename_dispatch.opcode;
        strcpy(cpu->godzilla.opcode_str, cpu->rename_dispatch.opcode_str);
        cpu->godzilla.overwritten_pd = cpu->overwritten_pd;
        cpu->godzilla.pc = cpu->rename_dispatch.pc;
        cpu->godzilla.pd = cpu->rename_dispatch.pd;
        cpu->godzilla.ps1 = cpu->rename_dispatch.ps1;
        cpu->godzilla.ps2 = cpu->rename_dispatch.ps2;
        cpu->godzilla.rd = cpu->rename_dispatch.rd;
        cpu->godzilla.rs1 = cpu->rename_dispatch.rs1;
        cpu->godzilla.rs2 = cpu->rename_dispatch.rs2;
        cpu->godzilla.has_insn = TRUE;

        switch (cpu->rename_dispatch.opcode) {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_STORE:
            case OPCODE_CMP:
            {
                if (cpu->cpu_prf[cpu->rename_dispatch.ps1].isValid || 
                    cpu->rename_dispatch.ps1 == cpu->intFU_broadcasted_tag || 
                    cpu->rename_dispatch.ps1 == cpu->mulFU_broadcasted_tag) {
                    cpu->godzilla.ps1_valid = TRUE;
                }
                else {
                    cpu->godzilla.ps1_valid = FALSE;
                }

                if (cpu->cpu_prf[cpu->rename_dispatch.ps2].isValid || 
                    cpu->rename_dispatch.ps2 == cpu->intFU_broadcasted_tag || 
                    cpu->rename_dispatch.ps2 == cpu->mulFU_broadcasted_tag) {
                    cpu->godzilla.ps2_valid = TRUE;
                }
                else {
                    cpu->godzilla.ps2_valid = FALSE;
                }

                break;
            }

            case OPCODE_STOREP:
            {
                // printf("\nSTOREP: ps1-valid: %d, broadcast = %d\n", cpu->cpu_prf[cpu->rename_dispatch.ps1].isValid, cpu->intFU_broadcasted_tag);
                if (cpu->cpu_prf[cpu->rename_dispatch.ps1].isValid || 
                    cpu->rename_dispatch.ps1 == cpu->intFU_broadcasted_tag || 
                    cpu->rename_dispatch.ps1 == cpu->mulFU_broadcasted_tag) {
                    cpu->godzilla.ps1_valid = TRUE;
                }
                else {
                    cpu->godzilla.ps1_valid = FALSE;
                }

                if (cpu->cpu_prf[cpu->rename_dispatch.ps2].isValid || 
                    cpu->rename_dispatch.ps2 == cpu->intFU_broadcasted_tag || 
                    cpu->rename_dispatch.ps2 == cpu->mulFU_broadcasted_tag) {
                    cpu->godzilla.ps2_valid = TRUE;
                }
                else {
                    cpu->godzilla.ps2_valid = FALSE;
                }

                printf("\nlpsp_inc_dest in rename_dispatch = %d\n", cpu->rename_dispatch.lpsp_inc_dest);
                cpu->godzilla.lpsp_inc_dest = cpu->rename_dispatch.lpsp_inc_dest;

                break;
            }

            case OPCODE_LOAD:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_CML:
            {
                if (cpu->cpu_prf[cpu->rename_dispatch.ps1].isValid || 
                    cpu->rename_dispatch.ps1 == cpu->intFU_broadcasted_tag || 
                    cpu->rename_dispatch.ps1 == cpu->mulFU_broadcasted_tag) {
                    cpu->godzilla.ps1_valid = TRUE;
                }
                else {
                    cpu->godzilla.ps1_valid = FALSE;
                }

                cpu->godzilla.ps2_valid = TRUE;

                break;
            }

            case OPCODE_LOADP:
            {
                if (cpu->cpu_prf[cpu->rename_dispatch.ps1].isValid || 
                    cpu->rename_dispatch.ps1 == cpu->intFU_broadcasted_tag || 
                    cpu->rename_dispatch.ps1 == cpu->mulFU_broadcasted_tag) {
                    cpu->godzilla.ps1_valid = TRUE;
                }
                else {
                    cpu->godzilla.ps1_valid = FALSE;
                }

                cpu->godzilla.ps2_valid = TRUE;

                cpu->godzilla.lpsp_inc_dest = cpu->rename_dispatch.lpsp_inc_dest;

                break;
            }

            case OPCODE_MOVC:
            {
                cpu->godzilla.ps1_valid = TRUE;
                cpu->godzilla.ps2_valid = TRUE;

                break;
            }
        }

        if(cpu->rename_dispatch.opcode == OPCODE_HALT)
        {
            cpu->godzilla.ps1_valid = TRUE;
            cpu->godzilla.ps2_valid = TRUE;

            cpu->rename_dispatch.has_insn = FALSE;
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode2", &cpu->rename_dispatch);
        }
    }

    return FALSE;
}

#pragma region - Godzilla Stage 

/*
This method sets the flag enter_godzilla to denote if dispatch should stall
*/
void should_dispatch_stall (APEX_CPU *cpu) {
    int entry_available_in_iq = 0;
    int entry_available_in_rob = 0;
    int entry_available_in_lsq = 0;
    int entry_available_in_bq = 0;
    int entry_available_in_bis = 0;

    for (int i = 0; i < IQ_SIZE; i++) {
        if (cpu->cpu_iq[i].isValid != TRUE) {
            entry_available_in_iq = 1;
            break;
        }
    }

    if (entry_available_in_iq == 0) {
        cpu->godzilla.enter_godzilla = FALSE;
        return;
    }

    if (cpu->rob_head == cpu->rob_tail && cpu->cpu_rob[cpu->rob_head].isValid) {
        cpu->godzilla.enter_godzilla = FALSE;
        return;
    }

    // for (int i = 0; i < ROB_SIZE; i++) {
    //     if (cpu->cpu_rob[i].isValid != TRUE) {
    //         entry_available_in_rob = 1;
    //         break;
    //     }
    // }

    // if (entry_available_in_rob == 0) {
    //     cpu->godzilla.enter_godzilla = FALSE;
    //     return;
    // }

    if (cpu->rename_dispatch.opcode == OPCODE_LOAD || 
        cpu->rename_dispatch.opcode == OPCODE_STORE || 
        cpu->rename_dispatch.opcode == OPCODE_LOADP || 
        cpu->rename_dispatch.opcode == OPCODE_STOREP) {
        if (cpu->lsq_head == cpu->lsq_tail && cpu->cpu_lsq[cpu->lsq_head].isValid) {
            cpu->godzilla.enter_godzilla = FALSE;
            return;
        }
        // for (int i = 0; i < LSQ_SIZE; i++) {
        //     if (cpu->cpu_lsq[i].isValid != TRUE) {
        //         entry_available_in_lsq = 1;
        //         break;
        //     }
        // }

        // if (entry_available_in_lsq == 0) {
        //     cpu->godzilla.enter_godzilla = FALSE;
        //     return;
        // }
    }

    if (cpu->rename_dispatch.opcode == OPCODE_BNP || 
        cpu->rename_dispatch.opcode == OPCODE_BNZ || 
        cpu->rename_dispatch.opcode == OPCODE_BP || 
        cpu->rename_dispatch.opcode == OPCODE_BZ || 
        cpu->rename_dispatch.opcode == OPCODE_JUMP || 
        cpu->rename_dispatch.opcode == OPCODE_JALR) {
            // Yet to implement for branch instructions
    }

    cpu->godzilla.enter_godzilla = TRUE;
    if (cpu->godzilla.opcode == OPCODE_HALT)
    {
        cpu->godzilla.enter_godzilla = FALSE;
    }
}

/*
This method is used to setup an entry in the LSQ
*/
void setup_entry_in_lsq (APEX_CPU *cpu) {
    cpu->cpu_lsq[cpu->lsq_tail].dest = cpu->godzilla.pd;
    cpu->cpu_lsq[cpu->lsq_tail].mem_valid = FALSE;
    cpu->cpu_lsq[cpu->lsq_tail].lORs = (cpu->godzilla.opcode == OPCODE_LOAD || cpu->godzilla.opcode == OPCODE_LOADP) ? 1 : 0;
    cpu->cpu_lsq[cpu->lsq_tail].ps1_tag = cpu->godzilla.ps1;
    cpu->cpu_lsq[cpu->lsq_tail].ps1_valid = cpu->godzilla.ps1_valid;
    cpu->cpu_lsq[cpu->lsq_tail].ps2_tag = cpu->godzilla.ps2;
    cpu->cpu_lsq[cpu->lsq_tail].ps2_valid = cpu->godzilla.ps2_valid;
    cpu->cpu_lsq[cpu->lsq_tail].pd = cpu->godzilla.pd;
    cpu->cpu_lsq[cpu->lsq_tail].isValid = TRUE;
    cpu->godzilla.lsq_index = cpu->lsq_tail;
    cpu->lsq_tail = (cpu->lsq_tail + 1) % LSQ_SIZE;
}

/*
This method is used to setup an entry in the ROB
*/
void setup_entry_in_rob(APEX_CPU *cpu) {
    cpu->cpu_rob[cpu->rob_tail].overwritten_pd = cpu->godzilla.overwritten_pd;
    cpu->cpu_rob[cpu->rob_tail].pc = cpu->godzilla.pc;
    cpu->cpu_rob[cpu->rob_tail].pd = cpu->godzilla.pd;
    cpu->cpu_rob[cpu->rob_tail].rd = cpu->godzilla.rd;
    cpu->cpu_rob[cpu->rob_tail].lsq_index = cpu->godzilla.lsq_index;
    // cpu->cpu_rob[cpu->rob_tail].insn_type = cpu->godzilla.lsq_index != -1 ? dest_load_store : 0;
    if (cpu->godzilla.lsq_index != -1) {
        cpu->cpu_rob[cpu->rob_tail].insn_type = dest_load_store;
    }
    else if (cpu->godzilla.opcode == OPCODE_HALT) {
        cpu->cpu_rob[cpu->rob_tail].insn_type = dest_halt;
    }
    else {
        cpu->cpu_rob[cpu->rob_tail].insn_type = 0;
    }
    cpu->cpu_rob[cpu->rob_tail].isValid = TRUE;
    cpu->rob_tail = (cpu->rob_tail + 1) % ROB_SIZE;
}

/*
This method is used to setup an entry in the IQ
*/
void setup_entry_in_iq(APEX_CPU *cpu) {
    // // printf("Setting up entry in IQ");
    for (int i = 0; i < IQ_SIZE; i++) {
        if (cpu->cpu_iq[i].isValid == FALSE) {
            cpu->cpu_iq[i].dest = cpu->godzilla.pd;
            printf("\ngodzilla pd = %d\n", cpu->cpu_iq[i].dest);
            cpu->cpu_iq[i].literal = cpu->godzilla.imm;
            cpu->cpu_iq[i].ps1_tag = cpu->godzilla.ps1;
            if (cpu->cpu_prf[cpu->cpu_iq[i].ps1_tag].isValid) {
                cpu->cpu_iq[i].ps1_valid = TRUE;
            }
            else {
                cpu->cpu_iq[i].ps1_valid = cpu->godzilla.ps1_valid;
            }
            // cpu->cpu_iq[i].ps1_valid = cpu->godzilla.ps1_valid;
            cpu->cpu_iq[i].ps2_tag = cpu->godzilla.ps2;
            if (cpu->cpu_prf[cpu->cpu_iq[i].ps2_tag].isValid) {
                cpu->cpu_iq[i].ps2_valid = TRUE;
            }
            else {
                cpu->cpu_iq[i].ps2_valid = cpu->godzilla.ps2_valid;
            }
            // cpu->cpu_iq[i].ps2_valid = cpu->godzilla.ps2_valid;
            cpu->cpu_iq[i].clock_cycle_at_dispatch = cpu->clock;
            cpu->cpu_iq[i].function_type = cpu->godzilla.opcode;

            if (cpu->godzilla.opcode == OPCODE_HALT) {
                cpu->cpu_iq[i].FU = INT_FU;
            }
            
            if (cpu->godzilla.opcode == OPCODE_BNP || 
                cpu->godzilla.opcode == OPCODE_BNZ || 
                cpu->godzilla.opcode == OPCODE_BP || 
                cpu->godzilla.opcode == OPCODE_BZ || 
                cpu->godzilla.opcode == OPCODE_JUMP || 
                cpu->godzilla.opcode == OPCODE_JALR) {
                    // Yet to implement for branch instructions
            }
            else if (cpu->godzilla.opcode == OPCODE_LOAD) {
                cpu->cpu_iq[i].dest_type = dest_load_store;
                cpu->cpu_iq[i].dest = cpu->godzilla.lsq_index;
                cpu->cpu_iq[i].FU = ADD_FU;
                cpu->cpu_iq[i].ps2_valid = TRUE;
                
                // cpu->cpu_prf[cpu->godzilla.ps1].lsq_dependency_list[cpu->lsq_tail - 1] = 1;
                cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                // cpu->cpu_prf[cpu->godzilla.ps2].iq_dependency_list[i] = 1;
            }
            else if (cpu->godzilla.opcode == OPCODE_STORE) {
                cpu->cpu_iq[i].dest_type = dest_load_store;
                cpu->cpu_iq[i].dest = cpu->godzilla.lsq_index;
                cpu->cpu_iq[i].FU = ADD_FU;
                cpu->cpu_iq[i].ps1_valid = TRUE;
                
                cpu->cpu_prf[cpu->godzilla.ps1].lsq_dependency_list[cpu->lsq_tail - 1] = 1;
                // cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                cpu->cpu_prf[cpu->godzilla.ps2].iq_dependency_list[i] = 1;
            }
            else if (cpu->godzilla.opcode == OPCODE_LOADP) {
                cpu->cpu_iq[i].dest_type = dest_loadp_storep;
                cpu->cpu_iq[i].dest = cpu->godzilla.lsq_index;
                cpu->cpu_iq[i].FU = ADD_FU;
                cpu->cpu_iq[i].lpsp_inc_dest = cpu->godzilla.lpsp_inc_dest;
                printf("\nlpsp_inc_dest in godzilla = %d\n", cpu->cpu_iq[i].lpsp_inc_dest);
                cpu->cpu_iq[i].ps2_valid = TRUE;
                
                cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
            }
            else if (cpu->godzilla.opcode == OPCODE_STOREP) {
                cpu->cpu_iq[i].dest_type = dest_loadp_storep;
                cpu->cpu_iq[i].dest = cpu->godzilla.lsq_index;
                cpu->cpu_iq[i].FU = ADD_FU;
                cpu->cpu_iq[i].lpsp_inc_dest = cpu->godzilla.lpsp_inc_dest;
                printf("\nlpsp_inc_dest in godzilla = %d\n", cpu->cpu_iq[i].lpsp_inc_dest);
                cpu->cpu_iq[i].ps1_valid = TRUE;
                
                cpu->cpu_prf[cpu->godzilla.ps2].iq_dependency_list[i] = 1;
                cpu->cpu_prf[cpu->godzilla.ps1].lsq_dependency_list[cpu->lsq_tail - 1] = 1;
            }
            else {
                cpu->cpu_iq[i].dest = cpu->godzilla.pd;

                // cpu->cpu_prf[cpu->godzilla.pd].iq_dependency_list[i] = 1;

                switch (cpu->godzilla.opcode) {
                    case OPCODE_ADD:
                    case OPCODE_SUB:
                    case OPCODE_AND:
                    case OPCODE_OR:
                    case OPCODE_XOR:
                    case OPCODE_CMP:
                    {
                        cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                        cpu->cpu_prf[cpu->godzilla.ps2].iq_dependency_list[i] = 1;
                        cpu->cpu_iq[i].FU = INT_FU;

                        break;
                    }

                    case OPCODE_ADDL:
                    case OPCODE_SUBL:
                    case OPCODE_CML:
                    {
                        cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                        cpu->cpu_iq[i].FU = INT_FU;

                        break;
                    }

                    case OPCODE_LOAD:
                    case OPCODE_LOADP:
                    {
                        cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                        cpu->cpu_iq[i].FU = ADD_FU;

                        break;
                    }

                    case OPCODE_STORE:
                    case OPCODE_STOREP:
                    {
                        cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                        cpu->cpu_prf[cpu->godzilla.ps2].iq_dependency_list[i] = 1;
                        cpu->cpu_iq[i].FU = ADD_FU;

                        break;
                    }

                    case OPCODE_MOVC:
                    {
                        cpu->cpu_iq[i].FU = INT_FU;

                        break;
                    }

                    case OPCODE_MUL:
                    {
                        cpu->cpu_prf[cpu->godzilla.ps1].iq_dependency_list[i] = 1;
                        cpu->cpu_prf[cpu->godzilla.ps2].iq_dependency_list[i] = 1;
                        cpu->cpu_iq[i].FU = MUL_FU;

                        break;
                    }
                }
            }

            cpu->cpu_iq[i].isValid = TRUE;

            break;
        }
    }
}

/*
This method is used to wakeup instructions after tag broadcast
*/
void wakeup_instructions (APEX_CPU *cpu, int tag) {
    // // printf("\nWaking up instructions\n");

    int clock_max_intfu = INT32_MAX;
    int clock_max_addfu = INT32_MAX;
    int clock_max_mulfu = INT32_MAX;

    // printf("\nBroadcasted tag: %d\n", tag);
    if (tag != -1) {
        // for (int i = 0; i < IQ_SIZE; i++) {
        //     if (cpu->cpu_prf[tag].iq_dependency_list[i] == 1) {
        //         // printf("\nMul check: ps1[%d], ps2[%d], tag[%d]\n", cpu->cpu_iq[i].ps1_tag, cpu->cpu_iq[i].ps2_tag, tag);
        //         printf("\nSTOREP: ps1-valid: %d, broadcast = %d\n", cpu->cpu_prf[cpu->cpu_iq[i].ps1_tag].isValid, tag);
        //         if (cpu->cpu_iq[i].ps1_tag == tag) {
        //             cpu->cpu_iq[i].ps1_valid = TRUE;
        //         }

        //         if (cpu->cpu_iq[i].ps2_tag == tag) {
        //             cpu->cpu_iq[i].ps2_valid = TRUE;
        //         }
        //     }
        // }

        printf("\nBroadcasted tag = %d\n", tag);
        for (int i = 0; i < IQ_SIZE; i++) {
            // printf("\nis valid = %d for %d\n", cpu->cpu_iq[i].isValid, i);
            if (cpu->cpu_iq[i].isValid) {
                printf("\nIQ check: ps1 = %d, tag = %d\n", cpu->cpu_iq[i].ps1_tag, tag);
                if (cpu->cpu_iq[i].ps1_tag == tag) {
                    cpu->cpu_iq[i].ps1_valid = TRUE;
                }
                printf("\nIQ check: ps2 = %d, tag = %d\n", cpu->cpu_iq[i].ps2_tag, tag);
                if (cpu->cpu_iq[i].ps2_tag == tag) {
                    cpu->cpu_iq[i].ps2_valid = TRUE;
                }
            }
        }

        for (int i = cpu->lsq_head; i != cpu->lsq_tail; i = (i + 1) % LSQ_SIZE) {
            // printf("\nSTOREP: ps1-valid: %d, broadcast = %d\n", cpu->cpu_prf[cpu->cpu_iq[i].ps1_tag].isValid, cpu->intFU_broadcasted_tag);
            if (cpu->cpu_lsq[i].ps1_tag == tag) {
                cpu->cpu_lsq[i].ps1_valid = TRUE;
            }
        }
    }
    else {
        for (int i = 0; i < IQ_SIZE; i++) {
            if (cpu->cpu_iq[i].isValid) {
                if (cpu->cpu_prf[cpu->cpu_iq[i].ps1_tag].isValid) {
                    cpu->cpu_iq[i].ps1_valid = TRUE;
                }
                if (cpu->cpu_prf[cpu->cpu_iq[i].ps2_tag].isValid) {
                    cpu->cpu_iq[i].ps2_valid = TRUE;
                }
            }
        }
    }

    for (int i = 0; i < IQ_SIZE; i++) {
        if (cpu->cpu_iq[i].isValid && cpu->cpu_iq[i].FU == INT_FU) {
            if (cpu->cpu_iq[i].ps1_valid && cpu->cpu_iq[i].ps2_valid && cpu->execute.intFU.has_insn == FALSE) {
                if (cpu->cpu_iq[i].clock_cycle_at_dispatch < clock_max_intfu) {
                    clock_max_intfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;

                    cpu->godzilla.intfu_ready_insn = i;

                    // // printf("\ninsn woken up: iq[%d]\n", i);
                }
            }
        }
    }

    // printf("\nintfu_ready_insn: %d\n", cpu->godzilla.intfu_ready_insn);

    for (int i = 0; i < IQ_SIZE; i++) {
        if (cpu->cpu_iq[i].isValid && cpu->cpu_iq[i].FU == ADD_FU) {
            if (cpu->cpu_iq[i].ps1_valid && cpu->cpu_iq[i].ps2_valid && cpu->execute.addFU.has_insn == FALSE) {
                if (cpu->cpu_iq[i].clock_cycle_at_dispatch < clock_max_addfu) {
                    clock_max_addfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;

                    cpu->godzilla.addfu_ready_insn = i;
                }
            }
        }
    }

    for (int i = 0; i < IQ_SIZE; i++) {
        if (cpu->cpu_iq[i].isValid && cpu->cpu_iq[i].FU == MUL_FU) {
            if (cpu->cpu_iq[i].ps1_valid && cpu->cpu_iq[i].ps2_valid && cpu->execute.mulFU.has_insn == FALSE) {
                if (cpu->cpu_iq[i].clock_cycle_at_dispatch < clock_max_mulfu) {
                    clock_max_mulfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;

                    cpu->godzilla.mulfu_ready_insn = i;

                    // printf("\nPicked MUL insn: %d\n", i);
                }
            }
        }
    }

    // for (int i = 0; i < IQ_SIZE; i++) {
    //     if (cpu->cpu_prf[tag].iq_dependency_list[i] == 1) {
    //         if (cpu->cpu_iq[i].FU == INT_FU && cpu->cpu_iq[i].clock_cycle_at_dispatch < clock_max_intfu) {
    //             if (cpu->cpu_iq[i].ps1_tag == tag && cpu->cpu_iq[i].ps2_valid == TRUE) {
    //                 cpu->godzilla.intfu_ready_insn = i;
    //                 clock_max_intfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;
    //             }
    //             else if (cpu->cpu_iq[i].ps2_tag == tag && cpu->cpu_iq[i].ps1_valid == TRUE) {
    //                 cpu->godzilla.intfu_ready_insn = i;
    //                 clock_max_intfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;
    //             }
    //         }
    //         else if (cpu->cpu_iq[i].FU == ADD_FU && cpu->cpu_iq[i].clock_cycle_at_dispatch < clock_max_addfu) {
    //             cpu->godzilla.addfu_ready_insn = i;
    //             clock_max_addfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;
    //         }
    //         else if (cpu->cpu_iq[i].FU == MUL_FU && cpu->cpu_iq[i].clock_cycle_at_dispatch < clock_max_mulfu) {
    //             if (cpu->cpu_iq[i].ps1_tag == tag && cpu->cpu_iq[i].ps2_valid == TRUE) {
    //                 cpu->godzilla.mulfu_ready_insn = i;
    //                 clock_max_mulfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;
    //             }
    //             else if (cpu->cpu_iq[i].ps2_tag == tag && cpu->cpu_iq[i].ps1_valid == TRUE) {
    //                 cpu->godzilla.mulfu_ready_insn = i;
    //                 clock_max_mulfu = cpu->cpu_iq[i].clock_cycle_at_dispatch;
    //             }
    //         }
    //     }
    // }

    for (int i = 0; i < LSQ_SIZE; i++) {
        if (cpu->cpu_lsq[tag].mem_valid == TRUE) {
            if (cpu->cpu_lsq[tag].lORs == 0 && cpu->cpu_lsq[tag].ps2_valid == TRUE) {
                cpu->godzilla.lsq_target = i;
            }
            else if (cpu->cpu_lsq[tag].lORs == 1) {
                cpu->godzilla.lsq_target = i;
            }
        }
    }
}

/*
This method is used to broadcast the tags of Integer and Address Function Units
*/
void broadcast_tags (APEX_CPU *cpu) {
    // // printf("\nBroadcasting tags\n");

    if (cpu->execute.intFU.has_insn) {
        cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;
        cpu->intcc_broadcast_tag = cpu->execute.intFU.cc_tag;
    }
    wakeup_instructions(cpu, cpu->intFU_broadcasted_tag);

    if (cpu->execute.addFU.has_insn) {
        cpu->addFU_broadcasted_tag = cpu->execute.addFU.pd;
    }
    wakeup_instructions(cpu, cpu->addFU_broadcasted_tag);
}

/*
This method is used to perform load/store operation by popping out the entry at the head of the LSQ 
and performing the memory access operations
*/
void perform_load_store (APEX_CPU *cpu) {
    if (cpu->cpu_lsq[cpu->lsq_head].mem_valid) {
        if (cpu->cpu_lsq[cpu->lsq_head].lORs == 0) {
            if (cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].ps1_tag].isValid || 
                cpu->cpu_lsq[cpu->lsq_head].ps1_tag == cpu->intFU_broadcasted_tag || 
                cpu->cpu_lsq[cpu->lsq_head].ps1_tag == cpu->mulFU_broadcasted_tag) {
                cpu->godzilla.mem_stage_clock++;

                if (cpu->godzilla.mem_stage_clock == 2) {
                    if (cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].ps1_tag].isValid) {
                        cpu->data_memory[cpu->cpu_lsq[cpu->lsq_head].memory] = cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].ps1_tag].value;
                    }
                    else {
                        if (cpu->cpu_lsq[cpu->lsq_head].ps1_tag == cpu->intFU_broadcasted_tag) {
                            cpu->data_memory[cpu->cpu_lsq[cpu->lsq_head].memory] = cpu->intFU_broadcasted_value;
                        }
                        else if (cpu->cpu_lsq[cpu->lsq_head].ps1_tag == cpu->mulFU_broadcasted_tag) {
                            cpu->data_memory[cpu->cpu_lsq[cpu->lsq_head].memory] = cpu->mulFU_broadcasted_value;
                        }
                    }

                    cpu->godzilla.mem_stage_clock = 0;

                    cpu->cpu_lsq[cpu->lsq_head].isValid = FALSE;
                    cpu->lsq_head = (cpu->lsq_head + 1) % LSQ_SIZE;

                    if (cpu->cpu_rob[cpu->rob_head].overwritten_pd != -1) {
                        cpu->free_reg_list[cpu->free_reg_tail] = cpu->cpu_rob[cpu->rob_head].overwritten_pd;
                        cpu->free_reg_tail = (cpu->free_reg_tail + 1) % PRF_SIZE;
                    }

                    if (cpu->godzilla.mem_insn_reg_updt.dest != -1) {
                        cpu->regs[cpu->godzilla.mem_insn_reg_updt.dest] = cpu->cpu_prf[cpu->godzilla.mem_insn_reg_updt.incr_dest].value;
                        cpu->godzilla.mem_insn_reg_updt.dest = -1;
                    }

                    cpu->cpu_rob[cpu->rob_head].isValid = FALSE;
                    cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;
                }
            }
        }
        else if (cpu->cpu_lsq[cpu->lsq_head].lORs == 1) {
            cpu->godzilla.mem_stage_clock++;

            if (cpu->godzilla.mem_stage_clock == 2) {
                cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].pd].isValid = TRUE;
                cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].pd].value = cpu->data_memory[cpu->cpu_lsq[cpu->lsq_head].memory];

                cpu->godzilla.mem_stage_clock = 0;

                cpu->cpu_lsq[cpu->lsq_head].isValid = FALSE;
                cpu->lsq_head = (cpu->lsq_head + 1) % LSQ_SIZE;

                if (cpu->cpu_rob[cpu->rob_head].overwritten_pd != -1) {
                    cpu->free_reg_list[cpu->free_reg_tail] = cpu->cpu_rob[cpu->rob_head].overwritten_pd;
                    cpu->free_reg_tail = (cpu->free_reg_tail + 1) % PRF_SIZE;
                }

                cpu->regs[cpu->cpu_rob[cpu->rob_head].rd] = cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].pd].value;
                if (cpu->godzilla.mem_insn_reg_updt.dest != -1) {
                    cpu->regs[cpu->godzilla.mem_insn_reg_updt.dest] = cpu->cpu_prf[cpu->godzilla.mem_insn_reg_updt.incr_dest].value;
                    cpu->godzilla.mem_insn_reg_updt.dest = -1;
                }

                cpu->cpu_rob[cpu->rob_head].isValid = FALSE;
                cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;

                printf("\nmem[%d]=%d => p[%d] = %d\n", cpu->cpu_lsq[cpu->lsq_head].memory, cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].pd].value, cpu->cpu_lsq[cpu->lsq_head].pd, cpu->cpu_prf[cpu->cpu_lsq[cpu->lsq_head].pd].value);
                wakeup_instructions(cpu, cpu->cpu_lsq[cpu->lsq_head].pd);
            }
        }
    }
}

/*
This method implements:
- setting up entries in IQ,
- setting up entries in ROB,
- setting up entries in LSQ,
- setting up entries in BQ,
- setting up entries in BIS,
- waking up instructions for issue,
- committing instructions from ROB,
- making load/store accesses from LSQ
- notifying if dispatch should stall
*/
static void
APEX_Godzilla(APEX_CPU *cpu) {
    if (cpu->godzilla.has_insn) {
        if (cpu->godzilla.enter_godzilla == TRUE) {
            if (cpu->godzilla.opcode == OPCODE_HALT) {
                cpu->godzilla.enter_godzilla = FALSE;
            }

            if (cpu->godzilla.opcode == OPCODE_LOAD || 
                cpu->godzilla.opcode == OPCODE_STORE || 
                cpu->godzilla.opcode == OPCODE_LOADP || 
                cpu->godzilla.opcode == OPCODE_STOREP) {
                setup_entry_in_lsq(cpu);
            }
            else {
                cpu->godzilla.lsq_index = -1;
            }

            setup_entry_in_rob(cpu);

            setup_entry_in_iq(cpu);
        }

        // // printf("\nInstruction woken is: iq[%d]\n", cpu->godzilla.intfu_ready_insn);
        // printf("\nINTFU: Instruction ready to go: %d\n", cpu->godzilla.intfu_ready_insn);
        // printf("\nMULFU: Instruction ready to go: %d\n", cpu->godzilla.mulfu_ready_insn);

        wakeup_instructions(cpu, cpu->intFU_broadcasted_tag);

        if (cpu->execute.addFU.opcode == OPCODE_LOADP || cpu->execute.addFU.opcode == OPCODE_STOREP) {
            printf("\nWaking after loadp/storep: %d\n", cpu->execute.addFU.lpsp_inc_dest);
            wakeup_instructions(cpu, cpu->execute.addFU.lpsp_inc_dest);
        }
        // wakeup_instructions(cpu, cpu->addFU_broadcasted_tag);

        if (cpu->godzilla.intfu_ready_insn != -1) {
            if (cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].function_type == OPCODE_HALT) {
                cpu->execute.intFU.has_insn = TRUE;
                cpu->execute.intFU.pd = -1;
                cpu->execute.intFU.ps1 = -1;
                cpu->execute.intFU.ps2 = -1;
            }
            else {
                cpu->execute.intFU.has_insn = TRUE;

                cpu->execute.intFU.pd = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].dest;
                
                if (cpu->intFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag) {
                    cpu->execute.intFU.ps1_value = cpu->intFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_valid = 1;
                    cpu->execute.intFU.ps1 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag] = 0;
                }
                else if (cpu->mulFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag) {
                    cpu->execute.intFU.ps1_value = cpu->mulFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_valid = 1;
                    cpu->execute.intFU.ps1 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag;
                    cpu->execute.intFU.forwarded_from_mul = 1;

                    // printf("\nMUL forwarded to INT: pd[%d], ps1[%d]:%d, ps2[%d]:%d\n", cpu->execute.intFU.pd, cpu->execute.intFU.ps1, cpu->execute.intFU.ps1_value, cpu->execute.intFU.ps2, cpu->execute.intFU.ps2_value);

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag] = 0;
                }
                else {
                    cpu->execute.intFU.ps1_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].value;
                    cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_valid = 1;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag] = 0;
                }

                if (cpu->intFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag) {
                    cpu->execute.intFU.ps2_value = cpu->intFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_valid = 1;
                    cpu->execute.intFU.ps2 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag] = 0;
                }
                else if (cpu->mulFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag) {
                    cpu->execute.intFU.ps2_value = cpu->mulFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_valid = 1;
                    cpu->execute.intFU.ps2 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag;
                    cpu->execute.intFU.forwarded_from_mul = 2;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag] = 0;
                }
                else {
                    cpu->execute.intFU.ps2_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag].value;
                    cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_valid = 1;
                    cpu->execute.intFU.ps2 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag] = 0;
                }
                
                // if (cpu->intFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag &&
                //     cpu->mulFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag &&
                //     cpu->intFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag &&
                //     cpu->mulFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag) {
                //     cpu->execute.intFU.ps1 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag;
                //     cpu->execute.intFU.ps2 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag;
                //     cpu->execute.intFU.ps1_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].value;
                //     cpu->execute.intFU.ps2_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag].value;

                //     cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag] = 0;
                //     cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps2_tag] = 0;
                // }

                cpu->execute.intFU.imm = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].literal;
                cpu->execute.intFU.opcode = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].function_type;

                // printf("\npd[%d] = ps1[%d], ps2[%d], imm[%d]\n", cpu->execute.intFU.pd, cpu->execute.intFU.ps1, cpu->execute.intFU.ps2, cpu->execute.intFU.imm);

                cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].isValid = FALSE;
                cpu->godzilla.intfu_ready_insn = -1;
            }
            // // printf("\nexecute: pd: %d, ps1: %d, ps2: %d\n", cpu->execute.intFU.pd, cpu->execute.intFU.ps1, cpu->execute.intFU.ps2);
        }

        if (cpu->godzilla.addfu_ready_insn != -1) {
            cpu->execute.addFU.has_insn = TRUE;

            cpu->execute.addFU.pd = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].dest;
            cpu->execute.addFU.lpsp_inc_dest = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].lpsp_inc_dest;
            
            if (cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].function_type == OPCODE_LOAD || cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].function_type == OPCODE_LOADP) {
                if (cpu->intFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag) {
                    cpu->execute.addFU.ps1_value = cpu->intFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_valid = 1;
                    cpu->execute.addFU.ps1 = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag] = 0;
                }
                else if (cpu->mulFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag) {
                    cpu->execute.addFU.ps1_value = cpu->mulFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_valid = 1;
                    cpu->execute.addFU.ps1 = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag;
                    cpu->execute.addFU.forwarded_from_mul = 1;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag] = 0;
                }
                else {
                    cpu->execute.addFU.ps1 = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag;
                    cpu->execute.addFU.ps1_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag].value;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps1_tag] = 0;
                }
            }
            else {
                if (cpu->intFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag) {
                    cpu->execute.addFU.ps2_value = cpu->intFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_valid = 1;
                    cpu->execute.addFU.ps2 = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag] = 0;
                }
                else if (cpu->mulFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag) {
                    cpu->execute.addFU.ps2_value = cpu->mulFU_broadcasted_value;
                    cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_valid = 1;
                    cpu->execute.addFU.ps2 = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag;
                    cpu->execute.addFU.forwarded_from_mul = 1;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag] = 0;
                }
                else {
                    cpu->execute.addFU.ps2 = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag;
                    cpu->execute.addFU.ps2_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag].value;

                    cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].ps2_tag] = 0;
                }
            }

            // if (cpu->intFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag &&
            //     cpu->mulFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag) {
            //     cpu->execute.intFU.ps1 = cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag;
            //     cpu->execute.intFU.ps1_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].value;

            //     cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].ps1_tag] = 0;
            // }

            cpu->execute.addFU.imm = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].literal;
            cpu->execute.addFU.opcode = cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].function_type;

            cpu->cpu_iq[cpu->godzilla.addfu_ready_insn].isValid = FALSE;
            cpu->godzilla.addfu_ready_insn = -1;

            // // printf("\nexecute: pd: %d, ps1: %d\n", cpu->execute.addFU.pd, cpu->execute.addFU.ps1);
        }

        if (cpu->godzilla.mulfu_ready_insn != -1) {
            cpu->execute.mulFU.has_insn = TRUE;

            cpu->execute.mulFU.pd = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].dest;
            
            if (cpu->mulFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag) {
                cpu->execute.mulFU.ps1_value = cpu->mulFU_broadcasted_value;
                cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_valid = 1;
                cpu->execute.mulFU.ps1 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag;
                cpu->execute.mulFU.forwarded_from_mul = 1;

                cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag] = 0;

                // printf("\n1. Mul insn being sent: pd[%d], ps1[%d], ps2[%d]\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
            }
            else if (cpu->intFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag) {
                cpu->execute.mulFU.ps1_value = cpu->intFU_broadcasted_value;
                cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_valid = 1;
                cpu->execute.mulFU.ps1 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag;

                cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag] = 0;

                // printf("\n2. Mul insn being sent: pd[%d], ps1[%d], ps2[%d]\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
            }
            else {
                cpu->execute.mulFU.ps1 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag;
                cpu->execute.mulFU.ps1_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag].value;
                cpu->execute.mulFU.ps1 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag;

                cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag] = 0;

                // printf("\n3. Mul insn being sent: pd[%d], ps1[%d]: %d, ps2[%d]: %d\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps1_value, cpu->execute.mulFU.ps2, cpu->execute.mulFU.ps2_value);
            }

            if (cpu->mulFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag) {
                cpu->execute.mulFU.ps2_value = cpu->mulFU_broadcasted_value;
                cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_valid = 1;
                cpu->execute.mulFU.ps2 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag;
                cpu->execute.mulFU.forwarded_from_mul = 2;

                cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag] = 0;

                // printf("\n4. Mul insn being sent: pd[%d], ps1[%d], ps2[%d]\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
            }
            else if (cpu->intFU_broadcasted_tag == cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag) {
                cpu->execute.mulFU.ps2_value = cpu->intFU_broadcasted_value;
                cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_valid = 1;
                cpu->execute.mulFU.ps2 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag;

                cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag] = 0;

                // printf("\n5. Mul insn being sent: pd[%d], ps1[%d]: %d, ps2[%d]: %d\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps1_value, cpu->execute.mulFU.ps2, cpu->execute.mulFU.ps2_value);            
                }
            else {
                cpu->execute.mulFU.ps2 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag;
                cpu->execute.mulFU.ps2_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag].value;
                cpu->execute.mulFU.ps2 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag;

                cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag] = 0;

                // printf("\n6. Mul insn being sent: pd[%d], ps1[%d], ps2[%d]\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
            }

            // if (cpu->intFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag &&
            //     cpu->mulFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag &&
            //     cpu->intFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag &&
            //     cpu->mulFU_broadcasted_tag != cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag) {
            //     cpu->execute.intFU.ps1 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag;
            //     cpu->execute.intFU.ps2 = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag;
            //     cpu->execute.intFU.ps1_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag].value;
            //     cpu->execute.intFU.ps2_value = cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag].value;

            //     cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps1_tag] = 0;
            //     cpu->cpu_prf[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag].iq_dependency_list[cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].ps2_tag] = 0;

            //     // printf("\n5. Mul insn being sent: pd[%d], ps1[%d], ps2[%d]\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
            // }

            cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].isValid = FALSE;
            cpu->execute.mulFU.opcode = cpu->cpu_iq[cpu->godzilla.mulfu_ready_insn].function_type;
            cpu->godzilla.mulfu_ready_insn = -1;

            // // printf("\nexecute: pd: %d, ps1: %d, ps2: %d\n", cpu->execute.mulFU.pd, cpu->execute.mulFU.ps1, cpu->execute.mulFU.ps2);
        }

        if (cpu->addFU_broadcasted_tag != -1) {
            cpu->cpu_lsq[cpu->addFU_broadcasted_tag].memory = cpu->addFU_broadcasted_value;
            cpu->cpu_lsq[cpu->addFU_broadcasted_tag].mem_valid = TRUE;

            cpu->addFU_broadcasted_tag = -1;
        }

        // printf("\nBroadcasted tag is: %d[%d] -> value is: %d\n", cpu->intFU_broadcasted_tag, cpu->cpu_prf[cpu->intFU_broadcasted_tag].isValid, cpu->intFU_broadcasted_value);
        if (cpu->intFU_broadcasted_tag != -1) {
            cpu->cpu_prf[cpu->intFU_broadcasted_tag].isValid = TRUE;
            cpu->cpu_prf[cpu->intFU_broadcasted_tag].value = cpu->intFU_broadcasted_value;

            // // printf("\nUpdated prf: prf[%d[%d]] = %d\n", cpu->intFU_broadcasted_tag, cpu->cpu_prf[cpu->intFU_broadcasted_tag].isValid, cpu->cpu_prf[cpu->intFU_broadcasted_tag].value);
        }
        if (cpu->mulFU_broadcasted_tag != -1 && cpu->execute.mulFU_clock == 0) {
            cpu->cpu_prf[cpu->mulFU_broadcasted_tag].isValid = TRUE;
            cpu->cpu_prf[cpu->mulFU_broadcasted_tag].value = cpu->mulFU_broadcasted_value;
        }

        if (cpu->cpu_rob[cpu->rob_head].insn_type == dest_load_store || cpu->cpu_rob[cpu->rob_head].insn_type == dest_loadp_storep) {
            perform_load_store(cpu);
        }
        else if (cpu->cpu_rob[cpu->rob_head].insn_type == dest_halt) {
            if (cpu->cpu_iq[cpu->godzilla.intfu_ready_insn].function_type == OPCODE_HALT) {
                cpu->execute.is_halt_insn = TRUE;
                cpu->godzilla.has_insn = FALSE;
                return;
            }
        }
        else {
            // printf("\nChecking if rob head is ready to commit: %d == %d\n", cpu->cpu_rob[cpu->rob_head].pd, cpu->cpu_prf[cpu->cpu_rob[cpu->rob_head].pd].isValid);
            if (cpu->cpu_prf[cpu->cpu_rob[cpu->rob_head].pd].isValid == TRUE) {
                cpu->regs[cpu->cpu_rob[cpu->rob_head].rd] = cpu->cpu_prf[cpu->cpu_rob[cpu->rob_head].pd].value;

                // printf("\nPutting %d into reg\n", cpu->cpu_prf[cpu->cpu_rob[cpu->rob_head].pd].value);

                if (cpu->cpu_rob[cpu->rob_head].overwritten_pd != -1) {
                    cpu->free_reg_list[cpu->free_reg_tail] = cpu->cpu_rob[cpu->rob_head].overwritten_pd;
                    cpu->free_reg_tail = (cpu->free_reg_tail + 1) % PRF_SIZE;
                }

                cpu->cpu_rob[cpu->rob_head].isValid = FALSE;
                cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;

                // printf("\nReg updated is: %d: %d\n", cpu->cpu_rob[cpu->rob_head-1].rd, cpu->regs[cpu->cpu_rob[cpu->rob_head-1].rd]);
            }
            // else if (cpu->cpu_rob[cpu->rob_head].pd == cpu->intFU_broadcasted_tag) {
            //     cpu->regs[cpu->cpu_rob[cpu->rob_head].rd] = cpu->intFU_broadcasted_value;

            //     if (cpu->cpu_rob[cpu->rob_head].overwritten_pd != -1) {
            //         cpu->free_reg_list[cpu->free_reg_tail] = cpu->cpu_rob[cpu->rob_head].overwritten_pd;
            //         cpu->free_reg_tail = (cpu->free_reg_tail + 1) % PRF_SIZE;
            //     }

            //     cpu->cpu_rob[cpu->rob_head].isValid = FALSE;
            //     cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;
            // }
            // else if (cpu->cpu_rob[cpu->rob_head].pd == cpu->mulFU_broadcasted_tag) {
            //     cpu->regs[cpu->cpu_rob[cpu->rob_head].rd] = cpu->mulFU_broadcasted_value;

            //     if (cpu->cpu_rob[cpu->rob_head].overwritten_pd != -1) {
            //         cpu->free_reg_list[cpu->free_reg_tail] = cpu->cpu_rob[cpu->rob_head].overwritten_pd;
            //         cpu->free_reg_tail = (cpu->free_reg_tail + 1) % PRF_SIZE;
            //     }

            //     cpu->cpu_rob[cpu->rob_head].isValid = FALSE;
            //     cpu->rob_head = (cpu->rob_head + 1) % ROB_SIZE;
            // }
        }

        if (cpu->intFU_broadcasted_tag != -1) {
            // printf("\nUpdated prf: prf[%d[%d]] = %d\n", cpu->intFU_broadcasted_tag, cpu->cpu_prf[cpu->intFU_broadcasted_tag].isValid, cpu->cpu_prf[cpu->intFU_broadcasted_tag].value);
        }

        cpu->intFU_broadcasted_tag = -1;
        cpu->mulFU_broadcasted_tag = -1;
        cpu->addFU_broadcasted_tag = -1;

        should_dispatch_stall(cpu);

        // printf("\nExecute.has_insn before broadcast: %d for pd[%d]\n", cpu->execute.intFU.has_insn, cpu->execute.intFU.pd);
        // broadcast_tags(cpu);

        if (ENABLE_DEBUG_MESSAGES) {
            print_godzilla(cpu);
        }
    }
}

#pragma endregion - Godzilla Stage

#pragma region - Execute Stage 

/*
This function is used to implement the Integer FU of the execute stage
*/
void run_intFU (APEX_CPU *cpu) {
    if (cpu->execute.intFU.has_insn) {
        if (cpu->execute.intFU.forwarded_from_mul == 1) {
            cpu->execute.intFU.ps1_value = cpu->mulFU_broadcasted_value;
            cpu->execute.intFU.forwarded_from_mul = 0;
            // printf("\nEXEC - MUL forwarded value: %d\n", cpu->execute.intFU.ps1_value);
        }
        else if (cpu->execute.intFU.forwarded_from_mul == 2) {
            cpu->execute.intFU.ps2_value = cpu->mulFU_broadcasted_value;
            cpu->execute.intFU.forwarded_from_mul = 0;
        }

        switch (cpu->execute.intFU.opcode) {
            case OPCODE_ADD:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value + cpu->execute.intFU.ps2_value;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value + cpu->execute.intFU.imm;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }

            case OPCODE_SUB:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value - cpu->execute.intFU.ps2_value;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value - cpu->execute.intFU.imm;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }
            
            case OPCODE_OR:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value | cpu->execute.intFU.ps2_value;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value ^ cpu->execute.intFU.ps2_value;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }

            case OPCODE_CMP:
            case OPCODE_CML:
            {
                break;
            }

            case OPCODE_AND:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.ps1_value & cpu->execute.intFU.ps2_value;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }

            case OPCODE_MOVC:
            {
                cpu->execute.intFU.result_buffer = cpu->execute.intFU.imm + 0;

                cpu->intFU_broadcasted_value = cpu->execute.intFU.result_buffer;
                cpu->intFU_broadcasted_tag = cpu->execute.intFU.pd;

                break;
            }
        }

        cpu->execute.intFU.has_insn = FALSE;

        wakeup_instructions(cpu, cpu->intFU_broadcasted_tag);
    }
}

/*
This function is used to implement the Multiplication FU of the execute stage
*/
void run_mulFU (APEX_CPU *cpu) {
    if (cpu->execute.mulFU.has_insn) {
        if (cpu->execute.mulFU.forwarded_from_mul == 1) {
            cpu->execute.mulFU.ps1_value = cpu->mulFU_broadcasted_value;
            cpu->execute.mulFU.forwarded_from_mul = 0;

            // printf("\nEXEC - MUL forwarded value: %d\n", cpu->execute.mulFU.ps1_value);
        }
        else if (cpu->execute.mulFU.forwarded_from_mul == 2) {
            cpu->execute.mulFU.ps2_value = cpu->mulFU_broadcasted_value;
            cpu->execute.mulFU.forwarded_from_mul = 0;

            // printf("\nEXEC - MUL forwarded value: %d\n", cpu->execute.mulFU.ps2_value);
        }

        if (cpu->execute.mulFU_clock == 0) {
            cpu->execute.mulFU.result_buffer = cpu->execute.mulFU.ps1_value * cpu->execute.mulFU.ps2_value;
            // printf("\nMUL => %d = %d X %d\n", cpu->execute.mulFU.result_buffer, cpu->execute.mulFU.ps1_value, cpu->execute.mulFU.ps2_value);
        }
        else if (cpu->execute.mulFU_clock == 1) {
            // cpu->mulFU_broadcasted_tag = cpu->execute.mulFU.pd;

            // wakeup_instructions(cpu, cpu->mulFU_broadcasted_tag);
        }
        else if (cpu->execute.mulFU_clock == 2) {
            cpu->mulFU_broadcasted_tag = cpu->execute.mulFU.pd;
            cpu->mulFU_broadcasted_value = cpu->execute.mulFU.result_buffer;

            // printf("\nMulFU fowarded value: %d\n", cpu->mulFU_broadcasted_value);
            cpu->execute.mulFU.has_insn = FALSE;

            cpu->mulFU_broadcasted_tag = cpu->execute.mulFU.pd;

            wakeup_instructions(cpu, cpu->mulFU_broadcasted_tag);
        }

        cpu->execute.mulFU_clock = (cpu->execute.mulFU_clock + 1)%3;
    }
}

/*
This function is used to implement the Address Calculation FU of the execute stage
*/
void run_addFU (APEX_CPU *cpu) {
    if (cpu->execute.addFU.has_insn) {
        if (cpu->execute.addFU.forwarded_from_mul == 1) {
            cpu->execute.addFU.ps1_value = cpu->mulFU_broadcasted_value;
            cpu->execute.addFU.forwarded_from_mul = 0;

            // printf("\nEXEC - MUL forwarded value: %d\n", cpu->execute.addFU.ps1_value);
        }

        printf("\nOPCODE is : %d\n", cpu->execute.addFU.opcode);

        if (cpu->execute.addFU.opcode == OPCODE_LOADP) {
            cpu->execute.addFU.result_buffer = cpu->execute.addFU.ps1_value + cpu->execute.addFU.imm;

            cpu->addFU_broadcasted_value = cpu->execute.addFU.result_buffer;
            cpu->addFU_broadcasted_tag = cpu->execute.addFU.pd;

            cpu->cpu_prf[cpu->execute.addFU.lpsp_inc_dest].value = cpu->execute.addFU.ps1_value + 4;
            cpu->cpu_prf[cpu->execute.addFU.lpsp_inc_dest].isValid = TRUE;

            // wakeup_instructions(cpu, cpu->execute.addFU.lpsp_inc_dest);

            cpu->godzilla.mem_insn_reg_updt.dest = cpu->cpu_lsq[cpu->execute.addFU.pd].dest;
            cpu->godzilla.mem_insn_reg_updt.incr_dest = cpu->execute.addFU.lpsp_inc_dest;

            printf("\nInit - LOADP: Committing %d -> %d\n", cpu->godzilla.mem_insn_reg_updt.dest, cpu->godzilla.mem_insn_reg_updt.incr_dest);
        }
        else if (cpu->execute.addFU.opcode == OPCODE_STOREP) {
            cpu->execute.addFU.result_buffer = cpu->execute.addFU.ps2_value + cpu->execute.addFU.imm;

            cpu->addFU_broadcasted_value = cpu->execute.addFU.result_buffer;
            cpu->addFU_broadcasted_tag = cpu->execute.addFU.pd;

            cpu->cpu_prf[cpu->execute.addFU.lpsp_inc_dest].value = cpu->execute.addFU.ps2_value + 4;
            cpu->cpu_prf[cpu->execute.addFU.lpsp_inc_dest].isValid = TRUE;

            // wakeup_instructions(cpu, cpu->execute.addFU.lpsp_inc_dest);

            cpu->godzilla.mem_insn_reg_updt.dest = cpu->cpu_lsq[cpu->execute.addFU.pd].dest;
            cpu->godzilla.mem_insn_reg_updt.incr_dest = cpu->execute.addFU.lpsp_inc_dest;

            printf("\nInit - STOREP: Committing %d -> %d\n", cpu->godzilla.mem_insn_reg_updt.dest, cpu->godzilla.mem_insn_reg_updt.incr_dest);
        }

        cpu->execute.addFU.has_insn = FALSE;

        // wakeup_instructions(cpu, cpu->addFU_broadcasted_tag);
    }
}

/*
This method implements the EXECUTE stage of the pipeline
*/
static void APEX_execute (APEX_CPU *cpu) {
    if (cpu->execute.is_halt_insn) {
        cpu->halt_cpu = TRUE;
    }

    run_mulFU(cpu);

    run_intFU(cpu);

    run_addFU(cpu);

    if (ENABLE_DEBUG_MESSAGES) {
        print_execute(cpu);
    }
}

#pragma endregion - Execute Stage

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
        cpu->cpu_prf[i].isValid = TRUE;
        cpu->cpu_prf[i].dependency_count = 0;
        for (int j = 0; j < IQ_SIZE; j++) {
            cpu->cpu_prf[i].iq_dependency_list[j] = 0;
        }
        
        for (int j = 0; j < LSQ_SIZE; j++) {
            cpu->cpu_prf[i].lsq_dependency_list[j] = 0;
        }
        cpu->cpu_prf[i].value = -1;
    }

    cpu->free_reg_head = 0;
    cpu->free_reg_tail = 0;

    for (int i = 0; i < IQ_SIZE; i++) {
        cpu->cpu_iq[i].isValid = FALSE;
    }

    cpu->rob_head = 0;
    cpu->rob_tail = 0;

    cpu->lsq_head = 0;
    cpu->lsq_tail = 0;

    for (int i = 0; i < ROB_SIZE; i++) {
        cpu->cpu_rob[i].isValid = FALSE;
    }

    for (int i = 0; i < LSQ_SIZE; i++) {
        cpu->cpu_lsq[i].isValid = FALSE;
    }

    for (int i = 0; i < REG_FILE_SIZE; i++) {
        cpu->rename_table[i] = -1;
    }

    for (int i = 0; i < PRF_SIZE; i++) {
        cpu->free_reg_list[i] = i;
    }

    cpu->free_reg_head = 0;
    cpu->free_reg_tail = 0;
    printf("\nFree list of registers: \n");
    for (int i = cpu->free_reg_head + 1; i != cpu->free_reg_tail; i = (i + 1)%PRF_SIZE) {
        printf("%d\t", cpu->free_reg_list[i-1]);
    }
    printf("\n\n");

    cpu->execute.addFU.has_insn = FALSE;
    cpu->execute.addFU.forwarded_from_mul = 0;
    cpu->execute.intFU.has_insn = FALSE;
    cpu->execute.intFU.forwarded_from_mul = 0;
    cpu->execute.mulFU.has_insn = FALSE;
    cpu->execute.mulFU.forwarded_from_mul = 0;
    cpu->execute.mulFU_clock = 0;
    cpu->execute.is_halt_insn = FALSE;

    cpu->godzilla.intfu_ready_insn = -1;
    cpu->godzilla.addfu_ready_insn = -1;
    cpu->godzilla.mulfu_ready_insn = -1;
    cpu->godzilla.lsq_target = -1;
    cpu->godzilla.mem_stage_clock = 0;
    cpu->godzilla.has_insn = FALSE;
    cpu->godzilla.enter_godzilla = TRUE;

    cpu->decode_rename.has_insn = FALSE;
    cpu->rename_dispatch.has_insn = FALSE;

    cpu->intFU_broadcasted_tag = -1;
    cpu->mulFU_broadcasted_tag = -1;
    cpu->addFU_broadcasted_tag = -1;

    cpu->halt_cpu = FALSE;

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
This method is used to print the contents of the stage after every clock cycle
*/
void print_cpu_status (APEX_CPU *cpu) {
    print_stage_content("Fetch", &cpu->fetch);
    print_stage_content("Decode1", &cpu->decode_rename);
    print_stage_content("Decode2", &cpu->rename_dispatch);
    print_godzilla(cpu);
    print_execute(cpu);
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
        // if (cpu->clock >= cpu->cycles_limit)
        // {
        //     printf("\nYou've exhausted all the clock cycles!\n");
        //     break;
        // }

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }

        // if (APEX_Dispatch(cpu))
        // {
        //     /* Halt in writeback stage */
        //     printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
        //     print_reg_file(cpu);
        //     break;
        // }

        APEX_execute(cpu);
        APEX_Godzilla(cpu);
        APEX_Dispatch(cpu);
        APEX_Decode(cpu);
        APEX_fetch(cpu);

        print_prf(cpu);
        print_reg_file(cpu);

        if (cpu->halt_cpu) {
            break;
        }

        // print_cpu_status(cpu);

        printf("Press any key to advance CPU Clock or <q> to quit or <d> to display register files:\n");
        scanf("%c", &user_prompt_val);

        if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
        {
            printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }
        else if ((user_prompt_val == 'D') || (user_prompt_val == 'd')) {
            printf("\nPRINTING REGISTER FILES\n");
            print_phy_reg_file(cpu);
            print_reg_file(cpu);
            printf("\nPRINTED REGISTER FILES\n");
        }

        // if (cpu->single_step)
        // {
        //     printf("Press any key to advance CPU Clock or <q> to quit:\n");
        //     scanf("%c", &user_prompt_val);

        //     if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
        //     {
        //         printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
        //         break;
        //     }
        // }
        // else
        // {
        //     if(cpu->clock == cpu->sim_n - 1)
        //     {
        //         printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
        //         break;
        //     }
        // }

        cpu->clock++;
    }

    print_reg_file(cpu);
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

// void ns_print_stage_content(const char *name, const CPU_Stage *stage)
// {
//     print_stage_content(name, stage);
// }

// void ns_print_reg_file(const APEX_CPU *cpu)
// {
//     print_reg_file(cpu);
// }
