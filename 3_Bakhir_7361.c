/*
 * Выполнил: Андрей Бахир группа 7361
 * Задание: №3 Обеспечение таймаута
 * Дата выполнения: 25.04.2022
 * Версия: 0.1
 *
 * Скрипт для компиляции и запуска программы:
 *      <компиляция>:    
 *      <без параметров>: gcc -pthread 3_Bakhir_7361.c -o 3_Bakhir_7361
 *      <с параметрами>: gcc -DALARM_TIME=5 -pthread 3_Bakhir_7361.c -o 3_Bakhir_7361
 *      <запуск>: ./3_Bakhir_7361
 */
// ------------------------------------------- // 
/*
 * Общее описание программы:
 *      Эмулируется работа системы с отслеживанием времени 
 *      её работы. В случае превышения времени выполнения
 *      выводится сообщение "deadline exceeded". В случае
 *      когда происходит эмуляция зависания программа обрабатывает
 *      SIGALRM и выводит сообщение о том, что программу нужно 
 *      перезапустить.
 */
// ------------------------------------------- //

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>


/**
 * Сигнал о превышении времени выполнения
 */
#ifndef DEADLINE 
#define DEADLINE 48
#endif

/**
 * Время до аварийного завершения
 */
#ifndef ALARM_TIME
#define ALARM_TIME 5
#endif


void do_control();
void deadline_handler(int signo, siginfo_t* info, void* nk);
void alarm_handler(int signo, siginfo_t* info, void* nk);
long int retreive_time_delta_ns(struct timespec* start, struct timespec* end);
long int do_control_and_measure_time();
void initialize_signal(int signumber, void(*handler)(int, siginfo_t *, void *));
void* thread_function(void* args);

int deadlock = 1;


int main()
{
    pthread_t thread;

    pthread_create(&thread, NULL, &thread_function, NULL);
    pthread_join(thread, NULL);

    return EXIT_SUCCESS;
}


void* thread_function(void* args)
{
    long int deadline = 0.5 * 1000000000 * 1.04;
    long int work_time;

    printf("########## Deadline time: %lf miliseconds\n", (double)deadline / (double)1000000);

    /**
     * Инициализация генератора случайных чисел 
     * и заполнение маски и структур sigaction
     */
    srand(time(NULL));
    initialize_signal(DEADLINE, &deadline_handler);
    initialize_signal(SIGALRM,  &alarm_handler);

    while (1)
    {
        alarm(0);
        alarm(ALARM_TIME);

        work_time = do_control_and_measure_time();
        if (work_time > deadline) raise(DEADLINE);

        sleep(3);
    }
}

/*struct sigaction*/ void initialize_signal(int signumber, void(*handler)(int, siginfo_t *, void *))
{
    sigset_t set;
    struct sigaction action;
    sigaddset(&set, signumber);
    action.sa_flags = SA_SIGINFO;
    action.sa_mask = set;
    action.sa_sigaction = handler;
    sigaction(signumber, &action, NULL);
}

long int do_control_and_measure_time()
{
    struct timespec start, end;

    clock_gettime(CLOCK_REALTIME, &start);
    do_control();
    clock_gettime(CLOCK_REALTIME, &end);

    return retreive_time_delta_ns(&start, &end);
}

void do_control()
{
    long int work_time;
    long int delta = rand() % 80000 - 30000;
    work_time = 500000 + delta;

    printf("########## Working for %lf miliseconds \n", (double)work_time / (double)1000);

    // Эмуляция зависания
    if (deadlock == 0) 
    {
        write(0, "####   Deadlock occured    ####\n", 33);
        while(1);
    }

    deadlock = rand() % 6;
    
    // Эмуляция работы
    usleep(work_time);
}

long int retreive_time_delta_ns(struct timespec* start, struct timespec* end)
{
    long int delta_seconds;
    long int delta_nanoseconds;

    delta_seconds     = (long int)(end->tv_sec - start->tv_sec);
    delta_nanoseconds = (long int)(end->tv_nsec - start->tv_nsec);

    return delta_seconds * 1000000000 + delta_nanoseconds;
}

void deadline_handler(int signo, siginfo_t* info, void* nk)
{
    printf("####   Deadline exceeded   ####\n");
}

void alarm_handler(int signo, siginfo_t* info, void* nk)
{
    printf("#####-- Restart requered! --#####\n");
    raise(SIGUSR1);
}




/**
 * ########## Deadline time: 520.000000 miliseconds
 * ########## Working for 477.972000 miliseconds 
 * ########## Working for 487.321000 miliseconds 
 * ########## Working for 507.282000 miliseconds 
 * ########## Working for 548.200000 miliseconds 
 * ####   Deadline exceeded   ####
 * ########## Working for 486.921000 miliseconds 
 * ########## Working for 480.257000 miliseconds 
 * ########## Working for 537.480000 miliseconds 
 * ####   Deadline exceeded   ####
 * ########## Working for 491.953000 miliseconds 
 * ########## Working for 523.309000 miliseconds 
 * ####   Deadlock occured    ####
 * #####-- Restart requered! --#####
 * [1]    67500 user-defined signal 1  ./build/3_Bakhir_7361
 * 
 */