#include "tests.h"

int run_tests(){
    CU_initialize_registry();
    CU_pSuite tests = CU_add_suite("MEMORIA SUITE",NULL,NULL);
    CU_add_test(tests,"Agregar Entradas en la TLB", add_new_tlb_instance);
    CU_add_test(tests, "Llenar TLB", full_tlb);
    //CU_add_test(tests, "Reemplazo TLB", replace_tlb);
    CU_add_test(tests, "Obtener Entrada de TLB (Condicion pagina)", fetch_instance_by_page);
    CU_add_test(tests, "FAIL Intentar obtener Entrada de TLB (Condicion pagina)", not_fetch_instance_by_page);
    CU_add_test(tests, "Obtener Entrada de TLB (Condicion PID)", fetch_instance_by_pid);
    CU_add_test(tests, "FAIL Intentar obtener Entrada de TLB (Condicion PID)", not_fetch_instance_by_pid);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

t_log* create_log_test(){
    return log_create("./cfg/memoria-test.log", "MEMORIA-TEST", false, LOG_LEVEL_DEBUG);
}

void add_new_tlb_instance(){
    int cant_tlb = 2;
    logger = create_log_test();
    config = config_create(CONFIG_PATH);
    initPaginacion();

    add_entrada_tlb(1, 1, 1);
    add_entrada_tlb(1, 2, 2);

    CU_ASSERT_EQUAL(list_size(tlb_list), cant_tlb);
}

void full_tlb(){
    logger = create_log_test();
    config = config_create(CONFIG_PATH);
    initPaginacion();
    int pid = 0, page = 1, frame = 1;

    for (int i = 0; i < max_entradas_tlb; i++) {
        add_entrada_tlb(pid, page, frame);
        pid++;
        page++;
        frame++;
    }

    CU_ASSERT_EQUAL(list_size(tlb_list), max_entradas_tlb);
}

void replace_tlb(){
    logger = create_log_test();
    config = config_create(CONFIG_PATH);

    initPaginacion();
    int pid = 0, page = 1, frame = 1;

    for (int i = 0; i < max_entradas_tlb; i++) {
        add_entrada_tlb(pid, page, frame);
        pid++;
        page++;
        frame++;
    }

    CU_ASSERT_EQUAL(list_size(tlb_list), max_entradas_tlb);
}

void fetch_instance_by_page(){
    int exp_page = 1;
    logger = create_log_test();
    config = config_create(CONFIG_PATH);

    initPaginacion();
    int pid = 0, page = 1, frame = 1;

    for (int i = 0; i < max_entradas_tlb; i++) {
        add_entrada_tlb(pid, page, frame);
        pid++;
        page++;
        frame++;
    }

    TLB* tlb = fetch_entrada_tlb_by_page(exp_page);

    CU_ASSERT_PTR_NOT_NULL(tlb);
    CU_ASSERT_EQUAL(tlb->pagina, exp_page);
}

void not_fetch_instance_by_page(){
    logger = create_log_test();
    config = config_create(CONFIG_PATH);

    initPaginacion();
    int pid = 0, page = 1, frame = 1;

    for (int i = 0; i < max_entradas_tlb; i++) {
        add_entrada_tlb(pid, page, frame);
        pid++;
        page++;
        frame++;
    }

    TLB* tlb = fetch_entrada_tlb_by_page(UINT32_MAX);

    CU_ASSERT_PTR_NULL(tlb);
}


void fetch_instance_by_pid(){
    int exp_pid = 1;
    logger = create_log_test();
    config = config_create(CONFIG_PATH);

    initPaginacion();
    int pid = 0, page = 1, frame = 1;

    for (int i = 0; i < max_entradas_tlb; i++) {
        add_entrada_tlb(pid, page, frame);
        pid++;
        page++;
        frame++;
    }

    TLB* tlb = fetch_entrada_tlb_by_pid(exp_pid);

    CU_ASSERT_PTR_NOT_NULL(tlb);
    CU_ASSERT_EQUAL(tlb->pid, exp_pid);
}

void not_fetch_instance_by_pid(){
    logger = create_log_test();
    config = config_create(CONFIG_PATH);

    initPaginacion();
    int pid = 0, page = 1, frame = 1;

    for (int i = 0; i < max_entradas_tlb; i++) {
        add_entrada_tlb(pid, page, frame);
        pid++;
        page++;
        frame++;
    }

    TLB* tlb = fetch_entrada_tlb_by_pid(UINT32_MAX);

    CU_ASSERT_PTR_NULL(tlb);
}