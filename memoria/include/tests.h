#ifndef TESTS_H
#define TESTS_H

#include <CUnit/Basic.h>
#include "main.h"

int run_tests();
t_log* create_log_test();
bool sort_by_max_lru(void* actual, void* next);
void add_new_tlb_instance();
void full_tlb();
void replace_tlb_fifo();
void replace_tlb_lru();
void fetch_instance();
void not_fetch_instance();

#endif