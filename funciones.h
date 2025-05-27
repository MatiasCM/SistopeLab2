#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void crear_anillo_y_jugar(int cantidad, int token, int max_decre);
void crear_hijos(int cantidad, int max_decre, int anillo[][2]);
void ciclo_token(int max_decre, int id_logico);
