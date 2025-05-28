#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int** crear_hijos(int cantidad, int **anillo);

void ciclo_token(int max_decre, int id_logico);

void hijos_esperan_conexiones();

void conectar_hijos(int **pipes_padrehijo, int **anillo, int hijos, pid_t pid, int token);