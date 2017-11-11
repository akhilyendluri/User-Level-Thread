/* Pet Thread Library
 *  (c) 2017, Jack Lange <jacklange@cs.pitt.edu>
 */

#ifndef __PET_THREAD_H__
#define __PET_THREAD_H__

#define _GNU_SOURCE

#include <stdint.h>



#define PET_INVALID_THREAD_ID (~0x0ULL)
#define PET_MASTER_THREAD_ID  (0x0ULL)

typedef uintptr_t pet_thread_id_t;

typedef void *(*pet_thread_fn_t)(void * arg);


int pet_thread_create(pet_thread_id_t * thread, pet_thread_fn_t func, void * arg);

int  pet_thread_join(pet_thread_id_t thread_id, void ** ret_val);
void pet_thread_exit(void * ret_val);

int pet_thread_yield_to(pet_thread_id_t thread_id);
int pet_thread_schedule( void );

int pet_thread_init( void );
int pet_thread_run ( void );


#endif
