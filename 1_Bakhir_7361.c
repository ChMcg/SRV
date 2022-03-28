/*
 * Выполнил: Андрей Бахир группа 7361
 * Задание: №1 Поставщик - потребитель
 * Дата выполнения: 27.02.2022
 * Версия: 0.1
 *
 * Скрипт для компиляции и запуска программы:
 *      <компиляция>:    
 *      <для варианта 1>: gcc -pthread -DTASK=1 -DTIME_DELTA=0 3_Bakhir_7361.c -o 3_Bakhir_7361
 *      <для варианта 2>: gcc -pthread -DTASK=2 -DTIME_DELTA=0 3_Bakhir_7361.c -o 3_Bakhir_7361
 *      <запуск>: ./3_Bakhir_7361
 * 
 * Параметры компиляции:
 *      можно менять параметры компиляции для 
 *      различного поведения программы:
 *      <выбор задания> 
 *          '-DTASK=1' скомпилирует программу на выполнение кода, согласно заданию 1
 *          '-DTASK=2' соответственно на выполнение кода, согласно заданию 2
 *      <скорость продюсера> 
 *          Изначально и продюсер и консюмер имеют равную задержку в одну секунду.
 *          Для проверки граничных случаев можно замедлить ("-DTIME_DELTA=+300") скорость
 *          работы продюсера, или ускорить ("-DTIME_DELTA=-300").
 */
// ------------------------------------------- // 
/*
 * Общее описание программы:
 * Задание 1:
 *      Продюсер кладёт значение в буфер, пока 
 *      семафор Shared.not_empty не остановит выполнение.
 *      Консюмер читает значения из буфера пока 
 *      доступен семафор Shared.has_values
 * Задание 2:
 *      Продюсер также кладёт значения в буфер, но уже сам
 *      отслеживает заполненность буфера через условную переменную
 *      buffer_full. Если буфер заполнен, ожидает сигнал через
 *      wait_for_space.
 *      Консюмер проверяет buffer_empty, и если там true, ожидает 
 *      сигнал на то что там что-то появилось. После того как прочитывает
 *      ставит buffer_full в false и пробуждает всех кто ждёт 
 *      сигнал wait_for_space.
 */
// ------------------------------------------- //

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>


// =============================== Задание 1 ===============================

#ifndef TASK
#define TASK 1
#endif

#ifndef TIME_DELTA
#define TIME_DELTA 0
#endif

#define TIME_SLEEP 1000 * 1000
#define TIME_SLEEP_WITH_DELTA TIME_SLEEP + (TIME_DELTA * 1000)
#define TIME_STARTUP_SLEEP 5

struct
{
    int buffer;
    sem_t mutex, not_empty, has_values;
} Shared;

void* produce_t1(void* arg)
{
    int tmp = 0;
    bool keepAlive = true;
    while (keepAlive)
    {
        sem_wait(&Shared.not_empty);
        sem_wait(&Shared.mutex);
        tmp = ++Shared.buffer;
        sem_post(&Shared.mutex);
        sem_post(&Shared.has_values);
        printf("produced: %d\n", tmp);
        usleep(TIME_SLEEP_WITH_DELTA);
    }
    return NULL;
}

void* consume_t1(void* args)
{
    sleep(TIME_STARTUP_SLEEP);
    int tmp = 0;
    bool keepAlive = true;
    while (keepAlive)
    {
        sem_wait(&Shared.has_values);
        sem_wait(&Shared.mutex);
        tmp = --Shared.buffer;
        sem_post(&Shared.mutex);
        sem_post(&Shared.not_empty);
        printf("consumed: %d\n", tmp);
        usleep(TIME_SLEEP);
    }
    return NULL;
}

int main_task_1()
{
    pthread_t producer_id, consumer_id;
    Shared.buffer = 0;

    sem_init(&Shared.mutex, 0, 1);
    sem_init(&Shared.not_empty, 0, 10);
    sem_init(&Shared.has_values, 0, 0);

    pthread_create(&producer_id, NULL, produce_t1, NULL);
    pthread_create(&consumer_id, NULL, consume_t1, NULL);

    pthread_join(producer_id, NULL);
    pthread_join(consumer_id, NULL);

    sem_destroy(&Shared.mutex);
    sem_destroy(&Shared.not_empty);
    sem_destroy(&Shared.has_values);

    return EXIT_SUCCESS;
}


// =============================== Задание 2 ===============================

int buffer = 0;
bool buffer_full = false;
bool buffer_empty = true;

pthread_cond_t wait_for_values;
pthread_cond_t wait_for_space;
pthread_mutex_t mutex;

void* produce_t2(void* arg)
{
    int tmp = 0;
    bool keepAlive = true;
    while (keepAlive)
    {
        pthread_mutex_lock(&mutex);
        if (buffer_full) pthread_cond_wait(&wait_for_space, &mutex);
        tmp = ++buffer;
        printf("produced: %d\n", tmp);
        if (tmp >= 10) buffer_full = true;
        buffer_empty = false;
        pthread_cond_signal(&wait_for_values);
        pthread_mutex_unlock(&mutex);
        usleep(TIME_SLEEP_WITH_DELTA);
    }
    return NULL;
}

void* consume_t2(void* args)
{
    sleep(TIME_STARTUP_SLEEP);
    int tmp = 0;
    bool keepAlive = true;
    while (keepAlive)
    {
        pthread_mutex_lock(&mutex);
        if (buffer_empty) pthread_cond_wait(&wait_for_values, &mutex);
        tmp = --buffer;
        printf("consumed: %d\n", tmp);
        if (tmp <= 0) buffer_empty = true;
        buffer_full = false;
        pthread_cond_signal(&wait_for_space);
        pthread_mutex_unlock(&mutex);
        usleep(TIME_SLEEP);
    }
    return NULL;
}


int main_task_2()
{
    pthread_t consume_thr, produce_thr;

    pthread_cond_init(&wait_for_values, NULL);
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&consume_thr, NULL, &consume_t2, NULL);
    pthread_create(&produce_thr, NULL, &produce_t2, NULL);

    pthread_join(consume_thr, NULL);
    pthread_join(produce_thr, NULL);

    return EXIT_SUCCESS;
}


int main(int argc, char* argv[])
{
    int task_id = TASK;
    switch (task_id)
    {
        case 1:     return main_task_1();
        case 2:     return main_task_2();
        default:    
            printf(
                "Unknown task id: %d.\n"
                "Try to recompile with correct TASK define"
                " ('-DTASK=1' for example)\n", task_id); 
            break;
    }

    return EXIT_SUCCESS;
}

/**
 * Лог выполнения идентичен для обоих заданий, поэтому привожу только один.
 * 
 * Продюсер и консюмер равны по сокрости:
 * $ gcc -pthread -DTASK=1 3_Bakhir_7361.c -o build/3_Bakhir_7361 && ./build/3_Bakhir_7361
 * produced: 1
 * produced: 2
 * produced: 3
 * produced: 4
 * produced: 5
 * consumed: 4
 * produced: 5
 * consumed: 4
 * produced: 5
 * consumed: 4
 * produced: 5
 * ^C
 * 
 * Продюсер быстрее:
 * $ gcc -pthread -DTIME_DELTA=-300 -DTASK=1 3_Bakhir_7361.c -o build/3_Bakhir_7361 && ./build/3_Bakhir_7361
 * produced: 1
 * produced: 2
 * produced: 3
 * produced: 4
 * produced: 5
 * produced: 6
 * produced: 7
 * produced: 8
 * consumed: 7
 * produced: 8
 * consumed: 7
 * produced: 8
 * consumed: 7
 * produced: 8
 * produced: 9
 * consumed: 8
 * produced: 9
 * consumed: 8
 * produced: 9
 * produced: 10
 * consumed: 9
 * produced: 10
 * consumed: 9
 * produced: 10
 * consumed: 9
 * produced: 10
 * consumed: 9
 * produced: 10
 * consumed: 9
 * produced: 10
 * consumed: 9
 * produced: 10
 * ^C
 * 
 * Консюмер быстрее:
 * $ gcc -pthread -DTIME_DELTA=+1300 -DTASK=1 3_Bakhir_7361.c -o build/3_Bakhir_7361 && ./build/3_Bakhir_7361
 * produced: 1
 * produced: 2
 * produced: 3
 * consumed: 2
 * consumed: 1
 * produced: 2
 * consumed: 1
 * consumed: 0
 * produced: 1
 * consumed: 0
 * produced: 1
 * consumed: 0
 * ^C
 * 
 */
