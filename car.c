#include "car.h"

void car_init(Car *c) {
    int mid_row = MAP_ROWS/2;

    c->row = mid_row;      //linha do meio (rua horizontal)
    c->col = 0;            //começa na borda esquerda
    c->dir = DIR_WEST;     //vamos considerar "WEST" como sendo ir de esquerda -> direita
    c->active = 1;
}

void car_update(Car *c) {
    if (!c->active) return;

    //por enquanto só tratamos DIR_WEST
    if (c->dir == DIR_WEST) {
        c->col++;               //anda uma coluna para a direita
        if (c->col >= MAP_COLS) {
            c->active = 0;      //saiu da tela
        }
    }

    //mais tarde trataremos outras direções (N, S, E)
}

void car_draw(const Car *c, char buffer[MAP_ROWS][MAP_COLS]) {
    if (!c->active) return;

    //checa se está dentro dos limites antes de desenhar
    if (c->row >= 0 && c->row < MAP_ROWS &&
        c->col >= 0 && c->col < MAP_COLS) {
        buffer[c->row][c->col] = 'c';  // 'c' = carro
    }
}
