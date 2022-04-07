#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <sys/shm.h>

// define colors for output
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

#define maxBox 100       // max size of storage area
#define minBox 80        // min size of storage area
#define numOfEmployee 10 // num of employee

int numOfLines = 10;            // num of lines Manf.
int numOfBoxes = 0;             // variable to number of box we manf.
int check[5] = {0, 1, 1, 1, 1}; // check for the employee from 6 to 10
int boxSize = 0;                // variable for put laptop on it.

int lines = -1;
int line; // refers which line we in

// MUTEX to lock and unlock thread
pthread_mutex_t mutex;
pthread_mutex_t mutex1;

// struct for message queue and check the employee in lines from 6 to 10
struct msgbuf
{
    char msg_txt[2];
    long msg_type;
};

struct laptop
{
    int steps[5];
    int busy; //flag
    int done; //flag
};

struct laptop laptops[5];

//to  store child process(fork)
pid_t ids[10];

//Declarate functions
void *handlingStorageWorkers(void *arg);
void checkLaps();
void checkAvaliableLaps(int index);
void *handlingLinesEmployee(void *arg);
void *suspendLines();
void *kill_lines();
int dealyTime(int min, int max);

int main()
{
    srand(time(NULL));

    pid_t pid; // create process to make fork
    int I, status;

    pthread_t Lines, out_lines; // create threads for lines and end prog (out_lines)

    // create threads
    if (pthread_create(&Lines, NULL, &suspendLines, NULL) != 0)
    {
        perror("Error create a thread");
        exit(1);
    }

    if (pthread_create(&out_lines, NULL, &kill_lines, NULL) != 0)
    {
        perror("Error create a thread");
        exit(1);
    }

    // for loop to create processes as the number of lines we have
    for (I = 0; I < numOfLines; I++)
    {
        pid = fork();

        if (pid == 0) // child
        {
            srand(time(NULL) ^ (getpid() << 16));
            line = I; // line = which line we now

            // threads for employee in the line and for employee who will put the box inside the storage
            pthread_t empO[numOfEmployee], storageWorkers;

            //for loop to check
            for (int i = 0; i < 10; i++)
            {
                laptops[i].steps[5] = 0;
                laptops[i].busy = 0;
                laptops[i].done = 0;
            }

            pthread_mutex_init(&mutex, NULL);

            // for loop for handle between employee in the same lines
            for (int i = 0; i < numOfEmployee; i++)
            {

                int *a = malloc(sizeof(int));
                *a = i;
                if (pthread_create(&empO[i], NULL, &handlingLinesEmployee, a) != 0)
                {
                    perror("Error create a thread");
                    exit(1);
                }
            }

            int *z = malloc(sizeof(int));
            *z = I;
            // thread for storage worker
            if (pthread_create(&storageWorkers, NULL, &handlingStorageWorkers, z) != 0)
            {
                perror("Error create a thread");
                exit(1);
            }

            // create employee in the line
            for (int i = 0; i < numOfEmployee; i++)
            {
                if (pthread_join(empO[i], NULL) != 0)
                {
                    perror("Error join a thread");
                    exit(1);
                }
            }

            if (pthread_join(storageWorkers, NULL) != 0)
            {
                perror("Error join a thread");
                exit(1);
            }

            pthread_mutex_destroy(&mutex);
            exit(0);
        }
        else
        {
            ids[I] = pid; // sotre ids for employee
        }
    }

    if (pthread_join(Lines, NULL) != 0)
    {
        perror("Error join a thread");
        exit(1);
    }

    if (pthread_join(out_lines, NULL) != 0)
    {
        perror("Error join a thread");
        exit(1);
    }

    wait(NULL);
    return 0;
}

// fucntion to make dealy by random
int dealyTime(int min, int max)
{
    int num;
    num = (rand() % (max - min + 1)) + min;
    return num;
}

// function to handle the storage worker work
void *handlingStorageWorkers(void *arg)
{
    // to open shared memory:
    int shm_id;
    key_t mem_key;
    int *shm_ptr, *s;
    mem_key = ftok(".", 'a');

    // to make message quque:
    key_t key;
    struct msgbuf msg;
    int msgid;
    int index = *(int *)arg;

    // create shared memory
    if ((shm_id = shmget(mem_key, sizeof(int) * 2, IPC_CREAT | 0666)) < 0)
    {
        perror("Error creation a shared memory");
        exit(1);
    }
    // attach with shared memory
    if ((shm_ptr = shmat(shm_id, NULL, 0)) == -1)
    {
        perror("Error with the attachment of the shared memory");
        exit(1);
    }

    // message quque between files
    if ((key = ftok("storage.c", 'b')) == -1)
    {
        perror("can't generate the key for the queue");
        exit(1);
    }
    // get the message
    if ((msgid = msgget(key, 0666 | IPC_CREAT)) == -1)
    {
        perror("can't generate the msgid");
        exit(1);
    }

    msg.msg_type = 1;
    sprintf(msg.msg_txt, "%d", 1);

    while (1)
    {
        int delay = dealyTime(5, 10); // make dealy
        printf("************************\n");
        printf(CYAN "The Line: %d work >>> \n" RESET, index);

        //refers to sahred memory to check and store
        s = shm_ptr;
        numOfBoxes = *s;
        if (boxSize >= 10 && numOfBoxes < maxBox)
        {
            printf(GREEN "The Storage employee is taking the box to the storage now please wait %d untill he's back... \n" RESET, delay);
            sleep(delay);
            s = shm_ptr;
            numOfBoxes = *s;
            while (numOfBoxes >= maxBox)
                ;

            if ((msgsnd(msgid, &msg, sizeof(msg), 0)) == -1)
            {
                perror("mes send error");
            }
            boxSize -= 10;
            printf(GREEN "Done storaging the box ..... \n" RESET);
        }
        /*    for (int i = 0; i < 5; i++)
        {
            printf("[%d]",check[i]);
        }
        printf("\n"); */

        sleep(1);
    }
}

// function to handle the employee in the lines
void *handlingLinesEmployee(void *arg)
{
    while (1)
    {
        int index = *(int *)arg; //index of number employee
        int j = dealyTime(1, 3);
        // check if employee between 0 to 4
        if (index < 5)
            while (check[index] == 1)
                ; // if there is no work the employee will wait until the previous employee finish his work

        if (index < 4)
        {
            sleep(j);
            while (check[index + 1] == 0) // wait until next worker finish his work
                ;
        }
        else if (index == 4)
        {
            sleep(j);
            checkLaps(); //  call function --> make new laptop max laptops =5
        }
        else
        {
            checkAvaliableLaps(index); // call function --> check if there is avaliable laptop to work
        }

        // check employee from 0 to 4 will handle the work, next one and tell the priveous employee finished his work
        if (index < 5)
        {
            check[index + 1] = 0;
            check[index] = 1;
        }

        if (index == 0)
        {
            check[index] = 0;
        }
    }
}

// check which employee free and which not form 6 to 10
void checkLaps()
{
    int d = 0;
    while (d == 0)
    {
        for (int i = 0; i < 5; i++)
        {
            if (laptops[i].done == 0)
            {
                laptops[i].done = 1;
                d = 1;
                printf("EMP5:We finished a new lap\n");
                break;
            }
        }
    }
}

// check which employee avlaibale
void checkAvaliableLaps(int index)

{
    int d = 0;
    int delay = dealyTime(1, 3);
    while (d == 0)
    { // check if there is avaliable laptops
        for (int i = 0; i < 5; i++)
        {
            int x = 0;
            pthread_mutex_lock(&mutex);
            // check if the laptop free and we didin't work on it.
            if (laptops[i].done == 1 && laptops[i].busy == 0 && laptops[i].steps[index - 5] == 0)
            {
                x = 1;
                laptops[i].busy = 1;
            }
            pthread_mutex_unlock(&mutex);

            if (x == 1)
            {
                printf(BLUE "Employee #:%d is working with laptop %d \n" RESET, index, i);
                sleep(delay);
                d = 1;
                laptops[i].steps[index - 5] = 1;
                int test = 1;
                for (int j = 0; j < 5; j++)
                {
                    if (laptops[i].steps[j] == 0)
                    {
                        test = 0;
                        break;
                    }
                }
                sleep(delay);
                printf(YELLOW "Employee #%d is done working with laptop: %d \n" RESET, index, i);
                sleep(delay);
                if (test == 1)
                {
                    if (numOfBoxes >= maxBox)
                    {
                        printf(MAGENTA "Wating until workers finsish uploading the trucks in the Stoarage \n" RESET);
                        while (numOfBoxes > minBox)
                            ;
                    }

                    printf(GREEN "The Employee number:%d put the laptop in the box, Box size from line [%d] is:%d \n" RESET, index, line, ++boxSize);
                    sleep(delay);
                    laptops[i].busy = 0;
                    laptops[i].done = 0;
                    for (int j = 0; j < 5; j++)
                    {
                        laptops[i].steps[j] = 0;
                    }
                }
                laptops[i].busy = 0;

                break;
            }
        }
    }
}

// function to suspend all employee from line when we lose
void *suspendLines()
{
    // for message queue
    key_t key;
    struct msgbuf msg1;
    int msgid;
    int value;

    if ((key = ftok("lines.c", 'c')) == -1)
    {
        perror("can't generate the key for the queue");
        exit(1);
    }

    if ((msgid = msgget(key, 0666 | IPC_CREAT)) == -1)
    {
        perror("can't generate the msgid");
        exit(1);
    }
    // when recieve message
    while (1)
    {

        if ((msgrcv(msgid, &msg1, sizeof(msg1), 0, 0)) == -1)
        {
            perror("MSG recieve error");
            exit(1);
        }
        //compare message that we get
        if (atoi(msg1.msg_txt) == 1)
        {
            lines++;
            kill(ids[lines], SIGSTOP); // suspend emplyee form line
        }
        if (atoi(msg1.msg_txt) == 0)
        {
            kill(ids[lines], SIGCONT); // rehiring the employee form line
            lines--;
        }
        // if we suspend more than half employee we lose and close the factory
        if (lines == 4)
        {
            printf(RED " We lose alot of employee stop the work we done\n" RESET);
            /* for (int i = 0; i < 10; i++)
            {
                kill(ids[i], SIGINT);
            }
            exit(0); */
        }
    }
}

// function to check if we lose or gain then stop the Manufacturing
void *kill_lines()
{
    // read shared memory
    int shm_id;
    key_t mem_key;
    int *shm_ptr, *s;

    mem_key = ftok(".", 'a');

    if ((shm_id = shmget(mem_key, sizeof(int) * 2, IPC_CREAT | 0666)) < 0)
    {
        perror("Error creation a shared memory");
        exit(1);
    }

    if ((shm_ptr = shmat(shm_id, NULL, 0)) == -1)
    {
        perror("Error with the attachment of the shared memory");
        exit(1);
    }
    // if we get the value form shared memory
    while (1)
    {
        s = shm_ptr + 4;
        if (*s == 2)
        {
            printf(RED "All Employees Stop Work we done\n" RESET);
            for (int i = 0; i < 10; i++)
            {
                kill(ids[i], SIGINT);
            }
            exit(2);
        }
    }
}
