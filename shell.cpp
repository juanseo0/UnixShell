#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
using namespace std;

#define MAX_LINE 80 /* The maximum length command */

int main(void) {
    char *args[MAX_LINE/2 + 1];
    int should_run = 1;
    char cmdHistory[MAX_LINE] = "";

    while (should_run) {
        printf("osh>");
        fflush(stdout);
        //User input
        char cmdInput[MAX_LINE];
        //Checks for special operators
        bool concurrentCheck = false;
        bool pipeCheck = false;
        bool inputCheck = false;
        bool outputCheck = false;
        //Used for special operators
        char *args_Pipe[MAX_LINE/2 + 1] = {0};
        char *inputFileName = NULL;
        char *outputFileName = NULL;
        //Read Input
        if (fgets(cmdInput, MAX_LINE, stdin) == NULL) { 
            break;
        }
        cmdInput[strcspn(cmdInput, "\n")] = 0;
        //History
        if (strcmp(cmdInput, "!!") == 0) {
            if (cmdHistory[0] == '\0') { //No History
                cout << "No commands in history." << endl;
                continue;
            } else { //Has History
                cout << cmdHistory << endl;
                strcpy(cmdInput, cmdHistory);
            }
        } else { //Copy to History
            strcpy(cmdHistory, cmdInput);
        }
        //Break cmdInput into Args
        bool endLoop = false;
        int wordCounter = 0;
        char *temp = strtok(cmdInput, " ");
        while (temp != NULL && !endLoop) {
            if (strcasecmp(temp, "<") == 0) { // Input Check
                inputCheck = true;
                temp = strtok(NULL, " ");
                if (temp == NULL) {
                    cout << "No input file entered!" << endl;
                    endLoop = true;
                } else {
                    inputFileName = temp;
                    temp = strtok(NULL, " ");
                }
            } else if (strcasecmp(temp, ">") == 0) { // Output Check
                outputCheck = true;
                temp = strtok(NULL, " ");
                if (temp == NULL) {
                    cout << "No output file entered!" << endl;
                    endLoop = true;
                } else {
                    outputFileName = temp;
                    temp = strtok(NULL, " ");
                }
            } else if (strcasecmp(temp, "|") == 0) { // Pipe Check
                args[wordCounter] = NULL;
                pipeCheck = true;
                int pipeWordCounter = 0;
                temp = strtok(NULL, " ");
                while (temp != NULL) {
                    args_Pipe[pipeWordCounter] = temp;
                    temp = strtok(NULL, " ");
                    pipeWordCounter++;
                }
                args_Pipe[pipeWordCounter] = NULL;
                break;
            } else if (strcmp(temp, "&") == 0) { // Concurrent Check
                concurrentCheck = true;
                args[wordCounter] = NULL;
                endLoop = true;
            } else {
                args[wordCounter] = temp;
                wordCounter++;
                temp = strtok(NULL, " ");
            }
        }
        args[wordCounter] = NULL;
        //No Command Entered Check
        if (args[0] == NULL) {
            cout << "No command entered." << endl;
            continue;
        }
        //Exit Check
        if (strcasecmp(args[0], "exit") == 0) {
            cout << "Exiting Shell" << endl;
            should_run = 0;
            exit(0);
        }
        //Command Execution
        pid_t pid = fork();
        if (pid < 0) { // Error Forking
            fprintf(stderr, "Fork failed");
            return 1;
        } else if (pid == 0) { // Child Process
            if (pipeCheck) { // Pipe Command
                int pipeFD[2];
                pipe(pipeFD);
                pid_t pid_Pipe = fork();
                if (pid_Pipe < 0) { // Error Forking
                    fprintf(stderr, "Fork failed in Pipe");
                    return 1;
                }
                if (pid_Pipe == 0) { // Pipe Child Process
                    close(pipeFD[0]);
                    dup2(pipeFD[1], STDOUT_FILENO);
                    execvp(args[0], args);
                    fprintf(stderr, "Execvp failed in Pipe\n");
                    return 1;
                } else { // Pipe Parent Process
                    close(pipeFD[1]);
                    wait(NULL);
                    dup2(pipeFD[0], STDIN_FILENO);
                    execvp(args_Pipe[0], args_Pipe);
                    fprintf(stderr, "Execvp failed in Pipe\n");
                    return 1;
                }
            } else { // Single Command Execution Child Process
                if (inputCheck) { // Input File Check
                    int fileCheck = open(inputFileName, O_RDONLY);
                    if (fileCheck == -1) {
                        fprintf(stderr, "Input file could not be opened\n");
                        return 1;
                    } else {
                        dup2(fileCheck, STDIN_FILENO);
                        close(fileCheck);
                    }
                }
                if (outputCheck) { // Output File Check
                    int fileCheck = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
                    if (fileCheck == -1) {
                        fprintf(stderr, "Output file could not be opened\n");
                        return 1;
                    } else {
                        dup2(fileCheck, STDOUT_FILENO);
                        close(fileCheck);
                    }
                }
                execvp(args[0], args);
                fprintf(stderr, "Execvp failed\n");
                return 1;
            }
        } else { // Parent Process
            if (!concurrentCheck) { // Concurrent Check
                wait(NULL);
            }
        }
    }
    return 0;
}
