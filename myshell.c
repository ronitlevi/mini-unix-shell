#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include </home/caspl202/ESPL/LAB2/lineParser.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <bits/waitflags.h>
#include <ctype.h>


typedef struct process {
    cmdLine* cmd;         
    pid_t pid;               
    int status;                
    struct process *next;      
} process;

#define TERMINATED  -1
#define RUNNING      1
#define SUSPENDED    0

#define HISTLEN 20

char* history[HISTLEN];  
int newest = 0;       
int oldest = 0;      
int historyCount = 0;  


void addCommandToHistory(char* cmd){
    char* copy = strdup(cmd);
    if (historyCount < HISTLEN) {
        history[newest] = copy;
        historyCount++;
    } else {
        free(history[oldest]);
        history[oldest] = copy;
        oldest = (oldest + 1) % HISTLEN;
    }
    newest = (newest + 1) % HISTLEN;
    if (newest == oldest && historyCount == HISTLEN) {
        oldest = (oldest + 1) % HISTLEN;
    }
}

void printHistory() {
    int i = oldest;
    int index = 1;
    do {
        if (history[i] != NULL) {
            printf("%d: %s", index, history[i]);
            index++;
        }
        i = (i + 1) % HISTLEN;
    } while (i != newest);
}

void freeHistory() {
    for (int i = 0; i < HISTLEN; i++) {
        if (history[i] != NULL) {
            free(history[i]);
        }
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* new_process = (process*)malloc(sizeof(process));
    if (new_process == NULL){
        perror("malloc");
        exit(EXIT_FAILURE); 
    }
    new_process->cmd = cmd;
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = *process_list;
    *process_list = new_process;
}


void freeProcessList(process* process_list){
    if (process_list != NULL){
        process* toDelete = process_list;
        process_list = process_list->next;
        freeCmdLines(toDelete->cmd);
        free(toDelete);
    }
}

void updateProcessList(process** process_list){
    process* curr = *process_list;
    while (curr != NULL) {
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (result == -1) {
            perror("process doesn't exist");
            exit(EXIT_FAILURE);
        } else if (result == 0){

        } else {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                curr->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {
                curr->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {
                curr->status = RUNNING;
            }
        }
        curr = curr->next;
    }
}

void updateProcessStatus(process* process_list, int pid, int status){
    process* start = process_list;
    while (process_list != NULL) {
        if (process_list->pid == pid) {
            process_list->status = status;
            break;
        }
        process_list = process_list->next;
    }
    process_list = start;
}

void printProcessList(process** process_list){
    updateProcessList(process_list);
    int index = 0;
    process* curr = *process_list;
    process* prevCurr = NULL;
    while (curr != NULL){ 
        char* status = (curr->status == TERMINATED) ? "Terminated" : (curr->status == RUNNING) ? "Running" : "Suspended";
        fprintf(stdout, "%d\t%d\t" , index, curr->pid);
        for (int i = 0; i < curr->cmd->argCount; i++){             
            fprintf(stdout, "%s", curr->cmd->arguments[i]);
        }
        fprintf(stdout, "\t%s\n", status);
        process* temp = curr;
        curr = curr->next;
        if (temp->status == TERMINATED) {
            if (prevCurr != NULL) 
                prevCurr->next = temp->next;
            else 
                *process_list = temp->next; 
            freeCmdLines(temp->cmd);
            free(temp);
        } else {
            prevCurr = temp;
        }
        index++;
    }
}

void execute(cmdLine *pCmdLine, bool debugMode, process** process_list){
    int pipefd[2];    
    if (pCmdLine->next != NULL){
        if (pipe(pipefd) == -1){
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }
    }
    pid_t child2, child1 = fork();
    if (child1 == -1){
        perror("fork1");
        exit(EXIT_FAILURE);
    } else if (child1 == 0){
        if (pCmdLine->inputRedirect != NULL){
            FILE* fd = fopen(pCmdLine->inputRedirect, "r");
            if (fd == NULL){
                perror("open input");
                _exit(EXIT_FAILURE);
            }
            dup2(fileno(fd), STDIN_FILENO);
            fclose(fd);
        }
        if (pCmdLine->outputRedirect != NULL) {
            FILE* fd = fopen(pCmdLine->outputRedirect, "w");
            if (fd == NULL){
                perror("open output");
                _exit(EXIT_FAILURE);
            }
            dup2(fileno(fd), STDOUT_FILENO);
            fclose(fd);
        }
           if(pCmdLine->next != NULL){
            close(STDOUT_FILENO);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            close(pipefd[0]);
        }   
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
        if (debugMode){
            fprintf(stdout, "PID: %d\n", child1);
            fprintf(stdout, "executing command: %s\n", pCmdLine->arguments[0]);
        }
        exit(EXIT_SUCCESS);    
    } else { // parent
        addProcess(process_list, pCmdLine, child1);
        if(pCmdLine->next != NULL){
            child2 = fork();
            if (child2 == -1){
                perror("fork 2");
                exit(EXIT_FAILURE);
            } else if (child2 == 0){
                close(STDIN_FILENO);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
                if (execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments) == -1) {
                    perror("execvp");
                    _exit(EXIT_FAILURE);
                }
            } else {
                close(pipefd[0]);
                close(pipefd[1]);
                int status;
                if(pCmdLine->blocking == 1){
                    if (waitpid(child1, &status, 0) == -1){
                        perror("waitpid1");
                        exit(EXIT_FAILURE);
                    }
                    if (waitpid(child2, &status, 0) == -1){
                        perror("waitpid2");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        } else {
            int status;
            if(pCmdLine->blocking == 1){
                if (waitpid(child1, &status, 0) == -1){
                    perror("waitpid1");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

int main(int argc, char* argv[]){ 
    char input[2048];
    cmdLine* cmd; 
    char cwd[PATH_MAX]; 
    bool debugMode = false; 
    process* process_list = NULL;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debugMode = true;
    }

    while (1){

        if (isatty(fileno(stdout))){
            if(getcwd(cwd, sizeof(cwd)) != NULL){
                printf("%s\n", cwd);
            } else {
                perror("getcwd not working");
                exit(EXIT_FAILURE);
            }
        }

        if (fgets(input, 2048, stdin) == NULL){
            if (feof(stdin)){
                break;
            }
            perror("unable to read");
            exit(EXIT_FAILURE);
        }
        if (strncmp(input, "history", 7) != 0 && input[0] != '!') {
            addCommandToHistory(input);
        } else if (strncmp(input, "!!", 2) == 0) {
                if (historyCount > 0) {
                    strcpy(input, history[(newest - 1 + HISTLEN) % HISTLEN]);
                    addCommandToHistory(input);
            } else {
                printf("No commands in history.\n");
                continue;
            }
        } else if (input[0] == '!' && isdigit(input[1])) {
            int histNum = atoi(&input[1]) - 1;
            if (histNum < 0 || histNum >= historyCount) {
                printf("No such command in history.\n");
                continue;
            } else {
                    strcpy(input, history[(newest - historyCount + histNum) % HISTLEN]);
                    addCommandToHistory(input);
            }
        } 
        cmd = parseCmdLines(input);
        if (cmd == NULL) {
            freeCmdLines(cmd);
            continue;
        }
        if (cmd->argCount > 1 && strcmp(cmd->arguments[0], "cd") ==0){
            if (chdir(cmd->arguments[1]) == -1){
                fprintf(stderr, "cd operation failed\n");
            }
            freeCmdLines(cmd);
        } else if (cmd->argCount > 1 && strcmp(cmd->arguments[0], "nuke") ==0){
            pid_t pid = atoi(cmd->arguments[1]);
            if (kill(pid, SIGKILL) == 0) {
                updateProcessStatus(process_list, pid, TERMINATED);
            } else {
                perror("nuke failed");
            }
            freeCmdLines(cmd);
        } else if (cmd->argCount > 1 && strcmp(cmd->arguments[0], "wakeup") ==0){
            pid_t pid = atoi(cmd->arguments[1]);
            if (kill(pid, SIGCONT) == 0) {
                updateProcessStatus(process_list, pid, RUNNING);
            } else {
                perror("wakeup failed");
            }
            freeCmdLines(cmd);
        } else if (strcmp(cmd->arguments[0], "procs") ==0){
            fprintf(stdout, "ID\tPID\tCOMMAND\t\tSTATUS\n");
            printProcessList(&process_list);
            freeCmdLines(cmd);
        } else if (cmd->argCount > 1 && strcmp(cmd->arguments[0], "suspend") == 0) {
            pid_t pid = atoi(cmd->arguments[1]);
            if (kill(pid, SIGTSTP) == 0) {
                updateProcessStatus(process_list, pid, SUSPENDED);
            } else {
                perror("suspend failed");
            }
            freeCmdLines(cmd);
        } else if (strcmp(cmd->arguments[0], "history") == 0) {
            printHistory();
        } else {
            if (cmd != NULL){ 
                execute(cmd, debugMode, &process_list);
            }
            if (strcmp(cmd->arguments[0], "quit") == 0){
                freeCmdLines(cmd);
                freeProcessList(process_list);
                freeHistory();
                break;
            }
        }
    }
    freeProcessList(process_list);
    freeHistory();
    return 0;
}