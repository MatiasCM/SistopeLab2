#include "funciones.h"

// pipe para comunicarse con el padre
int *fd_padre;

// pipe para escuchar al padre
int *fd2_padre;

int procesos_restantes;

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
    // establece los hijos que jugaran
    procesos_restantes = cantidad;

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
    int accion, token;
    int entrada[2];
    while (1) {
        read(STDIN_FILENO, entrada, sizeof(int)*2);
        accion = entrada[0];
        // sigue el juego
        if(accion == 1){
            token = entrada[1];
            fprintf(stderr, "Proceso %d recibió token: %d\n", id_logico, token);
            int decrecimiento = rand() % max_decre;
            token = token - decrecimiento;
            if(token < 0){
                fprintf(stderr, "Proceso %d termina\n", id_logico);
                write(fd_padre[1], &id_logico, sizeof(int));
                int salida[2];
                salida[0] = 2;
                salida[1] = id_logico;
                write(STDOUT_FILENO, salida, sizeof(int)*2);
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                exit(0);
            }
            else{
                int salida[2];
                salida[0] = 1;
                salida[1] = token;
                write(STDOUT_FILENO, salida, sizeof(int)*2);
            }
        }
        // aviso de salida del proceso anterior
        else if(accion == 2){
            int id_logico_quien_sale = entrada[1];
            fprintf(stderr, "Soy %d y salio %d\n", id_logico, id_logico_quien_sale);
            int *fd_in = (int*)malloc(sizeof(int)*2);
            read(fd2_padre[0], fd_in, sizeof(int)*2);
            dup2(fd_in[0], STDIN_FILENO);
            close(fd_in[0]);
            close(fd_in[1]);
            if(id_logico > 0){
                id_logico = id_logico - 1;
            }
            procesos_restantes = procesos_restantes - 1;
            fprintf(stderr, "AHora soy %d\n", id_logico);
            char respuesta;
            write(fd_padre[1], &respuesta, 1);
            if(procesos_restantes == 1){
                int termino = 1;
                write(fd_padre[1], &termino, sizeof(int));
                fprintf(stderr, "ganeeeee\n");
                exit(0);
            }
            else{
                int termino = 0;
                write(fd_padre[1], &termino, sizeof(int));
            }
            int salida[2];
            salida[0] = 3;
            salida[1] = id_logico_quien_sale;
            write(STDOUT_FILENO, salida, sizeof(int)*2);
        }
        // aviso de salida de un proceso
        else if(accion == 3){
            int id_logico_quien_sale = entrada[1];
            fprintf(stderr, "Soy %d y salio %d\n", id_logico, id_logico_quien_sale);
            if(id_logico_quien_sale < id_logico){
                id_logico = id_logico - 1;
                procesos_restantes = procesos_restantes - 1;
            }
            fprintf(stderr, "Ahora soy %d\n", id_logico);
            // si son iguales quiere decir que ya se dio una vuelta en el anillo y todos los hijos estan avisados de la salida
            // o si el que le sigue es el que salio quiere decir que salio el proceso con el id_logico mas grande
            if(id_logico != id_logico_quien_sale && (id_logico+1) != (id_logico_quien_sale)){
                int salida[2];
                salida[0] = 3;
                salida[1] = id_logico_quien_sale;
                write(STDOUT_FILENO, salida, sizeof(int)*2);
            }
            else{
                fprintf(stderr, "ya se nos aviso a todos %d\n", id_logico);
                char respuesta;
                write(fd_padre[1], &respuesta, 1);
            }
        }
        // es lider
        else if(accion == 4){
            // el lider se comunica con el padre
            int respuesta;
            int procesos_restantes = entrada[1];
            if(procesos_restantes <= 1){
                respuesta = 1;
            }
            else{
                respuesta = 0;
            }
            write(fd_padre[1], &respuesta, sizeof(int));
            int token_r;
            read(STDIN_FILENO, &token_r, sizeof(int));
            int salida[2];
            salida[0] = 1;
            salida[1] = token_r;
            write(STDOUT_FILENO, salida, sizeof(int)*2);
        }
    }
}

void hijos_esperan_conexiones(){
    srand(time(NULL));
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
    int salida[2];
    salida[0] = 1;
    salida[1] = token;
    if (write(anillo[0][1], salida, sizeof(int)*2) <= 0) {
        perror("write token");
    }

    /*// Cerrar todos los pipes en el padre
    for (int i = 0; i < hijos; i++) {
        close(anillo[i][0]);
        close(anillo[i][1]);
    }*/

    // Esperar a que todos los hijos terminen
    while(1){
        int id_logico_hijo;
        read(fd_padre[0], &id_logico_hijo, sizeof(int));
        // si el id_logico enviado por el hijo es -1, significa que es el ganador
        //  7      1       2       3       4       5       6      7
        // -> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 -
        //   muere y pipe 1 se va
        //  7      2       3       4       5       6      7
        // -> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 --> 0 1 -
        if(id_logico_hijo == 0){
            write(pipes_padrehijo[1][1], anillo[hijos-1], sizeof(int)*2);
        }
        else if(id_logico_hijo == (hijos-1)){
            write(pipes_padrehijo[0][1], anillo[id_logico_hijo-1], sizeof(int)*2);
        }
        else{
            write(pipes_padrehijo[id_logico_hijo+1][1], anillo[id_logico_hijo-1], sizeof(int)*2);
        }
        
        // cierre y eliminacion de las pipes que se comunican con el proceso eliminado
        
        free(pipes_padrehijo[id_logico_hijo]);
        free(anillo[id_logico_hijo]);
        int j = 0;
        for (int i = id_logico_hijo; i < hijos - 1; i++) {
            pipes_padrehijo[i] = pipes_padrehijo[i + 1];
            anillo[i] = anillo[i + 1];
        }
        hijos--;

        char espera;
        // espera a que el proceso siguiente al eliminado haya cambiado su STDIN_FILENO
        read(fd_padre[0], &espera, 1);

        int espera_y_termino;
        read(fd_padre[0], &espera_y_termino, sizeof(int));
        if(espera_y_termino){
            return;
        }
        // espera a que todos los procesos sepan de la salida del hermano
        read(fd_padre[0], &espera, 1);
        int salida_lider[2];
        salida_lider[0] = 4;
        salida_lider[1] = hijos;
        // selecciona el lider
        write(anillo[0][1], salida_lider, sizeof(int)*2);
        // espera a que el lider espere el recibo del token
        int respuesta_lider;
        read(fd_padre[0], &respuesta_lider, sizeof(int));
        // el lider notifica que si es el ganador o no
        if(respuesta_lider){
            return;
        }
        // le envia el token al lider
        write(anillo[0][1], &token, sizeof(int));
    }
}
