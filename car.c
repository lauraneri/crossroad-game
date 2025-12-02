#include "car.h"

void car_init(Car *c, Direction dir) {
    int mid_row = MAP_ROWS/2;
    int mid_col = MAP_COLS/2;

    c->dir = dir;
    c->active = 1;
    c->holding_sem = 0;

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

void car_next_position(const Car *c, int *next_row, int *next_col) {
    *next_row = c->row;
    *next_col = c->col;

    switch (c->dir) {
        case DIR_WEST:
            (*next_col)++;
            break;
        case DIR_EAST:
            (*next_col)--;
            break;
        case DIR_NORTH:
            (*next_row)++;
            break;
        case DIR_SOUTH:
            (*next_row)--;
            break;
    }
}