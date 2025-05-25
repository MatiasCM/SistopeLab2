#include "funciones.h"

int *fd_sig;

int *fd_padre;

int** crear_hijos(int cantidad){
    int **pipes_padrehijo = (int**)malloc(cantidad * sizeof(int*));
    // pipe que escuchara a los todos hijos
    fd_padre = (int*)malloc(sizeof(int)*2);
    if(!fd_padre){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if(pipe(fd_padre) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < cantidad; i++){
        pipes_padrehijo[i] = (int*)malloc(sizeof(int)*2);
    }
    if (!pipes_padrehijo) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for(int j = 0; j < cantidad; j++){
        if(pipe(pipes_padrehijo[j]) == -1){
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Hijo
            // se redirige la lectura de la pipe al STDOUT del hijo
            dup2(pipes_padrehijo[j][0], STDIN_FILENO);
            close(pipes_padrehijo[j][0]);
            close(pipes_padrehijo[j][1]);
            close(fd_padre[0]);
            conectar_hijos(NULL, cantidad, pid);
            return NULL;
        }
        else{
            close(pipes_padrehijo[j][0]);
        }
    }
    close(fd_padre[1]);
    return pipes_padrehijo;
}

void conectar_hijos(int **pipes_padrehijo, int hijos, pid_t pid){
    if(pid == 0){
        char buffer[100];
        read(STDIN_FILENO, buffer, sizeof(buffer));
        printf("%s\n", buffer);
        exit(0);
    }
    else{
        for(int i = 0; i < hijos; i++){
            char buffer[100];
            sprintf(buffer, "%d", getpid());
            write(pipes_padrehijo[i][1], buffer, sizeof(buffer));
        }
        return;
    }

    /*
    union sigval value2;
    for (int i = 0; i < hijos; i++) {
        if(i == (hijos-1)){
            value2.sival_int = (pids[0]*10) + 1;
            if (sigqueue(pids[i], SIGUSR2, value2) == -1) {
                perror("sigqueue");
            }
        }
        else{
            value2.sival_int = (pids[i+1]*10) + 1;
            if (sigqueue(pids[i], SIGUSR2, value2) == -1) {
                perror("sigqueue");
            }
        }
        
    }*/
}