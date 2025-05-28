#include "funciones.h"

// pipe para comunicarse con el padre
int *fd_padre;

// pipe para escuchar al padre
int *fd2_padre;

int** crear_hijos(int cantidad, int **anillo){
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
    if (!pipes_padrehijo || !anillo) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < cantidad; i++){
        pipes_padrehijo[i] = (int*)malloc(sizeof(int)*2);
        anillo[i] = (int*)malloc(sizeof(int)*2);
    }

    // abre todas las pipes del anillo antes de crear a los hijos
    for(int j = 0; j < cantidad; j++){
        if(pipe(anillo[j]) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
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
            
            hijos_esperan_conexiones();

            return NULL;
        }
        else{
            close(pipes_padrehijo[j][0]);
        }
    }
    close(fd_padre[1]);
    return pipes_padrehijo;
}

void ciclo_token(int max_decre, int id_logico) {
    int token;

    // Mensaje de depuración para confirmar que el hijo empieza
    fprintf(stderr, "Proceso %d (PID %d) empezó\n", id_logico, getpid());

    while (1) {
        ssize_t bytes = read(STDIN_FILENO, &token, sizeof(int));

        if (bytes == 0) {
            // Pipe cerrado, fin del juego
            fprintf(stderr, "Proceso %d detectó cierre de pipe\n", id_logico);
            break;
        } else if (bytes < 0) {
            perror("read");
            break;
        }

        fprintf(stderr, "Proceso %d recibió token: %d\n", id_logico, token);
        token = token - 5;

        //sleep(1);

        if (token < 0) {
            fprintf(stderr, "Proceso %d termina\n", id_logico);
            close(STDOUT_FILENO); // Importante para que el siguiente vea EOF
            break;
        }

        write(STDOUT_FILENO, &token, sizeof(int));
    }
}

void hijos_esperan_conexiones(){
    int *fd_in = (int*)malloc(sizeof(int)*2);
    int *fd_out = (int*)malloc(sizeof(int)*2);
        
    read(fd2_padre[0], fd_in, sizeof(int)*2);
    dup2(fd_in[0], STDIN_FILENO);

    char respuesta;
    write(fd_padre[1], &respuesta, 1);

    read(fd2_padre[0], fd_out, sizeof(int)*2);
    dup2(fd_out[1], STDOUT_FILENO);

    write(fd_padre[1], &respuesta, 1);

    close(fd_in[1]);
    close(fd_out[0]);

    int id_logico, decrecimiento;

    read(fd2_padre[0], &id_logico, sizeof(int));

    write(fd_padre[1], &respuesta, 1);

    read(fd2_padre[0], &decrecimiento, sizeof(int));

    ciclo_token(decrecimiento, id_logico);
    exit(0);
}

void conectar_hijos(int **pipes_padrehijo, int **anillo, int hijos, int token, int decrecimiento){
    for(int j = 0; j < hijos; j++){
        //  7      1       2       3       4       5       6      7
        // -> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 -
        if(j == 0){
            // dup2(anillo[hijos-1][0], STDIN_FILENO);
            write(pipes_padrehijo[j][1], anillo[hijos-1], sizeof(int)*2);
            char espera;
            read(fd_padre[0], &espera, 1);

            // dup2(anillo[j][1], STDOUT_FILENO);
            write(pipes_padrehijo[j][1], anillo[j], sizeof(int)*2);
            read(fd_padre[0], &espera, 1);
            
            // le pasa el id logico y el decrecimiento
            write(pipes_padrehijo[j][1], &j, sizeof(int));
            read(fd_padre[0], &espera, 1);
            
            write(pipes_padrehijo[j][1], &decrecimiento, sizeof(int));
        }
        else{
            // dup2(anillo[j-1][0], STDIN_FILENO);
            write(pipes_padrehijo[j][1], anillo[j-1], sizeof(int)*2);
            char espera;
            read(fd_padre[0], &espera, 1);

            // dup2(anillo[j][1], STDOUT_FILENO);
            write(pipes_padrehijo[j][1], anillo[j], sizeof(int)*2);
            read(fd_padre[0], &espera, 1);
            
            // le pasa el id logico y el decrecimiento
            write(pipes_padrehijo[j][1], &j, sizeof(int));
            read(fd_padre[0], &espera, 1);
            
            write(pipes_padrehijo[j][1], &decrecimiento, sizeof(int));
        }
    }

    // Enviar token inicial
    fprintf(stderr, "Padre envía token: %d\n", token);
    if (write(anillo[0][1], &token, sizeof(int)) <= 0) {
        perror("write token");
    }

    // Cerrar todos los pipes en el padre
    for (int i = 0; i < hijos; i++) {
        close(anillo[i][0]);
        close(anillo[i][1]);
    }

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < hijos; i++) {
        wait(NULL);
    }
    return;
}
