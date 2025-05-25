#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

int** crear_hijos(int cantidad);

void conectar_hijos(int **pipes_padrehijo, int hijos, pid_t pid);