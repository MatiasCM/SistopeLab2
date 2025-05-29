#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "funciones.h"

int main(int argc, char *argv[]) {
    int opt;
    int token = -1, numero = -1, hijos = -1;
    
    srand(time(NULL));

    while ((opt = getopt(argc, argv, "t:M:p:")) != -1) {
        switch (opt) {
            case 't':
                for (int i = 0; optarg[i] != '\0'; i++) {
                    if (optarg[i] < '0' || optarg[i] > '9') {
                        fprintf(stderr, "Error: el argumento -t debe ser un número positivo.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                token = atoi(optarg);
                break;
            case 'M':
                for (int i = 0; optarg[i] != '\0'; i++) {
                    if (optarg[i] < '0' || optarg[i] > '9') {
                        fprintf(stderr, "Error: el argumento -M debe ser un número positivo.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                numero = atoi(optarg);
                break;
            case 'p':
                for (int i = 0; optarg[i] != '\0'; i++) {
                    if (optarg[i] < '0' || optarg[i] > '9') {
                        fprintf(stderr, "Error: el argumento -p debe ser un número positivo.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                hijos = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Formato correcto: %s -t <num> -M <num> -p <num>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Validar que se entreguen todos los argumentos
    // Si alguno de los argumentos es -1, significa que no se ingresó
    // el argumento correspondiente

    if (token == -1 || numero == -1 || hijos == -1) {
        fprintf(stderr, "Faltan argumentos. Formato correcto: %s -t <num> -M <num> -p <num>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Validar tipo de argumentos
    if (token )
    if (token < 0) {
        fprintf(stderr, "El token debe ser un número positivo.\n");
        exit(EXIT_FAILURE);
    }
    if (numero <= 1) {
        fprintf(stderr, "El número maximo de decrecimiento debe ser mayor a 1.\n");
        exit(EXIT_FAILURE);
    }
    if (hijos <= 0) {
        fprintf(stderr, "El número de hijos debe ser mayor a 0.\n");
        exit(EXIT_FAILURE);
    }

    int **anillo = (int**)malloc(hijos * sizeof(int*));
    int **pipes_padrehijo = crear_hijos(hijos, anillo);

    conectar_hijos(pipes_padrehijo, anillo, hijos, token, numero);
    return 0;

}