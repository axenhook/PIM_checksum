/*
 * Copyright (c) 2014-2019 - UPMEM
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file host.c
 * @brief Template for a Host Application Source File.
 */

#include <dpu.h>
#include <dpu_log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"

#ifndef DPU_BINARY
#define DPU_BINARY "build/checksum_dpu"
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

static uint32_t test_file[BUFFER_SIZE/4];

/**
 * @brief creates a "test file"
 *
 * @return the theorical checksum value
 */
static uint32_t create_test_file()
{
    uint32_t checksum = 0;
    srand(0);

    for (uint32_t i = 0; i < BUFFER_SIZE/4; i++) {
        test_file[i] = (uint32_t)(rand());
        checksum += test_file[i];
    }

    return checksum;
}

static inline unsigned long long get_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec * 1.0e9 + t.tv_nsec);
}

/**
 * @brief Main of the Host Application.
 */
int main()
{
    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus;
    uint32_t theoretical_checksum, dpu_checksum;
    uint32_t dpu_cycles;
    bool status = true;
    unsigned long long ns;

    DPU_ASSERT((BUFFER_SIZE % 4));

    ns = get_ns();
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    printf("alloc dpu %d DPUs: %llu ns\n", NR_DPUS, get_ns() - ns);

    ns = get_ns();
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));
    printf("upload dpu program %d DPUs: %llu ns\n", NR_DPUS, get_ns() - ns);

    ns = get_ns();
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    printf("get available dpus num %d DPUs: %llu ns\n", nr_of_dpus, get_ns() - ns);

    // Create an "input file" with arbitrary data.
    // Compute its theoretical checksum value.
    theoretical_checksum = create_test_file();

    ns = get_ns();
    DPU_ASSERT(dpu_copy_to(dpu_set, XSTR(DPU_BUFFER), 0, test_file, BUFFER_SIZE));
    printf("upload data %d DPUs: %llu ns\n", nr_of_dpus, get_ns() - ns);

    ns = get_ns();
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    printf("run program %d DPUs: %llu ns\n", nr_of_dpus, get_ns() - ns);

    {
        unsigned int each_dpu = 0;
        printf("Display DPU Logs\n");
        DPU_FOREACH (dpu_set, dpu) {
            //printf("DPU#%d: \n", each_dpu);
            ns = get_ns();
            DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
            printf("read log from dpu %d %d DPUs: %llu ns\n", each_dpu, nr_of_dpus, get_ns() - ns);
            each_dpu++;
        }
    }

    printf("Retrieve results\n");
    DPU_FOREACH (dpu_set, dpu) {
        bool dpu_status;
        dpu_checksum = 0;
        dpu_cycles = 0;

        // Retrieve tasklet results and compute the final checksum.
        for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++) {
            dpu_results_t result;
          //  ns = get_ns();
          DPU_ASSERT(
                dpu_copy_from(dpu, XSTR(DPU_RESULTS), each_tasklet * sizeof(dpu_results_t), &result, sizeof(dpu_results_t)));
        //    printf("read result from dpu %d DPUs: %llu ns tasklet: %d\n", nr_of_dpus, get_ns() - ns, each_tasklet);

            dpu_checksum += result.checksum;
            if (result.cycles > dpu_cycles)
                dpu_cycles = result.cycles;
        }

        dpu_status = (dpu_checksum == theoretical_checksum);
        status = status && dpu_status;

        printf("DPU execution time  = %g cycles\n", (double)dpu_cycles);
       // printf("performance         = %g cycles/byte\n", (double)dpu_cycles / BUFFER_SIZE);
      //  printf("checksum computed by the DPU = 0x%08x\n", dpu_checksum);
      //  printf("actual checksum value        = 0x%08x\n", theoretical_checksum);
        if (dpu_status) {
        //    printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] checksums are equal.\n");
        } else {
            printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] checksums differ! real: 0x%x, expect: 0x%x\n", dpu_checksum, theoretical_checksum);
        }
    }

    ns = get_ns();
    DPU_ASSERT(dpu_free(dpu_set));
    printf("free dpu %d DPUs: %llu ns\n", nr_of_dpus, get_ns() - ns);
    return status ? 0 : -1;
}
