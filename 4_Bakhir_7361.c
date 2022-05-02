#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>


#define LM pthread_mutex_lock(&m_print);
#define UM pthread_mutex_unlock(&m_print);


pthread_cond_t cond_container_arrived;
pthread_mutex_t m_wait_for_container;
pthread_mutex_t m_print;
pthread_mutex_t m_queue;

pthread_cond_t container_ended_loadout;
pthread_mutex_t m_container_ended_loadout;

int T = 5; // Период появления контейнера
int loadout_time = 2; // время разгрузки



struct queue_node
{
    int container_id;
    pthread_cond_t* cond_var_allow_unload;
    pthread_mutex_t* m_cond;
    struct queue_node* next;
};

struct queue_node* q_head = NULL;

int pop_container_id_from_queue();
struct queue_node* pop_queue_node();
void put_container_id_in_queue(
        int container_id, 
        pthread_cond_t* cond_var_allow_unload, 
        pthread_mutex_t* m_cond
    );
bool is_queue_empty();
void print_queue_state();
struct queue_node* create_queue_node(
        int container_id, 
        pthread_cond_t* cond_var_allow_unload, 
        pthread_mutex_t* m_cond
    );



void container(int container_id, sigval_t* info);
void* dispatcher(void* args);
int sec_to_ns(float seconds);
void fill_timerspec(struct itimerspec* spec, long int time_ns, long int interval_time_ns);
struct sigevent* initialize_event(int signumber, int container_id);
void initialize_signal(int signumber, void(*handler)(int, siginfo_t *, void *));
char* now();





int main(int argc, char* argv[]) 
{
    struct sigevent* created_event;
    struct itimerspec itime[3];
    timer_t timer_id[3];
    int signumber;
    
    int i=3;
    int container_ID;

    setbuf(stdout, NULL);
    srand(time(NULL));
    pthread_mutex_init(&m_wait_for_container, NULL);

    for(i=0; i<3; i++)
    {
        container_ID = i;
        signumber = SIGRTMIN + i + 1;

        initialize_signal(signumber, &container);
        created_event = initialize_event(signumber, container_ID);

        timer_create(CLOCK_REALTIME, created_event, &timer_id[i]);        
        fill_timerspec(&itime[i], sec_to_ns(5), sec_to_ns(T));
        timer_settime(timer_id[i], 0, &itime[i], NULL);
    }

    pthread_t  dispatch_thread;
    pthread_create(&dispatch_thread, NULL, &dispatcher, NULL);
    pthread_join(dispatch_thread, NULL);

    return EXIT_SUCCESS;
}



int sec_to_ns(float seconds)
{
    return (int)(seconds * 1000000);
}

void fill_timerspec(struct itimerspec* spec, long int time_ns, long int interval_time_ns)
{
    int value_sec = time_ns / 1000000;
    int value_nsec = time_ns % 1000000;
    int i_value_sec = interval_time_ns / 1000000;
    int i_value_nsec = interval_time_ns % 1000000;

    spec->it_value.tv_sec = value_sec;
    spec->it_value.tv_nsec = value_nsec;
    spec->it_interval.tv_sec = i_value_sec;
    spec->it_interval.tv_nsec = i_value_nsec;
}



struct sigevent* initialize_event(int signumber, int container_id)
{
    struct sigevent* sigevp;

    LM; printf("Intitializing event for container %d (signal: %d)\n", container_id, signumber); UM;

    sigevp = malloc(sizeof(struct sigevent));
    sigevp->sigev_notify = SIGEV_THREAD;
    sigevp->sigev_notify_function = &container;
    sigevp->sigev_signo = signumber;
    sigevp->sigev_value.sival_int = container_id;

    return sigevp;
}

void initialize_signal(int signumber, void(*handler)(int, siginfo_t *, void *))
{
    sigset_t set;
    sigemptyset(&set);
    struct sigaction action;
    sigaddset(&set, signumber);
    action.sa_flags = SA_SIGINFO;
    action.sa_mask = set;
    action.sa_sigaction = handler;
    sigaction(signumber, &action, NULL);
}


void* dispatcher(void* args)
{
    struct queue_node* current_unloading_container_data;
    bool first_notify = false;

    while(true)
    {
        LM; printf("%s\t| [Dispatcher]: Cycle started\n", now()); UM;
        if (is_queue_empty())
        {
            LM; printf("%s\t| [Dispatcher]: Waiting for new containers added\n", now()); UM;
            pthread_cond_wait(&cond_container_arrived, &m_wait_for_container);
        }


        LM; printf("%s\t| [Dispatcher]: Queue state: ", now()); print_queue_state(); printf("\n"); UM;
        LM; printf("%s\t| [Dispatcher]: Queue not empty, working on\n", now()); UM;

        current_unloading_container_data = pop_queue_node();

        LM; printf("%s\t| [Dispatcher]: Dispatcher maked his descision: %d loads out\n", now(), current_unloading_container_data->container_id); UM;

        // Диспетчер посылает разрешение на разгрузку

        pthread_mutex_lock(current_unloading_container_data->m_cond);
        pthread_cond_signal(current_unloading_container_data->cond_var_allow_unload);
        pthread_mutex_unlock(current_unloading_container_data->m_cond);

        LM; 
        printf
            (
                "%s\t| [Dispatcher]: Waiting container %d to end his load out\n", 
                now(), current_unloading_container_data->container_id
            ); 
        UM;

        pthread_cond_wait(&container_ended_loadout, &m_container_ended_loadout);
    };
}


void container(int container_id, sigval_t* info)
{   
    pthread_cond_t* cond_var_allow_unload = malloc(sizeof(pthread_cond_t));
    pthread_mutex_t* waiting_allowance = malloc(sizeof(pthread_mutex_t));
    int ret_code, error_code;
    
    pthread_cond_init(cond_var_allow_unload, NULL);
    pthread_mutex_init(waiting_allowance, NULL);
    int container_ID = container_id;

    LM; printf("%s\t| [Container %d]: created in thread 0x%x\n", now(), container_ID, (int)pthread_self()); UM;

    put_container_id_in_queue(container_ID, cond_var_allow_unload, waiting_allowance);

    pthread_cond_signal(&cond_container_arrived);

    LM; printf("%s\t| [Container %d]: Waiting fow allowance\n", now(), container_ID); UM;

    pthread_cond_wait(cond_var_allow_unload, waiting_allowance);

    // Контейнер разгружается

    LM; printf("%s\t| [Container %d]: Unloading\n", now(), container_ID); UM;
    sleep(loadout_time);
    LM; printf("%s\t| [Container %d]: Unloaded, end of cycle\n", now(), container_ID); UM;

    pthread_mutex_lock(&m_container_ended_loadout);
    pthread_cond_signal(&container_ended_loadout);
    pthread_mutex_unlock(&m_container_ended_loadout);
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


int pop_container_id_from_queue()
{
    pthread_mutex_lock(&m_queue);
    if (q_head == NULL)
    {
        LM; printf("Error: empty queue"); UM;
        return -1;
    }

    int container_id = q_head->container_id;
    struct queue_node* to_delete = q_head;

    q_head = q_head->next;
    free(to_delete);

    pthread_mutex_unlock(&m_queue);
    return container_id;
}


struct queue_node* pop_queue_node()
{
    pthread_mutex_lock(&m_queue);
    if (q_head == NULL)
    {
        LM; printf("Error: empty queue"); UM;
        return NULL;
    }

    struct queue_node* node = q_head;

    q_head = q_head->next;

    pthread_mutex_unlock(&m_queue);
    return node;
}


void put_container_id_in_queue(int container_id, pthread_cond_t* cond_var_allow_unload, pthread_mutex_t* m_cond)
{
    pthread_mutex_lock(&m_queue);

    if (q_head == NULL) 
    {
        q_head = create_queue_node(container_id, cond_var_allow_unload, m_cond);
    } 
    else
    {
        struct queue_node* current_node = q_head;
        while (current_node->next != NULL)
        {
            current_node = current_node->next;
        }
        current_node->next = create_queue_node(container_id, cond_var_allow_unload, m_cond);
    }

    pthread_mutex_unlock(&m_queue);
}

bool is_queue_empty()
{
    return (q_head == NULL);
}

void print_queue_state()
{
    struct queue_node* node;
    if (q_head == NULL)
    {
        printf("queue: empty");
        return;
    }

    node = q_head;

    while(node != NULL)
    {
        if (node->next != NULL) 
            printf("(%d)-", node->container_id);
        else 
            printf("(%d)", node->container_id);
        
        node = node->next;
    }
}

struct queue_node* create_queue_node(int container_id, pthread_cond_t* cond_var_allow_unload, pthread_mutex_t* m_cond)
{
    struct queue_node* node;
    node = malloc(sizeof(struct queue_node));
    node->container_id = container_id;
    node->cond_var_allow_unload = cond_var_allow_unload;
    node->m_cond = m_cond;
    node->next = NULL;
    return node;
}

