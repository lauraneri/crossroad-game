#include "car.h"

void car_init(Car *c, Direction dir) {
    int mid_row = MAP_ROWS / 2;
    int mid_col = MAP_COLS / 2;

    c->dir = dir;
    c->active = 1;

    switch (dir) {
        case DIR_WEST: //da esquerda para a direita
            c->row = mid_row;
            c->col = 0;
            break;

        case DIR_EAST: //da direita para a esquerda
            c->row = mid_row;
            c->col = MAP_COLS-1;
            break;

        case DIR_NORTH: //de cima para baixo
            c->row = 0;
            c->col = mid_col;
            break;

        case DIR_SOUTH: //de baixo para cima
            c->row = MAP_ROWS-1;
            c->col = mid_col;
            break;
    }
}

void car_update(Car *c) {
    if (!c->active) return;

    switch (c->dir) {
        case DIR_WEST: //esquerda pra direita
            c->col++;
            if (c->col >= MAP_COLS) {
                c->active = 0;
            }
            break;

        case DIR_EAST: //direita pra esquerda
            c->col--;
            if (c->col < 0) {
                c->active = 0;
            }
            break;

        case DIR_NORTH: //cima pra baixo
            c->row++;
            if (c->row >= MAP_ROWS) {
                c->active = 0;
            }
            break;

        case DIR_SOUTH: // baixo pra cima
            c->row--;
            if (c->row < 0) {
                c->active = 0;
            }
            break;
    }
}

void car_draw(const Car *c, char buffer[MAP_ROWS][MAP_COLS]) {
    if (!c->active) return;

    //verifica se ta dentro do mapa antes de desenhar
    if (c->row >= 0 && c->row < MAP_ROWS &&
        c->col >= 0 && c->col < MAP_COLS) {
        buffer[c->row][c->col] = 'c';  //icone do carro colocado na posicao
    }
}
