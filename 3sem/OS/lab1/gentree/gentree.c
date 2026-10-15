#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h>

int main(){
    pid_t pid;
    printf("PID: %d, PPID: %d\n", getpid(), getppid());

    pid = fork();
    if(pid == -1) { perror("Error fork"); exit(1); }
    else if (pid > 0) { 
        printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
        wait(NULL);
        printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid());
        exit(0);
    } else { 
        printf(" PID1: %d, PPID1: %d\n", getpid(), getppid());

        pid = fork();
        if(pid == -1) { perror("Error fork"); exit(1); }
        else if (pid > 0) { 
            printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
            wait(NULL);
            printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid());
            exit(0);
        } else { 
            printf(" PID2: %d, PPID2: %d\n", getpid(), getppid());

            pid = fork();
            if(pid == -1) { perror("Error fork"); exit(1); }
            else if (pid > 0) { 
                printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
                wait(NULL);
                
                pid = fork();
                if(pid == -1) { perror("Error fork"); exit(1); }
                else if (pid > 0) { 
                    printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
                    wait(NULL);
                    printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid());
                    exit(0);
                } else { 
                    printf(" PID4: %d, PPID4: %d\n", getpid(), getppid());
                    
                    pid = fork();
                    if(pid == -1) { perror("Error fork"); exit(1); }
                    else if (pid > 0) { 
                        printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
                        wait(NULL);
                        printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid());
                        exit(0);
                    } else {
                        printf(" PID6: %d, PPID6: %d\n", getpid(), getppid());
                        
                        pid = fork();
                        if(pid == -1) { perror("Error fork"); exit(1); }
                        else if (pid > 0) { 
                            printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
                            wait(NULL);
                            printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid());
                            exit(0);
                        } else { 
                            printf(" PID7: %d, PPID7: %d\n", getpid(), getppid());
                            execlp("whoami", "whoami", "--version", (char *)NULL);
                            perror("execlp failed");
                            exit(1);
                        }
                    }
                }
            } else { 
                printf(" PID3: %d, PPID3: %d\n", getpid(), getppid());
                
                pid = fork();
                if(pid == -1) { perror("Error fork"); exit(1); }
                else if (pid > 0) { 
                    printf("Процесс с ID %d породил процесс с ID %d\n", getpid(), pid);
                    wait(NULL);
                    printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid()); 
                    exit(0);
                } else { 
                    printf(" PID5: %d, PPID5: %d\n", getpid(), getppid());
                    printf("Процесс с ID %d и ID родителя %d завершает работу\n", getpid(), getppid());
                    exit(0);
                }
            }
        }
    }
    return 0;
}