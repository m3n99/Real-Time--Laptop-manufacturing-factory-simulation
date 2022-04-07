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
#include <sys/msg.h>

// define colors for output
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define RESET "\x1b[0m"

int numOfBoxes = 60; // number of boxes we have in the storage

pthread_mutex_t mutex;

struct msgbuf
{
    char msg_txt[2];
    long msg_type;
};

// declaration function
void *handlingTruckWorkers(void *arg);
void *handlingStorageWorkers(void *arg);
void *updatingStorage(void *arg);

int main()
{
    srand(time(NULL));
    // define threads
    pthread_t storageWorkers, truckWorkers, storageCapacity;

    pthread_mutex_init(&mutex, NULL);

    // create threades and join them
    if (pthread_create(&storageWorkers, NULL, &handlingStorageWorkers, NULL) != 0)
    {
        perror("Error create a thread");
        exit(1);
    }

    if (pthread_create(&truckWorkers, NULL, &handlingTruckWorkers, NULL) != 0)
    {
        perror("Error create a thread");
        exit(1);
    }

    if (pthread_create(&storageCapacity, NULL, &updatingStorage, NULL) != 0)
    {
        perror("Error create a thread");
        exit(1);
    }

    if (pthread_join(storageWorkers, NULL) != 0)
    {
        perror("Error join a thread");
        exit(1);
    }

    if (pthread_join(truckWorkers, NULL) != 0)
    {
        perror("Error join a thread");
        exit(1);
    }

    if (pthread_join(storageCapacity, NULL) != 0)
    {
        perror("Error join a thread");
        exit(1);
    }

    pthread_mutex_destroy(&mutex);

    // pthread_mutex_lock(&mutex);

    return 0;
}

// function to handle the worker those put the boxes from storage to truck
void *handlingTruckWorkers(void *arg)
{
    // message queue
    key_t key;
    struct msgbuf msg1;
    int msgid;
    int value;

    // create message queue
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

    while (1)
    {
        if ((msgrcv(msgid, &msg1, sizeof(msg1), 0, 0)) == -1)
        {
            perror("MSG recieve error");
            exit(1);
        }

        pthread_mutex_lock(&mutex);

        // recieve messgage queue and dec the number of boxes we have
        if (atoi(msg1.msg_txt) == 1)
            numOfBoxes--;
        printf(YELLOW "Number of Boxes after decrement(Put in the Truck):%d \n " RESET, numOfBoxes);
        pthread_mutex_unlock(&mutex);
    }
}

// function to handle the storage worker those get the boxes from the lines Manf.
void *handlingStorageWorkers(void *arg)
{
    // Message Queue
    key_t key;
    struct msgbuf msg;
    int msgid;
    int value;

    // create messages
    key = ftok("storage.c", 'b');
    if (key == -1)
    {
        perror("can't generate the key for the queue");
        exit(1);
    }

    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("can't generate the msgid");
        exit(1);
    }

    while (1)
    {
        // recive the message
        if ((msgrcv(msgid, &msg, sizeof(msg), 0, 0)) == -1)
        {
            perror("MSG recieve error");
            exit(1);
        }
        // make mutex to make sure 1 at time handle it
        pthread_mutex_lock(&mutex);
        if (atoi(msg.msg_txt) == 1)
            numOfBoxes++;

        printf(GREEN "Number of Boxes after increment(Storage the Boxes): %d \n " RESET, numOfBoxes);
        pthread_mutex_unlock(&mutex);
    }
}

// function to update the sorage area
void *updatingStorage(void *arg)
{
    // For shared memory
    int shm_id;
    key_t mem_key;
    int *shm_ptr, *s;
    mem_key = ftok(".", 'a');

    // create shared memory and make attach with it
    if ((shm_id = shmget(mem_key, sizeof(int) * 2, 0666 | IPC_CREAT)) < 0)
    {
        perror("Error creation a shared memory");
        exit(1);
    }

    if ((shm_ptr = shmat(shm_id, NULL, 0)) == -1)
    {
        perror("Error with the attachment of the shared memory");
        exit(1);
    }
    // to go through shared memory
    s = shm_ptr;
    s = s + 4;
    *s = 1;

    while (1)
    {
        s = shm_ptr;
        *s = numOfBoxes;
        s = shm_ptr + 4;
        if (*s == 2) // if we read form the second location in the shared memory 2 this mean that this order from CEO
        {
            printf(RED "Storage Worker stop storaging and all Employees out \n" RESET);
            // deattach with shared memory to delete it
            shmdt(shm_ptr);
            shmctl(shm_id, IPC_RMID, NULL);
            exit(2);
        }
    }
}
