//
// Created by alexander on 8/31/21.
//


#include <iostream>
using namespace std;
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* globalPrompt;
int backgroundProcessCount = 0;

extern char **environ;		/* environment info */

double timevalToMilliseconds(timeval t){
    return t.tv_sec * 1000 + t.tv_usec * 0.001;
}

double datetimeToWallclock(timeval start, timeval end){
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.001;
}

void printStatistics(rusage usage, timeval startTime, timeval endTime){
    struct timeval userTime = usage.ru_utime;
    struct timeval systemTime = usage.ru_stime;

    cout << "\nStatistics:\n";
    cout << " - System CPU Time Used: " << timevalToMilliseconds(systemTime)<< " milliseconds \n";
    cout << " - User CPU Time Used: " << timevalToMilliseconds(userTime) << " milliseconds \n";
    cout << " - Walk Clock Time Spent: " << datetimeToWallclock(startTime, endTime) << " milliseconds\n";
    cout << " - Involuntary Context Switches: " << usage.ru_nivcsw << "\n";
    cout << " - Voluntary Context Switches: " << usage.ru_nvcsw << "\n";
    cout << " - Major Page Faults: " << usage.ru_majflt << "\n";
    cout << " - Minor Page Faults: " << usage.ru_minflt << "\n";
    cout << " - Maximum Resident Set Size used: " << usage.ru_maxrss << "\n";
    cout.flush();
}

void forkAndRunCommand(char **argvNew, rusage *usage, timeval *startTime, timeval *endTime){
    int pid;
    if ((pid = fork()) < 0) {
        cerr << "Fork error\n";
        exit(1);
    } else if (pid == 0) {
        /* child process */
        gettimeofday(startTime, NULL);
        if (execvp(argvNew[0], argvNew) < 0) {
            cerr << "Execvp error\n";
            exit(1);
        }
        getrusage(RUSAGE_SELF, usage);

    } else {
        /* parent */
        wait(0);        /* wait for the child to finish */
        gettimeofday(endTime, NULL);
    }
}

void shellLine(char *prompt){
    char* cmd = new char[32];
    cout << prompt;
    cin.getline(cmd, 32);

    char *argvNew[32];
    int counter = 0;

    char* args = strtok(cmd, " ");

    if(strcmp(args, "exit") == 0){
        exit(0);
    }

    while (args != NULL)
    {
        //printf ("%s\n",args);
        argvNew[counter] = args;
        counter++;
        args = strtok (NULL, " ");
    }

    if(argvNew[0] != NULL && strcmp(argvNew[0], "set") == 0
        && argvNew[1] != NULL && strcmp(argvNew[1], "prompt") == 0
            && argvNew[2] != NULL && strcmp(argvNew[2], "=") == 0 ) {

        if(argvNew[3] != NULL){
            globalPrompt = argvNew[3];
            return;
        }
    }

    if(argvNew[0] != NULL && strcmp(argvNew[0], "cd") == 0) {
        if(argvNew[1] != NULL){
            char s[100];
            chdir(argvNew[1]);
            printf("Current Directory is Changed to %s.\n", getcwd(s, 100));
            return;
        }
    }

    for(int i = counter; i < 32; i++){
        argvNew[i] = NULL;
    }

    if(strcmp(argvNew[counter-1], "&") == 0) { // Background Task
        argvNew[counter-1] = NULL;
        int pid;
        backgroundProcessCount++;
        if ((pid = fork()) < 0) {
            cerr << "Fork error\n";
            exit(1);
        } else if (pid == 0) {
            /* child process */
            setpgid(0, 0);
            cout << "[" << backgroundProcessCount << "] " << getpid() << " " << argvNew[0] << "\n";
            cout.flush();
            if (execvp(argvNew[0], argvNew) < 0) {
                cerr << "Execvp error\n";
                exit(1);
            }

        } else {
            /* parent */
            wait(0);        /* wait for the child to finish */
        }
        return;
    }

    struct timeval startTime;
    struct timeval endTime;

    gettimeofday(&startTime, NULL);
    gettimeofday(&endTime, NULL);

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    forkAndRunCommand(argvNew, &usage, &startTime, &endTime);

    printStatistics(usage, startTime, endTime);
}

main(int argc, char *argv[]) {
/* argc -- number of arguments */
/* argv -- an array of strings */
    if(argc == 1){ // Shell / Part 2
        globalPrompt= "==>";
        while(true){
            shellLine(globalPrompt);
        }
    } else {
        struct timeval startTime;
        struct timeval endTime;

        gettimeofday(&startTime, NULL);
        gettimeofday(&endTime, NULL);

        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        char *argvNew[argc];
        for (int i = 1; i < argc; i++) {
            //cout << argv[i] << "\n";
            argvNew[i - 1] = argv[i];
        }
        argvNew[argc - 1] = NULL;

        forkAndRunCommand(argvNew, &usage, &startTime, &endTime);

        printStatistics(usage, startTime, endTime);
    }
}



