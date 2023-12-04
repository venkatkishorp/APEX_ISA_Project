/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "apex_cpu.h"

/*
 * This function is related to parsing input file
 *
 * Note : You are not supposed to edit this function
 */
static int
get_num_from_string(const char buffer[])
{
    char str[16];
    int i, j = 0;

    for (i = 0; buffer[i] != '\0'; ++i)
    {
        if (buffer[i] != '#' && buffer[i] != ' ' && (buffer[i] == '-' || (buffer[i] >= '0' && buffer[i] <= '9'))) {
            str[j] = buffer[i];
            j++;
        }
    }
    str[j] = '\0';

    return atoi(str);
}

int
main(int argc, char const *argv[])
{
    APEX_CPU *cpu = NULL;
    char command;
    int run_sim = TRUE;

    fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);

    if (argc != 4)
    {
        fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
        exit(1);
    }

    // cpu = APEX_cpu_init(argv[1]);
    // if (!cpu)
    // {
    //     fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
    //     exit(1);
    // }

    // APEX_cpu_run(cpu);

    while (run_sim) {
        printf("\nPress <i> to Initialize, <s> to Single Step, <d> to Display stage values, <m> to Show memory, <q> to Quit simulator\n");
        command = getchar();
        getchar();

        command = tolower(command);

        switch(command) {
            case 'i':
            {
                cpu = APEX_cpu_init(argv[1]);
                if (!cpu)
                {
                    fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
                    exit(1);
                }

                printf("\n%d\n", get_num_from_string(argv[3]));
                cpu->cycles_limit = get_num_from_string(argv[3]);
                break;
            }

            case 's':
            {
                if (!cpu) {
                    fprintf(stderr, "APEX_Error: Did not initialize CPU\n");
                    break;
                }

                printf("\nAbout to run sim\n");

                APEX_cpu_run(cpu);
                break;
            }

            case 'd':
            {
                if (!cpu) {
                    fprintf(stderr, "APEX_Error: Did not initialize CPU\n");
                    break;
                }

                display_function(cpu);
                break;
            }

            case 'm':
            {
                if (!cpu) {
                    fprintf(stderr, "APEX_Error: Did not initialize CPU\n");
                    break;
                }

                printf("Enter the memory address: ");
                int add = 0;
                scanf("%d", &add);
                getchar();

                printf("\nMEM[%d] = %d\n", add, cpu->data_memory[add]);

                break;
            }

            case 'q':
            {
                APEX_cpu_stop(cpu);
                exit(1);
            }
        }
    }

    return 0;
}