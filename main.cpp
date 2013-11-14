/* Wikipedia: possible solution
Another relatively simple solution is achieved by introducing a waiter at the table. Philosophers must ask his permission before taking up any forks. Because the waiter is aware of how many forks are in use, he is able to arbitrate and prevent deadlock. When four of the forks are in use, the next philosopher to request one has to wait for the waiter's permission, which is not given until a fork has been released. The logic is kept simple by specifying that philosophers always seek to pick up their left-hand fork before their right-hand fork (or vice versa). The waiter acts as a semaphore, a concept introduced by Dijkstra in 1965.[5]
To illustrate how this works, consider that the philosophers are labelled clockwise from A to E. If A and C are eating, four forks are in use. B sits between A and C so has neither fork available, whereas D and E have one unused fork between them. Suppose D wants to eat. When he wants to take up the fifth fork, deadlock becomes likely. If instead he asks the waiter and is told to wait, we can be sure that next time two forks are released there will certainly be at least one philosopher who could successfully request a pair of forks. Therefore deadlock cannot happen.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>

using namespace std;

//a philosopher i has access to fork i and (i+1)%n
int noPhilosophers;
bool *forkInUse;
unsigned long long *noFeedings;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t *waiting;
pthread_t *philosopherThread;
bool finish = false;

const char fileName[] = "output.txt";

void readyToEat(int i, int leftFork, int rightFork) {
    pthread_mutex_lock(&mutex);
    while (forkInUse[leftFork] || forkInUse[rightFork]) {
        pthread_cond_wait(&waiting[i], &mutex);
    }
    forkInUse[leftFork] = true;
    forkInUse[rightFork] = true;
    pthread_mutex_unlock(&mutex);
}

void doneEating(int i, int leftFork, int rightFork, int leftPhilosopher, int rightPhilosopher) {    
    pthread_mutex_lock(&mutex);
    forkInUse[leftFork] = false;
    forkInUse[rightFork] = false;
    pthread_cond_signal(&waiting[leftPhilosopher]);
    pthread_cond_signal(&waiting[rightPhilosopher]);
    pthread_mutex_unlock(&mutex);
}

void *philosopher(void *args) {
    int *i = (int *) args;
    int leftFork = *i;
    int rightFork = (*i + 1) % noPhilosophers;
    int leftPhilosopher = *i == 0 ? noPhilosophers - 1 : *i - 1; //mod on negative number results in negative number in C++
    int rightPhilosopher = rightFork;


    while (!finish) {
        readyToEat(*i, leftFork, rightFork);
        
        //eat
        noFeedings[*i]++;
        
        doneEating(*i, leftFork, rightFork, leftPhilosopher, rightPhilosopher);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    //read input
    noPhilosophers = atoi(argv[1]);
    int noSeconds = atoi(argv[2]);

    forkInUse = new bool[noPhilosophers];
    noFeedings = new unsigned long long[noPhilosophers];
    waiting = new pthread_cond_t[noPhilosophers];
    philosopherThread = new pthread_t[noPhilosophers];

    //initialize data structures and spawn threads
    for (int i = 0; i < noPhilosophers; i++) {
        forkInUse[i] = false;
        noFeedings[i] = 0;
        waiting[i] = PTHREAD_COND_INITIALIZER;

        int *privateI = new int;
        *privateI = i;
        pthread_create(&philosopherThread[i], NULL, philosopher, (void *) privateI);
    }

    //sleep
    std::this_thread::sleep_for(std::chrono::seconds(noSeconds));
    finish = true;

    //join threads
    for (int i = 0; i < noPhilosophers; i++) {
        pthread_join(philosopherThread[i], NULL);
    }

    //output result
    ostringstream itemString;
    for (int i = 0; i < noPhilosophers; i++) {
        itemString << noFeedings[i] << ";";
    }
    string s = itemString.str();
    s = s.substr(0, s.size() - 1);
    printf("Result: %s\n", s.c_str());
    ofstream output;
    remove(fileName);
    output.open(fileName);
    output << s;
    output.close();

    exit(0);
}
