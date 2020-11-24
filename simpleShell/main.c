#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>


const int MAX_LENGTH = 4096;    // max length of command
const char* file = "log.txt";   // file to log
FILE *fptr;                     // global pointer for the file


// get Time
char* getTime() {

    struct tm* local;
    time_t t = time(NULL);

    local = localtime(&t);

    return asctime(local);
}



void logger(pid_t pid, int status) {

    fprintf(fptr, "Child Proccess %d was terminated with status %d at %s", pid, status, getTime());
}



void interruptHandler (int signNum) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        logger(pid, status);
    }

}



char** splitCommand(char* command) {

    // using strtok to split with white space
    char* splitted = strtok(command, " ");
    char** commandList = NULL; // array to contains splitted string
    int argc = 0;

    // while there is still some strings, add to the array
    while (splitted) {


        if (*splitted == '\n' || *splitted == ' ' || *splitted == '\0') {
            splitted = strtok(NULL, " "); // next string
            continue;
        }
        // reallocate some memory for the new string pointer
        commandList = realloc(commandList, sizeof(char*) * (++argc));

        // not possible
        if (commandList == NULL) {
            return NULL;
        }

        // add to the array
        commandList[argc - 1] = splitted;
        splitted = strtok(NULL, " "); // next string
    }

    // remove the new line '\n'
    if (argc > 0) {
        int ind = 0;
        while (*(commandList[argc - 1] + ind) != '\n' && *(commandList[argc - 1] + ind) != '\0') {
            ind++;
        }
        if (*(commandList[argc - 1] + ind) == '\n')
            commandList[argc - 1][ind] = '\0';
    }

    // add the null to the end
    commandList = realloc(commandList, sizeof(char*) * (++argc));

    if (commandList == NULL) {
        return NULL;
    }

    commandList[argc - 1] = NULL;


    return commandList;
}


void forkProcess(char* args[], bool wait_b) {

    // make a fork
    pid_t pid = fork();
    int status; //status of child


    // if it failed
    if (pid < 0) {
        printf("Internal Error.\n");
        _Exit(3);
    }
    // if it is the child
    else if (pid == 0) {
        execvp(args[0], args);
        printf("Command not found\n");
        _Exit(3);
    }
    // if it is the parent
    else {
        if (wait_b) { // if it is not a background process, we need to wait
            waitpid(pid, &status, NULL);
            logger(pid, status);
        }
    }
}



int main()
{
    char* command = (char* )malloc(MAX_LENGTH);
    signal(SIGCHLD, interruptHandler);

    // clear past logs
    fptr = fopen(file, "w");
    if (fptr == NULL) {
        printf("Can not start logging proccesses\n");
        exit(1);
    }

    fprintf(fptr, "");
    fclose(fptr);

    // open file again but with appending
    fptr = fopen(file, "a");


    // starting the shell
    fprintf(fptr, "Starting new Shell at %s", getTime());


    while (true) {

        printf("shell >> ");

        // get the command line
        int size = getline(&command, &MAX_LENGTH, stdin);

        // if it is empty
        if (!strcmp(command, "\n"))
            continue;

        // checking if it is background process and remove the &
        bool backGroundProccess = (size >= 2 && command[size - 2] == '&');
        if (backGroundProccess) {
            command[size - 2] = command[size - 1] = '\0';
        }


        // splitting the command to array of pointers(strings)
        char** arr = splitCommand(command);

        if (!strcmp(arr[0], "exit") || arr == NULL) // if it 'exit'
            break;


        // pass the parsed command to exec
        forkProcess(arr, !backGroundProccess);
    }

    free(command);


    fprintf(fptr, "Closing the shell at %s", getTime());

    // close open file
    fclose(fptr);

    return 0;
}
