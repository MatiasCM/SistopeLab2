#include "funciones.h"

// pipe para comunicarse con el padre
int *fd_padre;

// pipe para escuchar al padre
int *fd2_padre;

int** crear_hijos(int cantidad){
    int **pipes_padrehijo = (int**)malloc(cantidad * sizeof(int*));
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
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Hijo

            close(pipes_padrehijo[j][1]);
            fd2_padre = pipes_padrehijo[j];
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
        int *fd_in = (int*)malloc(sizeof(int)*2);
        int *fd_out = (int*)malloc(sizeof(int)*2);

        read(fd2_padre[0], fd_in, sizeof(fd_in));
        dup2(fd_in[0], STDIN_FILENO);

        char respuesta;
        write(fd_padre[1], &respuesta, 1);

        read(fd2_padre[0], fd_out, sizeof(fd_out));
        dup2(fd_out[1], STDOUT_FILENO);

        write(fd_padre[1], &respuesta, 1);
        
        exit(0);
    }
    else{
        int **anillo = (int**)malloc(hijos * sizeof(int*));
        for(int i = 0; i < hijos; i++){
            anillo[i] = (int*)malloc(sizeof(int)*2);
        }
        if (!pipes_padrehijo || !anillo) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i < hijos; i++){
            anillo[i] = (int*)malloc(sizeof(int)*2);
        }
        for(int j = 0; j < hijos; j++){
            if(pipe(anillo[j]) == -1){
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            //  7      1       2       3       4       5       6      7
            // -> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 -
    
            if(j == 0){
                // dup2(anillo[hijos-1][0], STDIN_FILENO);
                write(pipes_padrehijo[j][1], anillo[hijos-1], sizeof(anillo[hijos-1]));
                char espera;
                read(fd_padre[0], &espera, 1);

                // dup2(anillo[j][1], STDOUT_FILENO);
                write(pipes_padrehijo[j][1], anillo[j], sizeof(anillo[j]));
                read(fd_padre[0], &espera, 1);
            }
            else{
                // dup2(anillo[j-1][0], STDIN_FILENO);
                write(pipes_padrehijo[j][1], anillo[j-1], sizeof(anillo[j-1]));
                char espera;
                read(fd_padre[0], &espera, 1);

                // dup2(anillo[j][1], STDOUT_FILENO);
                write(pipes_padrehijo[j][1], anillo[j], sizeof(anillo[j]));
                read(fd_padre[0], &espera, 1);
            }
        }
        return;
    }
}