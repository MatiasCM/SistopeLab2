#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "funciones.h"

int main(int argc, char *argv[]) {
    int hermanos = atoi(argv[1]);
    int id = atoi(argv[2]);
    inicializacion_hijos(hermanos, id);
    return 0;

}