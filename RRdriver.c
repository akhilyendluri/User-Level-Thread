/* Pet Thread Library test driver
 *  (c) 2017, Jack Lange <jacklange@cs.pitt.edu>
 *  Akhil Yendluri
 *  Krithika Ganesh
 */

#include <stdio.h>
#include <stdlib.h>

#include "pet_thread.h"
#include "pet_log.h"

#define TC 32 // divisible by 4
pet_thread_id_t test_thread1[TC];

/*
 * First 25% of threads run in RR scheduling
 * Second 25% of threads yield to next immediate thread
 * Third 25% of threads join to thread which is 2 positions ahead
 * last 25% of threads exit
 */
void *
test_func1(void * arg) {
    int index = arg;
    if (index <= 0.25 * TC) {
        printf("%d : Running \n", test_thread1[index]);
    } else if (index <= 0.5 * TC) {
        printf("%d : Yeild to : %d\n", test_thread1[index], test_thread1[index + 1]);
        pet_thread_yield_to(test_thread1[index + 1]);
        printf("%d : Back from yield \n", test_thread1[index]);

    } else if (index <= 0.75 * TC) {
        printf("%d : Join to : %d\n", test_thread1[index], test_thread1[index + 2]);
        pet_thread_join(test_thread1[index + 2], NULL);
        printf("%d : Back from join \n", test_thread1[index]);

    } else {
        printf("%d : Exiting \n", test_thread1[index]);
        pet_thread_exit(NULL);
    }

    return NULL;
}

int main(int argc, char ** argv)
{

    int ret = 0;
    ret = pet_thread_init();
    if (ret == -1) {
        ERROR("Could not initialize Pet Thread Library\n");
        return -1;
    }
    printf("Testing Pet Thread Library\n");

    for(long i = 1; i <= TC; i++) {
        ret = pet_thread_create(&test_thread1[i], test_func1, (void *)i);
        if (ret == -1) {
            ERROR("Could not create test_thread1\n");
            return -1;
        }
    }

    ret = pet_thread_run();

    if (ret == -1) {
        ERROR("Error encountered while running pet threads (ret=%d)\n", ret);
        return -1;
    }
    
    return 0;

}
