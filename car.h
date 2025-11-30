#ifndef CAR_H
#define CAR_H

#include "map.h"

typedef enum {
    DIR_NORTH,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST
} Direction;

typedef struct {
    int row;
    int col;
    Direction dir;
    int active; //1 = andando, 0 = ja saiu da tela
} Car;

//inicializa um carro vindo de uma direção específica
void car_init(Car *c, Direction dir);

//atualiza a posicao de um carro
void car_update(Car *c);

//desenha o carro em cima de um buffer (tmp) passado
void car_draw(const Car *c, char buffer[MAP_ROWS][MAP_COLS]);

#endif
