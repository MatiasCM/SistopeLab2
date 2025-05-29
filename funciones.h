#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

// Entradas:    Número de procesos hijos a crear (cantidad), arreglo de pipes para comunicación en anillo (**anillo)
//
// Salidas:     Retorna un puntero a un arreglo de pipes
//
// Descripción: Crea los procesos hijos con fork(), configura pipes para comunicación entre padre e hijos y
//              entre hijos en forma de anillo, y prepara los procesos para comenzar la ejecución
int** crear_hijos(int cantidad, int **anillo);

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
void ciclo_token(int max_decre, int id_logico);

// Entradas:    Número total de procesos hijos (cantidad), identificador lógico del proceso actual (id_proceso)
//
// Salidas:     No retorna nada
//
// Descripción: Recibe y configura las pipes para comunicación entre procesos y llama a hijos_esperan_conexiones para continuar
void inicializacion_hijos(int cantidad, int id_proceso);

// Entradas:    No recibe nada
//
// Salidas:     No retorna nada
//
// Descripción: Hace que el proceso hijo espere las conexiones necesarias de sus hermanos,
//              configura sus pipes para comunicación con los vecinos en el anillo y gestiona la comunicación
//              para recibir el token y el decrecimiento, y luego inicia el ciclo de token
void hijos_esperan_conexiones();

// Entradas:    Arreglo de pipes padre-hijo para cada proceso (**pipes_padrehijo), arreglo de pipes que forman el anillo entre hijos (**anillo),
//              cantidad de procesos hijos (hijos), valor inicial del token (token), máximo decrecimiento para el token (decrecimiento)
//
// Salidas:     No retorna nada
//
// Descripción: Conecta todos los hijos entre sí configurando el anillo de comunicación, inicializa el
//              envío del token y maneja la eliminación de procesos cuando el token decrece a menos de 0,
//              ajustando las conexiones en el anillo conforme se eliminan procesos.
void conectar_hijos(int **pipes_padrehijo, int **anillo, int hijos, pid_t pid, int token);