#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void crear_anillo_y_jugar(int cantidad, int token, int max_decre);
void ciclo_token(int max_decre, int id_logico);