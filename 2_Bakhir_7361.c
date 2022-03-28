/*
 * Выполнил: Андрей Бахир группа 7361
 * Задание: №2 Вызов лифта
 * Дата выполнения: 27.03.2022
 * Версия: 0.1
 *
 * Скрипт для компиляции и запуска программы:
 *      <компиляция>:    
 *      <для варианта 1>: gcc -pthread 2_Bakhir_7361.c -o 2_Bakhir_7361
 *      <запуск>: ./2_Bakhir_7361
 */
// ------------------------------------------- // 
/*
 * Общее описание программы:
 * Задание 1:
 * Лифт может находиться в трёх состояниях. Если он 
 * находится на нашем этаже, то двери открываются и
 * лифт едет в нужную сторону. Если нет, сначала нужно 
 * дождаться пока лифт доедет на наш этаж, затем двери 
 * откроются и лифт поедет в нужнуь сторону.
 * 
 * Взаимодействие происходит в виде управления автома-
 * том лифта из трёх состояний. Передача управления 
 * автомату происходит посредством отправки соответствующих 
 * сигналов и их обработке в функции lift_contol
 */
// ------------------------------------------- //

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>


#define    TOP          0  // лифт вверху
#define    BOTTOM       1  // лифт внизу
#define    THIS_FLOOR   2  // лифт на нашем этаже

#define    Up  	48
#define    Down	49

int state = THIS_FLOOR;
bool prompt_printed = false;

void* push_button(void* args);
void move(int sig);
void lift_control(int signo);
void print_prompt();


int main() 
{
    pthread_t t;
    printf("Start\n");
    printf("Avaliable commands: u - Up, d - Down, q - Exit\n");

    signal(Up, &lift_control);
    signal(Down, &lift_control);

    pthread_create(&t, NULL, &push_button, NULL);
    pthread_join(t, NULL);

    printf("Finish\n");
    return EXIT_SUCCESS;
}


void* push_button(void* args) 
{
    char ch;

    while(ch != 'q')
    {
        print_prompt();
        ch = getchar();
        switch(ch){
            case 'u': raise(Up); break;
            case 'd': raise(Down); break;
            case 'q': return EXIT_SUCCESS;
        };
    }
   return EXIT_SUCCESS;
}

void move(int sig)
{
    int i;
    switch (sig)
    {
    case Up:
        for(i = 1; i <= 3; i++)
        {
            printf("*** Going Up\n");
            usleep(500000);
        }
        state = TOP;
        break;

    case Down:
        for(i = 1; i <= 3; i++)
        {
            printf("*** Going Down\n");
            usleep(500000);
        }
        state = BOTTOM;
        break;

    default:
        printf("Unknown signal received: %d", sig);
        break;
    }
}

void lift_control(int signo)
{
    switch(state) {              
        case THIS_FLOOR:
            if(signo == Up) 
            {
                printf("This Floor, door is opened \n");
                sleep(1);
                move(Up);
                state = TOP;
            }
            if(signo == Down) 
            {
                printf("This Floor, door is opened \n");
                sleep(1);
                move(Down);
                state = TOP;
            }
        break;
        case TOP:
            if(signo == Up) 
            {
                move(Down);
                printf("This Floor, door is opened \n");
                sleep(1);
                move(Up);
                state = TOP;
            }
            if(signo == Down) 
            {
                move(Down);
                printf("This Floor, door is opened \n");
                sleep(1);
                move(Down);
                state = BOTTOM;
            }

        break;
        case BOTTOM:
            if(signo == Up) 
            {
                move(Up);
                printf("This Floor, door is opened \n");
                sleep(1);
                move(Up);
                state = TOP;
            }
            if(signo == Down) 
            {
                move(Up);
                printf("This Floor, door is opened \n");
                sleep(1);
                move(Down);
                state = BOTTOM;
            }

        break;
        }
    return;
}

void print_prompt()
{
    if (!prompt_printed)
    {
        printf(">>> ");
        prompt_printed = true;
    }
    else 
    {
        prompt_printed = false;
    }
}



/**
 * Start
 * Avaliable commands: u - Up, d - Down, q - Exit
 * >>> u
 * This Floor, door is opened 
 * *** Going Up
 * *** Going Up
 * *** Going Up
 * >>> u
 * *** Going Down
 * *** Going Down
 * *** Going Down
 * This Floor, door is opened 
 * *** Going Up
 * *** Going Up
 * *** Going Up
 * >>> d
 * *** Going Down
 * *** Going Down
 * *** Going Down
 * This Floor, door is opened 
 * *** Going Down
 * *** Going Down
 * *** Going Down
 * >>> d
 * *** Going Up
 * *** Going Up
 * *** Going Up
 * This Floor, door is opened 
 * *** Going Down
 * *** Going Down
 * *** Going Down
 * >>> u
 * *** Going Up
 * *** Going Up
 * *** Going Up
 * This Floor, door is opened 
 * *** Going Up
 * *** Going Up
 * *** Going Up
 * >>> q
 * Finish
 */