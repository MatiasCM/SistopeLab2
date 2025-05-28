#include "funciones.h"

void ciclo_token(int max_decre, int id_logico) {
    int token;
    int primera_lectura = 1;  // Bandera para indicar si es la primera vez que el proceso recibe el token

    //fprintf(stderr, "Proceso %d empezó\n", id_logico);

    srand(time(NULL));

    while (1) {
        // Leer el token desde el pipe de entrada (esto bloquea hasta que haya algo que leer)
        ssize_t datos = read(STDIN_FILENO, &token, sizeof(int));

        // Si no hay más datos (el pipe se cierra), el proceso termina
        if (datos == 0) {
            //fprintf(stderr, "Proceso %d detectó cierre de pipe\n", id_logico);
            exit(255);
        } else if (datos < 0) {
            perror("read");
            exit(255);
        }

        // Si es el primer proceso, imprimimos que ha recibido el token
        if (primera_lectura && id_logico == 0) {
            fprintf(stderr, "Proceso %d recibió token: %d\n", id_logico, token);
            primera_lectura = 0;
        } else {
            fprintf(stderr, "Proceso %d recibió token: %d\n", id_logico, token);
        }

        // Resta un valor aleatorio entre 0 y M-1 al token
        int decre = rand() % max_decre;
        token = token - decre;

        fprintf(stderr, "Proceso %d resta %d, token ahora: %d\n", id_logico, decre, token);

        // Si el token es menor que 0, el proceso termina
        if (token < 0) {
            fprintf(stderr, "Proceso %d termina\n", id_logico);
            close(STDOUT_FILENO);  // Cerramos el pipe de salida para indicar que ya no enviaremos más datos
            exit(id_logico);  // Terminamos el proceso con su id lógico
        }

        // Escribimos el token actualizado en el pipe de salida (para el siguiente proceso)
        if (write(STDOUT_FILENO, &token, sizeof(int)) <= 0) {
            perror("write");
            exit(255);  // Si hay un error al escribir, terminamos el proceso
        }
    }
}

void crear_anillo_y_jugar(int cantidad, int token, int max_decre) {
    int vivos[cantidad];
    int procesos_vivos = cantidad;

    // Inicializamos todos los procesos como vivos
    for (int i = 0; i < cantidad; i++) vivos[i] = 1;

    int sync_pipes[cantidad][2];  // Pipes para sincronización entre procesos

    // Mientras haya más de un proceso vivo, seguimos con el ciclo
    while (procesos_vivos > 1) {
        int vivos_indices[procesos_vivos];  // Arreglo con los índices de los procesos vivos
        int idx = 0;
        // Guardamos los índices de los procesos vivos en vivos_indices
        for (int i = 0; i < cantidad; i++) {
            if (vivos[i]) vivos_indices[idx++] = i;
        }

        // Imprimimos el estado actual de los procesos vivos
        //fprintf(stderr, "Ronda con %d procesos vivos: ", procesos_vivos);
        //for (int k = 0; k < procesos_vivos; k++) fprintf(stderr, "%d ", vivos_indices[k]);
        //fprintf(stderr, "\n");

        // Creamos los pipes para el anillo de procesos
        int anillo[procesos_vivos][2];
        for (int i = 0; i < procesos_vivos; i++) {
            if (pipe(anillo[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            if (pipe(sync_pipes[i]) == -1) {
                perror("sync pipe");
                exit(EXIT_FAILURE);
            }
        }

        // Creamos los procesos hijos
        for (int i = 0; i < procesos_vivos; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {  // Proceso hijo
                close(sync_pipes[i][0]);  // Cerramos la entrada del pipe de sincronización

                int idx_antes = (i + procesos_vivos - 1) % procesos_vivos;  // Índice del proceso anterior en el anillo

                // Redirigimos el pipe de entrada a la entrada del hijo
                dup2(anillo[idx_antes][0], STDIN_FILENO);
                // Redirigimos el pipe de salida a la salida del hijo
                dup2(anillo[i][1], STDOUT_FILENO);

                // Cerramos todos los pipes que no usamos
                for (int j = 0; j < procesos_vivos; j++) {
                    close(anillo[j][0]);
                    close(anillo[j][1]);
                }

                // Avisamos al padre que el hijo está listo
                if (write(sync_pipes[i][1], "X", 1) != 1) {
                    perror("write sync");
                    exit(EXIT_FAILURE);
                }
                close(sync_pipes[i][1]);

                // Llamamos a la función que maneja el paso del token y la resta
                ciclo_token(max_decre, vivos_indices[i]);
                exit(vivos_indices[i]);  // Terminamos el proceso con su id
            }
            close(sync_pipes[i][1]);  // El padre cierra la parte de escritura del pipe de sincronización
        }

        // El padre espera que todos los hijos terminen de inicializarse
        for (int i = 0; i < procesos_vivos; i++) {
            char buf;
            if (read(sync_pipes[i][0], &buf, 1) != 1) {
                fprintf(stderr, "Error sincronizando hijo %d\n", vivos_indices[i]);
                exit(EXIT_FAILURE);
            }
            close(sync_pipes[i][0]);
        }

        // Escribir el token inicial en el pipe correcto del primer proceso
        int ultimo = procesos_vivos - 1;  // El último proceso en el anillo
        fprintf(stderr, "Padre envía token %d al proceso %d\n", token, vivos_indices[0]);
        if (write(anillo[ultimo][1], &token, sizeof(int)) <= 0) {
            perror("write token");
        }

        // Cerramos todos los pipes en el padre después de enviar el token inicial
        for (int i = 0; i < procesos_vivos; i++) {
            close(anillo[i][0]);
            close(anillo[i][1]);
        }

        // Esperamos a que los hijos terminen y actualizamos el estado de los procesos vivos
        int hijos_esperar = procesos_vivos;
        while (hijos_esperar > 0) {
            int status;
            pid_t pid = wait(&status);  // Esperamos por cada hijo
            if (pid == -1) {
                perror("wait");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code >= 0 && exit_code < cantidad) {
                    if (vivos[exit_code]) {
                        vivos[exit_code] = 0;  // Marcamos el proceso como eliminado
                        procesos_vivos--;  // Reducimos el número de procesos vivos
                        //fprintf(stderr, "Padre detectó que proceso %d fue eliminado. Quedan %d procesos.\n", exit_code, procesos_vivos);
                    }
                }
            }
            hijos_esperar--;
        }
    }

    // Después de que solo quede un proceso, el padre lo declara como ganador
    int ganador = -1;
    for (int i = 0; i < cantidad; i++) {
        if (vivos[i]) {
            ganador = i;
            break;
        }
    }

    fprintf(stderr, "Proceso %d es el ganador!\n", ganador);
}
