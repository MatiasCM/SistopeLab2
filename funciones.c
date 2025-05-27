#include "funciones.h"

void crear_hijos(int cantidad, int max_decre, int anillo[][2]) {
    pid_t pid;

    for (int i = 0; i < cantidad; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Hijo

            // Redirigir STDIN y STDOUT al anillo
            dup2(anillo[(i + cantidad - 1) % cantidad][0], STDIN_FILENO); // entrada
            dup2(anillo[i][1], STDOUT_FILENO); // salida

            // Cerrar todos los pipes
            for (int j = 0; j < cantidad; j++) {
                close(anillo[j][0]);
                close(anillo[j][1]);
            }

            // Ejecutar la lógica del token
            ciclo_token(max_decre, i);
            exit(0);
        }
    }
}

void crear_anillo_y_jugar(int cantidad, int token, int max_decre) {
    int anillo[cantidad][2];

    // Crear pipes para el anillo
    for (int i = 0; i < cantidad; i++) {
        if (pipe(anillo[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Crear hijos
    crear_hijos(cantidad, max_decre, anillo);

    // Esperar brevemente para asegurar que los hijos hagan dup2()
    //usleep(100000); // 0.1 segundos

    // Enviar token inicial
    fprintf(stderr, "Padre envía token: %d\n", token);
    if (write(anillo[0][1], &token, sizeof(int)) <= 0) {
        perror("write token");
    }

    // Cerrar todos los pipes en el padre
    for (int i = 0; i < cantidad; i++) {
        close(anillo[i][0]);
        close(anillo[i][1]);
    }

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < cantidad; i++) {
        wait(NULL);
    }
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
