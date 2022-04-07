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
#define RESET "\x1b[0m"

#define minBox 20   // min number of box in the storage area to take
#define maxTruck 10 // maximum storage of truck

int numOfTrucks = 2;
int numOfWorkers = 2;
int numOfBoxes = 0;
int truckCap = 0;
int Truckindex; // to now which truck we are

pthread_mutex_t mutex;

pid_t ids[2]; // to store process (fork)

// struct for message queue
struct msgbuf
{
    char msg_txt[2];
    long msg_type;
};

// Declarate Functions./*  */
int dealyTime(int min, int max);
void *handlingtruckWorkers();
void *handlingtrucks();
void *Kill_truck();

int main()
{
    srand(time(NULL));
    pid_t pid; // to create fork
    int I, status;
    pthread_t out_kill; // to out from truck and kill the process when we get and order from CEO
    // create threads
    if (pthread_create(&out_kill, NULL, &Kill_truck, NULL) != 0)
    {
        perror("Error create a thread");
        exit(1);
    }

    // to create process (fork) and handle them
    for (I = 0; I < numOfTrucks; I++)
    {
        pid = fork();
        if (pid == 0) // fork
        {
            srand(time(NULL) ^ (getpid() << 16));
            Truckindex = I;
            // define threads
            pthread_t empW[numOfWorkers], truckShipping;

            pthread_mutex_init(&mutex, NULL);

            // for loop to create threads worker
            for (int i = 0; i < numOfWorkers; i++)
            {
                if (pthread_create(&empW[i], NULL, &handlingtruckWorkers, NULL) != 0)
                {
                    perror("Error create a thread");
                    exit(1);
                }
            }
            // create the second thread
            if (pthread_create(&truckShipping, NULL, &handlingtrucks, NULL) != 0)
            {
                perror("Error create a thread");
                exit(1);
            }
            // make Join
            for (int i = 0; i < numOfWorkers; i++)
            {
                if (pthread_join(empW[i], NULL) != 0)
                {
                    perror("Error join a thread");
                    exit(1);
                }
            }

            if (pthread_join(truckShipping, NULL) != 0)
            {
                perror("Error join a thread");
                exit(1);
            }

            pthread_mutex_destroy(&mutex);

            exit(0);
        }
        else
        {                 // parent
            ids[I] = pid; // store ips for child
        }
    }

    if (pthread_join(out_kill, NULL) != 0)
    {
        perror("Error join a thread");
        exit(1);
    }
    wait(NULL);

    return 0;
}

// function to make a delay time
int dealyTime(int min, int max)
{
    int num;
    num = (rand() % (max - min + 1)) + min;
    return num;
}

// function to handle the truck employee (Drivers)
void *handlingtruckWorkers()
{
    // for read and write on shared memory
    int shm_id;
    key_t mem_key;
    int *shm_ptr, *s;
    mem_key = ftok(".", 'a');

    // for messgae queue
    key_t key;
    struct msgbuf msg;
    int msgid;

    // create shred memory and make an attach
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

    // to create message queue and read it
    if ((key = ftok("trucks.c", 'c')) == -1)
    {
        perror("can't generate the key for the queue");
        exit(1);
    }

    if ((msgid = msgget(key, 0666 | IPC_CREAT)) == -1)
    {
        perror("can't generate the msgid");
        exit(1);
    }

    msg.msg_type = 1;
    sprintf(msg.msg_txt, "%d", 1);

    while (1)
    {
        // check shared memory
        s = shm_ptr;
        numOfBoxes = *s;

        if (numOfBoxes > minBox && truckCap < maxTruck)
        {
            int delay = dealyTime(5, 10);
            printf(BLUE "The Worker of Truck #%d is uploading the box to the truck now please wait %d seconds untill he's finsih... \n" RESET, Truckindex, delay);
            sleep(delay);

            // compare with storage area and cap of truck to block and unblock the uploading boxes
            while (truckCap >= maxTruck)
                ;
            if (*s <= minBox)
            {
                printf(YELLOW "Waitting Until Storage area to get fill again\n" RESET);
            }
            while (*s <= minBox)
                ;
            pthread_mutex_lock(&mutex);
            truckCap++;
            pthread_mutex_unlock(&mutex);

            printf("Truck #%d has:%d on it \n", Truckindex, truckCap);
            if ((msgsnd(msgid, &msg, sizeof(msg), 0)) == -1)
            {
                perror("mes send error");
            }
        }
    }
}

// function to handle the Truck and it's storage
void *handlingtrucks()
{
    // for messgae Q
    key_t key;
    struct msgbuf msg;
    int msgid;

    // create message and read it
    if ((key = ftok("ceo.c", 'b')) == -1)
    {
        perror("can't generate the key for the queue");
        exit(1);
    }

    if ((msgid = msgget(key, 0666 | IPC_CREAT)) == -1)
    {
        perror("can't generate the msgid");
        exit(1);
    }

    msg.msg_type = 1;
    sprintf(msg.msg_txt, "%d", 1);

    while (1)
    {
        // if the storage of truck = the max size of truck
        if (truckCap == maxTruck)
        {
            int delay = dealyTime(15, 20);
            printf(GREEN "Truck shipping the Laptops, please wait %d sec \n" RESET, delay);
            sleep(delay);

            if ((msgsnd(msgid, &msg, sizeof(msg), 0)) == -1)
            {
                perror("mes send error");
            }
            truckCap = 0;
            printf(MAGENTA "Truck is return form delivery the Laptops\n" RESET);
        }
    }
}

// function to kill the Truck employee and thread when recieve order form CEO
void *Kill_truck()
{
    // to read shared memory.
    int shm_id;
    key_t mem_key;
    int *shm_ptr, *s;

    mem_key = ftok(".", 'a');

    //create shared memory and make attach
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
    while (1)
    {
        s = shm_ptr + 4; // to go to the second location
        if (*s == 2)
        {
            // if we read 2 then we gain and exit
            printf(RED "We Done Truck Drivers stop work we done\n" RESET);
            for (int i = 0; i < numOfTrucks; i++)
            {
                kill(ids[i], SIGINT);
            }
            exit(2);
        }
    }
}