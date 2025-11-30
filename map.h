#ifndef MAP_H
#define MAP_H

//definição do tamanho do mapa
#define MAP_ROWS 15
#define MAP_COLS 31

//buffer do mapa
extern char map_buffer[MAP_ROWS][MAP_COLS];

//inicializa o mapa com um cruzamento simples
void map_init(void);

#endif
