FLAGS = -Wall -lm

all: desafio2 hijos

desafio2: desafio2.c funciones.c
	gcc $(FLAGS) -o desafio2 desafio2.c funciones.c

hijos: hijos.c funciones.c
	gcc $(FLAGS) -o hijos hijos.c funciones.c

clean:
	rm -f desafio2 hijos
