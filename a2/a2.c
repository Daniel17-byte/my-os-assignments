#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <semaphore.h>
#include <pthread.h>

sem_t P4_sem;
sem_t P4_14_sem;
sem_t T7_4_sem;
sem_t T8_1_sem;
sem_t T7_1_sem;
sem_t T8_5_sem;

int running_threads = 0;
pthread_mutex_t running_threads_mutex;

void *P8_thread_function(void *arg) {
    int process_no = *((int *)arg);
    int thread_no = *((int *)arg + 1);

    if (thread_no == 1) {
        sem_wait(&T8_1_sem);
    } else if (thread_no == 5) {
        sem_wait(&T7_4_sem);
    }

    info(BEGIN, process_no, thread_no);

    sem_post(&T8_1_sem);

    usleep(100000);

    info(END, process_no, thread_no);

    if (thread_no == 1) {
        sem_post(&T7_4_sem);
    }

    return NULL;
}

void create_P8_threads() {
    pthread_t threads[5];
    int process_and_thread_no[][2] = {
            {8, 1},
            {8, 2},
            {8, 3},
            {8, 4},
            {8, 5},
    };

    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, P8_thread_function, process_and_thread_no[i]);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
}

void *P4_thread_function(void *arg) {
    int process_no = *((int *)arg);
    int thread_no = *((int *)arg + 1);

    sem_wait(&P4_sem);

    pthread_mutex_lock(&running_threads_mutex);
    running_threads++;
    pthread_mutex_unlock(&running_threads_mutex);

    info(BEGIN, process_no, thread_no);

    if (thread_no == 14) {
        while (running_threads != 5) {
            usleep(1000);
        }
    } else {
        usleep(100000);
    }

    info(END, process_no, thread_no);

    pthread_mutex_lock(&running_threads_mutex);
    running_threads--;
    pthread_mutex_unlock(&running_threads_mutex);

    sem_post(&P4_sem);

    return NULL;
}

void create_P4_threads() {
    pthread_t threads[36];
    int process_and_thread_no[][2] = {
            {4, 1},
            {4, 2},
            {4, 3},
            {4, 4},
            {4, 5},
            {4, 6},
            {4, 7},
            {4, 8},
            {4, 9},
            {4, 10},
            {4, 11},
            {4, 12},
            {4, 13},
            {4, 14},
            {4, 15},
            {4, 16},
            {4, 17},
            {4, 18},
            {4, 19},
            {4, 20},
            {4, 21},
            {4, 22},
            {4, 23},
            {4, 24},
            {4, 25},
            {4, 26},
            {4, 27},
            {4, 28},
            {4, 29},
            {4, 30},
            {4, 31},
            {4, 32},
            {4, 33},
            {4, 34},
            {4, 35},
            {4, 36}
    };

    for (int i = 0; i < 36; i++) {
        pthread_create(&threads[i], NULL, P4_thread_function, process_and_thread_no[i]);
    }

    for (int i = 0; i < 36; i++) {
        pthread_join(threads[i], NULL);
    }
}

void *P7_thread_function(void *arg) {
    int process_no = *((int *)arg);
    int thread_no = *((int *)arg + 1);

    info(BEGIN, process_no, thread_no);

    if (thread_no == 1) {
        usleep(100000);
        sem_post(&T7_1_sem);
    } else if (thread_no == 2) {
        sem_wait(&T7_1_sem);
        usleep(200000);
    }

    info(END, process_no, thread_no);

    return NULL;
}

void create_P7_threads() {
    pthread_t threads[5];
    int process_and_thread_no[][2] = {
            {7, 1},
            {7, 2},
            {7, 3},
            {7, 4},
            {7, 5},
    };

    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, P7_thread_function, process_and_thread_no[i]);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main() {
    init();

    sem_init(&P4_sem, 0, 5);
    sem_init(&P4_14_sem, 0, 0);
    sem_init(&T8_1_sem, 0, 0);
    sem_init(&T7_4_sem, 0, 0);
    sem_init(&T7_1_sem, 0, 0);
    sem_init(&T8_5_sem, 0, 0);

    pthread_mutex_init(&running_threads_mutex, NULL);

    info(BEGIN, 1, 0);

    if (fork() == 0) {
        info(BEGIN, 2, 0);

        if (fork() == 0) {
            info(BEGIN, 5, 0);
            if (fork() == 0) {
                info(BEGIN, 6, 0);
                info(END, 6, 0);
                return 0;
            }
            wait(NULL);
            info(END, 5, 0);
            return 0;
        }

        if (fork() == 0) {
            info(BEGIN, 7, 0);
            create_P7_threads();
            info(END, 7, 0);
            return 0;
        }

        wait(NULL);
        wait(NULL);
        info(END, 2, 0);
        return 0;
    }

    if (fork() == 0) {
        info(BEGIN, 3, 0);
        if (fork() == 0) {
            info(BEGIN, 4, 0);
            create_P4_threads();
            if (fork() == 0) {
                info(BEGIN, 9, 0);
                info(END, 9, 0);
                return 0;
            }
            wait(NULL);
            info(END, 4, 0);
            return 0;
        }
        wait(NULL);
        info(END, 3, 0);
        return 0;
    }

    if (fork() == 0) {
        info(BEGIN, 8, 0);
        create_P8_threads();
        info(END, 8, 0);
        return 0;
    }

    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    sem_destroy(&P4_sem);
    sem_destroy(&P4_14_sem);
    sem_destroy(&T8_1_sem);
    sem_destroy(&T7_4_sem);
    sem_destroy(&T7_1_sem);
    sem_destroy(&T8_5_sem);

    pthread_mutex_destroy(&running_threads_mutex);

    info(END, 1, 0);
    return 0;
}