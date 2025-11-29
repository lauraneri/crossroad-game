#include <stdio.h>
#include <windows.h>

//definição do tamanho do mapa
#define MAP_ROWS 15
#define MAP_COLS 31

char map_buffer[MAP_ROWS][MAP_COLS];

//função para inicializar o mapa
void init_map() {
    for (int i=0; i<MAP_ROWS; i++) {
        for (int j=0; j<MAP_COLS; j++) {
            map_buffer[i][j] = ' ';
        }
    }

    int mid_row = MAP_ROWS/2;
    int mid_col = MAP_COLS/2;

    //rua horizontal
    for (int j=0; j<MAP_COLS; j++) {
        map_buffer[mid_row][j] = '-';
    }
    //rua vertical
    for (int i=0; i<MAP_ROWS; i++) {
        map_buffer[i][mid_col] = '|';
    }

    map_buffer[mid_row][mid_col] = '+'; //centro do cruzamento
}

//função para desenhar o mapa
void draw_map() {
    for (int i=0; i<MAP_ROWS; i++) {
        for (int j=0; j<MAP_COLS; j++) {
            putchar(map_buffer[i][j]);
        }
        putchar('\n');
    }
}

int main(void) {
    init_map();
    draw_map();
    system("pause");  //comando para pausar a tela no windows, esperando uma tecla pra continuar
    return 0;
}
