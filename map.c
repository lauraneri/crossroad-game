#include "map.h"

char map_buffer[MAP_ROWS][MAP_COLS];

void map_init(void) {
    //preenche todo o mapa com espaço
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            map_buffer[i][j] = ' ';
        }
    }

    int mid_row = MAP_ROWS/2;
    int mid_col = MAP_COLS/2;

    //rua horizontal
    for (int j = 0; j < MAP_COLS; j++) {
        map_buffer[mid_row][j] = '-';
    }

    //rua vertical
    for (int i = 0; i < MAP_ROWS; i++) {
        map_buffer[i][mid_col] = '|';
    }

    //centro do cruzamento
    map_buffer[mid_row][mid_col] = '+';
}
