#include "funciones.h"

void ciclo_token(int max_decre, int id_logico) {
    int token;

    srand(time(NULL));

    while (1) {
        // Leer token enviado por el padre (desde stdin)
        ssize_t datos = read(STDIN_FILENO, &token, sizeof(int));
        if (datos == 0) {
            // Si el pipe se cierra, terminar el proceso hijo
            exit(0);
        }
        if (datos < 0) {
            perror("read");
            exit(1);
        }

        // Mostrar que el proceso recibió el token
        fprintf(stderr, "Proceso %d recibió token: %d\n", id_logico, token);

        // Si token negativo, proceso debe terminar (quedó eliminado)
        if (token < 0) {
            fprintf(stderr, "Proceso %d termina\n", id_logico);
            exit(id_logico);
        }

        int decre = rand() % max_decre;
        token -= decre;
        fprintf(stderr, "Proceso %d resta %d, token ahora: %d\n", id_logico, decre, token);

        // Enviar el token actualizado al padre (stdout)
        if (write(STDOUT_FILENO, &token, sizeof(int)) <= 0) {
            perror("write");
            exit(1);
        }
    }
}

void crear_anillo_y_jugar(int cantidad, int token, int max_decre) {
    int vivos[cantidad];
    pid_t pids[cantidad];
    int pipes_in[cantidad][2];
    int pipes_out[cantidad][2];

    // Inicializar todos los procesos como vivos
    for (int i = 0; i < cantidad; i++) vivos[i] = 1;

    // Crear todos los pipes necesarios para la comunicación padre-hijos
    for (int i = 0; i < cantidad; i++) {
        if (pipe(pipes_in[i]) == -1) {
            perror("pipe in");
            exit(EXIT_FAILURE);
        }
        if (pipe(pipes_out[i]) == -1) {
            perror("pipe out");
            exit(EXIT_FAILURE);
        }
    }

    // Crear todos los procesos hijos con fork una sola vez
    for (int i = 0; i < cantidad; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            //El hijo

            // Redirigir stdin para leer desde pipe_in
            close(pipes_in[i][1]); 
            dup2(pipes_in[i][0], STDIN_FILENO);
            close(pipes_in[i][0]);

            // Redirigir stdout para escribir en pipe_out 
            close(pipes_out[i][0]); 
            dup2(pipes_out[i][1], STDOUT_FILENO);
            close(pipes_out[i][1]);

            // Cerrar todos los demás pipes no usados para evitar fugas
            for (int j = 0; j < cantidad; j++) {
                if (j != i) {
                    close(pipes_in[j][0]);
                    close(pipes_in[j][1]);
                    close(pipes_out[j][0]);
                    close(pipes_out[j][1]);
                }
            }

            // Llamar a la función que maneja el ciclo del token para el proceso hijo
            ciclo_token(max_decre, i);

            exit(i);
        } else {
            //El padre

            pids[i] = pid;

            // El padre cierra extremos de pipe que no usará
            close(pipes_in[i][0]);
            close(pipes_out[i][1]);
        }
    }

    int procesos_vivos = cantidad;   // Contador de procesos vivos
    int current = 0;                 // Índice del proceso que tiene el token actualmente
    int token_actual = token;        // Token actual que se pasa entre procesos
    int ultimo_token_valido = token; // Último token válido no negativo

    //fprintf(stderr, "Padre inicia juego con token=%d, max_decre=%d, procesos=%d\n", token, max_decre, cantidad);

    // Enviar token inicial al primer proceso vivo
    while (!vivos[current]) current = (current + 1) % cantidad;
    if (write(pipes_in[current][1], &token_actual, sizeof(int)) <= 0) {
        perror("write token inicio");
        exit(EXIT_FAILURE);
    }

    // Bucle principal: mientras haya más de un proceso vivo
    while (procesos_vivos > 1) {
        int token_leido;
        // Leer token actualizado del proceso actual
        ssize_t leido = read(pipes_out[current][0], &token_leido, sizeof(int));
        if (leido <= 0) {
            // Error o pipe cerrado, proceso eliminado inesperadamente
            fprintf(stderr, "Padre detecta pipe cerrado o error proceso %d\n", current);
            vivos[current] = 0;
            procesos_vivos--;
        } else {
            //fprintf(stderr, "Padre recibe token %d de proceso %d\n", token_leido, current);

            if (token_leido < 0) {
                // Proceso eliminado (token negativo)
                fprintf(stderr, "Proceso %d eliminado\n", current);
                vivos[current] = 0;
                procesos_vivos--;

                // Reiniciar token para la nueva ronda al valor original
                ultimo_token_valido = token;
            } else {
                // Actualizar token actual y último válido
                token_actual = token_leido;
                ultimo_token_valido = token_leido;
            }
        }

        // Buscar siguiente proceso vivo
        do { current = (current + 1) % cantidad; } while (!vivos[current]);

        // Si sólo queda un proceso vivo, salir del bucle
        if (procesos_vivos == 1) break;

        // Enviar token válido al siguiente proceso vivo
        if (write(pipes_in[current][1], &ultimo_token_valido, sizeof(int)) <= 0) {
            perror("write padre token siguiente");
            exit(EXIT_FAILURE);
        }
    }

    // Encontrar el proceso ganador
    int ganador = -1;
    for (int i = 0; i < cantidad; i++) {
        if (vivos[i]) {
            ganador = i;
            break;
        }
    }

    fprintf(stderr, "Proceso %d es el ganador!\n", ganador);

    // Terminar todos los procesos restantes y cerrar pipes
    for (int i = 0; i < cantidad; i++) {
        close(pipes_in[i][1]);
        close(pipes_out[i][0]);
        if (vivos[i]) {
            kill(pids[i], SIGTERM);
        }
    }

    // Esperar que todos los hijos terminen para evitar zombies
    for (int i = 0; i < cantidad; i++) {
        waitpid(pids[i], NULL, 0);
    }
}
