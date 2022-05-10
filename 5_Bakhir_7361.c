/*
 * Выполнил: Андрей Бахир группа 7361
 * Задание: №5 Модель циклической системы управления
 * Дата выполнения: 10.05.2022
 * Версия: 0.1
 *
 * Скрипт для компиляции и запуска программы:
 *      <компиляция>: gcc -lrt 5_Bakhir_7361.c -o 5_Bakhir_7361
 *      <запуск>: ./5_Bakhir_7361
 */
// ------------------------------------------- // 
/*
 * Общее описание программы:
 *      Три активных таймера запускают процедуры 
 *      каждые 500, 750 и 1000 миллисекунд с помощью
 *      сигналов. Процедуры в своб очередь определяют
 *      время, прошедшее с предыдущего запуска и выводят
 *      в терминал текуще время и время с последнего
 *      вызова
 */
// ------------------------------------------- //



#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>


char* now();
int resolve_delta_and_update_call_time(int procedure_number);
int sec_to_ns(float seconds);
timer_t* setup_timer(sigset_t* set, int signumber, void(*handler)(int));
void default_control(int procedure_number);
void fill_timerspec(struct itimerspec* spec, long int time_ns, long int interval_time_ns);

void doControl_1(int signumber);
void doControl_2(int signumber);
void doControl_3(int signumber);


#define SIGACT1 10
#define SIGACT2 11
#define SIGACT3 12

struct timespec global_time[3]; 


int main()
{
    timer_t* timer_id[3];
    struct itimerspec itime[3];
    float interval_time_s;
    int i;

    sigset_t set;
    sigemptyset(&set);

    timer_id[0] = setup_timer(&set, SIGACT1, doControl_1);
    timer_id[1] = setup_timer(&set, SIGACT2, doControl_2);
    timer_id[2] = setup_timer(&set, SIGACT3, doControl_3);
    
    for (i = 0; i < 3; i++)
    {
        interval_time_s = 0.5 + 0.25*i;
        fill_timerspec(&(itime[i]), 1, sec_to_ns(interval_time_s));

        clock_gettime(CLOCK_REALTIME, &global_time[i]);
        timer_settime(*(timer_id[i]), 0, &itime[i], NULL);
    }

    for (;;) {}

    return 0;
}

timer_t* setup_timer(sigset_t* set, int signumber, void(*handler)(int))
{
    struct sigevent event;
    struct sigaction act;
    timer_t* timer;
    timer = malloc(sizeof(timer_t));

    sigaddset(set, signumber);
    act.sa_handler = handler;
    act.sa_flags = 0;
    act.sa_mask = *set;
    sigaction(signumber, &act, NULL);
    event.sigev_notify = SIGEV_SIGNAL;
    event.sigev_signo = signumber;
    timer_create(CLOCK_REALTIME, &event, timer);

    return timer;
}

void doControl_1(int signumber)
{
    default_control(0);
}

void doControl_2(int signumber)
{
    default_control(1);
}

void doControl_3(int signumber)
{
    default_control(2);
}

void default_control(int procedure_number)
{
    long int delta = resolve_delta_and_update_call_time(procedure_number);
    printf("%s\t| [Procedure %d] Procedure called. Period from last call: %ld ms\n", now(), procedure_number+1, delta);
}

void fill_timerspec(struct itimerspec* spec, long int time_ns, long int interval_time_ns)
{
    int value_sec = time_ns / 1000000000;
    int value_nsec = time_ns % 1000000000;
    int i_value_sec = interval_time_ns / 1000000000;
    int i_value_nsec = interval_time_ns % 1000000000;

    spec->it_value.tv_sec = value_sec;
    spec->it_value.tv_nsec = value_nsec;
    spec->it_interval.tv_sec = i_value_sec;
    spec->it_interval.tv_nsec = i_value_nsec;
}

int sec_to_ns(float seconds)
{
    return (int)(seconds * 1000000000);
}

char* now()
{
	time_t time_struct = time(NULL);
    struct timeval tv;
    int maxlen = 50;
    int len = 0;
    int current_msec;
    char* buff = calloc(sizeof(char), maxlen);

    gettimeofday(&tv, NULL);
    current_msec = tv.tv_usec;

    len = strftime(buff, maxlen, "%H:%M:%S", localtime(&time_struct));
    buff[len] = '\0';
    len += sprintf(buff + len, ".%06d", current_msec);

    return buff;
}

int resolve_delta_and_update_call_time(int procedure_number)
{
    struct timespec time;
    long int delta = 0;
    long long int cl;
    clock_gettime(CLOCK_REALTIME, &time);
    cl = (long int)(time.tv_sec) * 1000 + (long int)((time.tv_nsec) / 1000000);
    delta = (long int)(time.tv_sec - global_time[procedure_number].tv_sec) * 1000 + (long int)((time.tv_nsec - global_time[procedure_number].tv_nsec) / 1000000);
    global_time[procedure_number] = time;
    return delta;
}


/**
 * 
 * $ gcc -Wno-incompatible-pointer-types -pthread -lrt 5_Bakhir_7361.c -o build/5_Bakhir_7361 && ./build/5_Bakhir_7361
 * 17:53:01.417073	| [Procedure 2] Procedure called. Period from last call: 0 ms
 * 17:53:01.417133	| [Procedure 1] Procedure called. Period from last call: 0 ms
 * 17:53:01.417147	| [Procedure 3] Procedure called. Period from last call: 0 ms
 * 17:53:01.917071	| [Procedure 1] Procedure called. Period from last call: 499 ms
 * 17:53:02.167072	| [Procedure 2] Procedure called. Period from last call: 751 ms
 * 17:53:02.417070	| [Procedure 1] Procedure called. Period from last call: 500 ms
 * 17:53:02.417146	| [Procedure 3] Procedure called. Period from last call: 1000 ms
 * 17:53:02.917071	| [Procedure 2] Procedure called. Period from last call: 749 ms
 * 17:53:02.917113	| [Procedure 1] Procedure called. Period from last call: 500 ms
 * 17:53:03.417071	| [Procedure 1] Procedure called. Period from last call: 500 ms
 * 17:53:03.417145	| [Procedure 3] Procedure called. Period from last call: 1000 ms
 * 17:53:03.667072	| [Procedure 2] Procedure called. Period from last call: 750 ms
 * 17:53:03.917070	| [Procedure 1] Procedure called. Period from last call: 499 ms
 * 17:53:04.417073	| [Procedure 2] Procedure called. Period from last call: 751 ms
 * 17:53:04.417098	| [Procedure 1] Procedure called. Period from last call: 501 ms
 * 17:53:04.417145	| [Procedure 3] Procedure called. Period from last call: 1000 ms
 * ^C
 */