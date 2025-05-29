#include "funciones.h"

// pipe para comunicarse con el padre
int *fd_padre;

// pipe para escuchar al padre por cambios en los fd
int *fd2_padre;

int id;

int procesos_restantes;

// Entradas:    Número de procesos hijos a crear (cantidad), arreglo de pipes para comunicación en anillo (**anillo)
//
// Salidas:     Retorna un puntero a un arreglo de pipes
//
// Descripción: Crea los procesos hijos con fork(), configura pipes para comunicación entre padre e hijos y
//              entre hijos en forma de anillo, y prepara los procesos para comenzar la ejecución
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
            dup2(pipes_padrehijo[j][0], STDIN_FILENO);
            dup2(fd_padre[1], STDOUT_FILENO);

            char cantidad_str[20];
            char id_str[20];
            snprintf(cantidad_str, sizeof(cantidad_str), "%d", cantidad);
            snprintf(id_str, sizeof(id_str), "%d", j);

            char *args[] = {"./hijos", cantidad_str, id_str, NULL};

            execv("./hijos", args);

            return NULL;
        }
        else{
            close(pipes_padrehijo[j][0]);
        }
    }
    // el padre espera a que los hijos esten listos para inicializarlos luego del exec
    char espera;
    for(int i = 0; i < cantidad; i++){
        read(fd_padre[0], &espera, 1);
       
    }
    for(int j = 0; j < cantidad; j++){
        write(pipes_padrehijo[j][1], fd_padre, sizeof(int)*2);
        write(pipes_padrehijo[j][1], pipes_padrehijo[j], sizeof(int)*2);
        close(pipes_padrehijo[j][0]);
    }
    close(fd_padre[1]);
    return pipes_padrehijo;
}


// Entradas:    Valor máximo de decrecimiento aleatorio para el token (max_decre), Identificador lógico del proceso hijo (id_logico)
//
// Salidas:     No retorna nada
//
// Descripción: Función ejecutada por cada proceso hijo que recibe el token, calcula su decrecimiento,
//              decide si continúa o termina y comunica con los demás procesos.
//              
//              Si accion es 1, recibe el token, calcula un decrecimiento aleatorio y envía el nuevo token o termina si el token es negativo
//              Si accion es 2, recibe la notificación de que el proceso anterior ha salido, ajusta el anillo y notifica a los demás procesos
//              Si accion es 3, se recibe la notificacion de la salida de un procesos en el resto del anillo
//              Si accion es 4, el proceso es el líder y espera recibir el token del padre
void ciclo_token(int max_decre, int id_logico) {
    int accion, token;
    int entrada[2];
    while (1) {
        read(STDIN_FILENO, entrada, sizeof(int)*2);
        wait(NULL);
        accion = entrada[0];
        // sigue el juego
        if(accion == 1){
            token = entrada[1];
            int decrecimiento = rand() % max_decre;
            int nuevo_token = token - decrecimiento;
            if(nuevo_token < 0){
                fprintf(stderr, "Proceso %d ; Token recibido: %d ; Token resultante: %d (Proceso %d es eliminado)\n", id, token, nuevo_token, id);
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
                fprintf(stderr, "Proceso %d ; Token recibido: %d ; Token resultante: %d\n", id, token, nuevo_token);
                int salida[2];
                salida[0] = 1;
                salida[1] = nuevo_token;
                write(STDOUT_FILENO, salida, sizeof(int)*2);
            }
        }
        // aviso de salida del proceso anterior
        else if(accion == 2){
            int id_logico_quien_sale = entrada[1];
            int *fd_in = (int*)malloc(sizeof(int)*2);
            read(fd2_padre[0], fd_in, sizeof(int)*2);
            dup2(fd_in[0], STDIN_FILENO);
            close(fd_in[0]);
            close(fd_in[1]);
            if(id_logico > 0){
                id_logico = id_logico - 1;
            }
            procesos_restantes = procesos_restantes - 1;
            char respuesta;
            write(fd_padre[1], &respuesta, 1);
            if(procesos_restantes <= 1){
                int termino = 1;
                write(fd_padre[1], &termino, sizeof(int));
                fprintf(stderr, "Proceso %d es el ganador\n", id);
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
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
            procesos_restantes = procesos_restantes - 1;
            int id_logico_quien_sale = entrada[1];
            if(id_logico_quien_sale < id_logico){
                id_logico = id_logico - 1;
            }
            // si son iguales quiere decir que ya se dio una vuelta en el anillo y todos los hijos estan avisados de la salida
            // o si el que le sigue es el que salio quiere decir que salio el proceso con el id_logico mas grande
            if(id_logico != id_logico_quien_sale && (id_logico+1) != (id_logico_quien_sale)){
                int salida[2];
                salida[0] = 3;
                salida[1] = id_logico_quien_sale;
                write(STDOUT_FILENO, salida, sizeof(int)*2);
            }
            else{
                char respuesta;
                write(fd_padre[1], &respuesta, 1);
            }
        }
        // es lider
        else if(accion == 4){
            // el lider espera el token del padre
            int token_r;
            read(STDIN_FILENO, &token_r, sizeof(int));
            int salida[2];
            salida[0] = 1;
            salida[1] = token_r;
            write(STDOUT_FILENO, salida, sizeof(int)*2);
        }
    }
}

// Entradas:    Número total de procesos hijos (cantidad), identificador lógico del proceso actual (id_proceso)
//
// Salidas:     No retorna nada
//
// Descripción: Recibe y configura las pipes para comunicación entre procesos y llama a hijos_esperan_conexiones para continuar
void inicializacion_hijos(int cantidad, int id_proceso){
    procesos_restantes = cantidad;
    id = id_proceso;
    char respuesta = 0;
    fd_padre = (int*)malloc(sizeof(int)*2);
    fd2_padre = (int*)malloc(sizeof(int)*2);
    write(STDOUT_FILENO, &respuesta, 1);
    read(STDIN_FILENO, fd_padre, sizeof(int)*2);
    read(STDIN_FILENO, fd2_padre, sizeof(int)*2);
    close(fd2_padre[1]);
    close(fd_padre[0]);
    hijos_esperan_conexiones();
}

// Entradas:    No recibe nada
//
// Salidas:     No retorna nada
//
// Descripción: Hace que el proceso hijo espere las conexiones necesarias de sus hermanos,
//              configura sus pipes para comunicación con los vecinos en el anillo y gestiona la comunicación
//              para recibir el token y el decrecimiento, y luego inicia el ciclo de token
void hijos_esperan_conexiones(){
    if(procesos_restantes == 1){
        exit(0);
    }
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

    id = id_logico;

    write(fd_padre[1], &respuesta, 1);

    read(fd2_padre[0], &decrecimiento, sizeof(int));

    ciclo_token(decrecimiento, id_logico);
    exit(0);
}

// Entradas:    Arreglo de pipes padre-hijo para cada proceso (**pipes_padrehijo), arreglo de pipes que forman el anillo entre hijos (**anillo),
//              cantidad de procesos hijos (hijos), valor inicial del token (token), máximo decrecimiento para el token (decrecimiento)
//
// Salidas:     No retorna nada
//
// Descripción: Conecta todos los hijos entre sí configurando el anillo de comunicación, inicializa el
//              envío del token y maneja la eliminación de procesos cuando el token decrece a menos de 0,
//              ajustando las conexiones en el anillo conforme se eliminan procesos.
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
    int salida[2];
    salida[0] = 1;
    salida[1] = token;
    if (write(anillo[0][1], salida, sizeof(int)*2) <= 0) {
        perror("write token");
    }

    // Esperar a que todos los hijos terminen
    while(hijos > 1){
        int id_logico_hijo;
        read(fd_padre[0], &id_logico_hijo, sizeof(int));
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

        close(pipes_padrehijo[id_logico_hijo][0]);
        close(pipes_padrehijo[id_logico_hijo][1]);
        close(anillo[id_logico_hijo][0]);
        close(anillo[id_logico_hijo][1]);

        free(pipes_padrehijo[id_logico_hijo]);
        free(anillo[id_logico_hijo]);
        //int j = 0;
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
            wait(NULL);
            return;
        }
        // espera a que todos los procesos sepan de la salida del hermano
        read(fd_padre[0], &espera, 1);
        int salida_lider[2];
        salida_lider[0] = 4;
        salida_lider[1] = hijos;
        // selecciona el lider
        // mecanismo de eleccion de lider, selecciona el proceso anterior al que sale
        if(id_logico_hijo == 0){
            // sale el primero entonces el ultimo es el anterior
            write(anillo[hijos-1][1], salida_lider, sizeof(int)*2); 
            // le envia el token al lider
            write(anillo[hijos-1][1], &token, sizeof(int));
        }
        else{
            write(anillo[id_logico_hijo-1][1], salida_lider, sizeof(int)*2); 
            // le envia el token al lider
            write(anillo[id_logico_hijo-1][1], &token, sizeof(int));
        }
    }
}
