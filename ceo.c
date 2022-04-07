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
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

// varaibles to calculation the salary and compare
int budget = 0;   // inisital budget we make
int expenses = 0; // to determine if we expenss other things
// salaries of employees
int salaryCEO = 2500, salaryHR = 2000, salaryTech = 1100, slarayStorage = 800, salaryLoad = 800, salaryDrivers = 700;
int salaryExtra = 700, costFab = 500, priceCell = 3000;
int numofTruck = 2, numofLoadEmp = 2, numofLines = 10, numofExtra = 5;
// Threshold to compare
int Threshold = 600000; // thesold to know if we lose or gain
int gain = 1000000;     // threshold to make sure that we Gain alot

// make a genral pipe to talk
int pipe_CEO[2];

int line = -1;

pthread_mutex_t mutex;

// struct for message queue
struct msgbuf
{
  char msg_txt[2];
  long msg_type;
};

int HRpid; // value to store the ip of HR

// Declarate functions
int dealyTime(int min, int max);
void *handlingShpping(void *arg);
void *calculate();

int main()
{
  srand(time(NULL));

  int pipe1[2];  // pipe for CEO with HR
  pid_t CEO, HR; // to create 2 process

  // check the pipes
  if (pipe(pipe_CEO) == -1)
  {
    printf("Unable to create pipe for parent, try again \n");
    return 1;
  }

  if (pipe(pipe1) == -1)
  {
    printf("Unable to create pipe for parent, try again \n");
    return 1;
  }

  // create process
  (CEO = fork()) && (HR = fork());
  if (CEO < 0 || HR < 0)
  {
    printf("Error create CEO or HR");
  }
  else if (HR == 0) // child process HR
  {
    HRpid = getpid(); // store ip of it

    // to read and write from pipe
    close(pipe1[0]);
    write(pipe1[1], &HRpid, sizeof(int));

    pthread_t reciveProfit, distribution; // define threads

    // create threads
    if (pthread_create(&reciveProfit, NULL, &handlingShpping, NULL) != 0)
    {
      perror("Error create a thread");
      exit(1);
    }

    if (pthread_create(&distribution, NULL, &calculate, NULL) != 0)
    {
      perror("Error create a thread");
      exit(1);
    }
    // make join
    if (pthread_join(reciveProfit, NULL) != 0)
    {
      perror("Error join a thread");
      exit(1);
    }

    if (pthread_join(distribution, NULL) != 0)
    {
      perror("Error join a thread");
      exit(1);
    }

    exit(1);
  }
  else if (CEO == 0) // child --> CEO
  {
    // define for message queue
    key_t key;
    struct msgbuf msg;
    int msgid;

    pid_t ids[10]; // to store ids on it

    // define for shared memory
    int shm_id;
    key_t mem_key;
    int *shm_ptr, *s;

    mem_key = ftok(".", 'a');

    // create shared memory and message queue
    if ((shm_id = shmget(mem_key, sizeof(int) * 2, IPC_CREAT | 0666)) < 0)
    {
      perror("Error creation a shared memory");
      exit(1);
    }

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

    msg.msg_type = 1;

    int l;
    int par = 1;
    while (1)
    {
      // read pipe from function
      read(pipe_CEO[0], &l, sizeof(int)); // take the value form calcauatiion function

      if (l == -1)
      {
        sprintf(msg.msg_txt, "%d", 1);

        if ((msgsnd(msgid, &msg, sizeof(msg), 0)) == -1)
        {
          perror("MSG send error");
          exit(1);
        }

        printf(RED "CEO:I'm going to suspend all employees on line:%d! \n" RESET, ++line);
      }
      else
      {
        if (line != -1)
        {
          sprintf(msg.msg_txt, "%d", 0);
          if ((msgsnd(msgid, &msg, sizeof(msg), 0)) == -1)
          {
            perror("MSG send error");
            exit(1);
          }
          printf(GREEN "CEO:All Employees on line:%d are Hire to work Again! \n" RESET, line--);
        }
        printf(YELLOW "CEO:Our Gain is:%d$ \n" RESET, l);
      }

      numofLines = 10 - (line + 1);
      printf(BLUE "CEO:Number of lines is:%d lines\n" RESET, numofLines);

      if (l >= gain)
      {
        read(pipe1[0], &l, sizeof(int)); // take the value form calcauatiion function

        if ((shm_ptr = shmat(shm_id, NULL, 0)) == -1)
        {
          perror("Error with the attachment of the shared memory");
          exit(1);
        }
        printf(RED " WE Gain alot stop working and exit\n" RESET);
        s = shm_ptr + 4;
        *s = 2; // sotre value 2 in shared memory
        kill(l, SIGINT);
        exit(2);
      }
      // if we suspend  lines and we just have 5 line we BROKE
      if (numofLines == 5)
      {
        read(pipe1[0], &l, sizeof(int)); // l is the HR PID number
        printf(RED "CEO:We are broke, we are going to sell the factory\n" RESET);
        if ((shm_ptr = shmat(shm_id, NULL, 0)) == -1)
        {
          perror("Error with the attachment of the shared memory");
          exit(1);
        }
        s = shm_ptr + 4;
        *s = 2; // sotre value 2 in shared memory
        kill(l, SIGINT);
        exit(2);
      }
    } // end while (1)
    exit(1);
  }
  else
  {

    wait(NULL);
  }

  return 0;
}

//function to make a random delay time
int dealyTime(int min, int max)
{
  int num;
  num = (rand() % (max - min + 1)) + min;
  return num;
}

// function to handle worker of shipping
void *handlingShpping(void *arg)
{

  // define for message queue
  key_t key;
  struct msgbuf msg1;
  int msgid;
  int value;

  // create message queue
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

  while (1)
  {
    if ((msgrcv(msgid, &msg1, sizeof(msg1), 0, 0)) == -1)
    {
      perror("MSG recieve error");
      exit(1);
    }

    pthread_mutex_lock(&mutex);

    if (atoi(msg1.msg_txt) == 1)
    {
      budget += (priceCell - costFab) * 100; // Update our budgete
    }
    printf(MAGENTA "The Shipping is done, Budget= %d$ \n " RESET, budget);
    pthread_mutex_unlock(&mutex);
  }
}

// function to make calculation
void *calculate()
{

  while (1)
  {
    sleep(60);
    int SD = salaryDrivers * numofTruck;                             // salary of Drivers
    int SS = slarayStorage * numofLines;                             // salary of storage employee
    int SL = salaryLoad * numofLoadEmp * numofTruck;                 // salary of Load employee into Trucks
    int ST = salaryTech * numofLines * 10;                           // salary of TEchinical empolyee
    int SE = numofExtra * salaryExtra;                               // salary for Extra Employee
    budget = budget - salaryCEO - salaryHR - SD - SS - SL - ST - SE; // Total Gain after give the salaries
    int x = -1;                                                      // if we lose

    //compare budget with threshold to know if we lose or gain
    if (budget >= Threshold) // for gain
    {
      printf("HR:We have a gain in the budget! \n");
      printf("HR:What about adding new lines ?! \n");
      close(pipe_CEO[0]);
      write(pipe_CEO[1], &budget, sizeof(int));
      if (line != -1)
        line--;
    }
    else // for lose
    {
      printf(CYAN "HR:We have a lose in the budget we need to suspense some employees! \n" RESET);
      printf(CYAN "HR:Our budget is:%d$\n" RESET, budget);
      close(pipe_CEO[0]);
      write(pipe_CEO[1], &x, sizeof(int));
      line++;
    }
    numofLines = 10 - (line + 1);
  }
}
