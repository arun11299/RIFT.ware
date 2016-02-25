
/*
 * Copyright 2006 Tail-F Systems AB
 * Tail-F customers are permitted to redistribute in binary form,
 * with or without modification, for use in customer products.
 */

#ifndef _MAIN_H
#define _MAIN_H 1

#include "ns_prefix.h"

struct zconfd_data;

typedef int thread_read_func_t(void *thread_data, int sock);
typedef int thread_timer_func_t(void *thread_data );


void* thread_add_read(thread_read_func_t* func, void *thread_data, int sock);
void* thread_add_timer(thread_timer_func_t* func, void *thread_data, int timer);
void thread_cancel(void* thread);


/**
 * Framework initialization. Must be implemented elsewhere.
 */
extern void framework_init();

#endif
