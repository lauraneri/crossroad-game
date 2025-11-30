#include <stdio.h>
#include <windows.h>
#include "map.h"
#include "car.h"

#define MAX_CARS 100

Car cars[MAX_CARS];
int car_count = 0;

CRITICAL_SECTION csCars; //protege o vetor cars[]
int game_running = 1;

//thread que atualiza o movimento dos carros
DWORD WINAPI cars_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        EnterCriticalSection(&csCars);
        for (int i = 0; i < car_count; i++) {
            car_update(&cars[i]);
        }
        LeaveCriticalSection(&csCars);

        Sleep(200); //velocidade dos carros
    }

    return 0;
}

//função que junta mapa e carro e desenha tudo
void draw_all(void) {
    char tmp[MAP_ROWS][MAP_COLS];

    //copia o mapa base para tmp
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            tmp[i][j] = map_buffer[i][j];
        }
    }

    //desenha todos os carros em cima do tmp
    EnterCriticalSection(&csCars);
    for (int i = 0; i < car_count; i++) {
        car_draw(&cars[i], tmp);
    }
    LeaveCriticalSection(&csCars);

    //limpa tela
    system("cls");

    //imprime o tmp
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            putchar(tmp[i][j]);
        }
        putchar('\n');
    }
}

//adiciona um novo carro ao vetor, entrando na seção crítica e verificando limites
int spawn_car(Direction dir) {
    EnterCriticalSection(&csCars);

    if (car_count >= MAX_CARS) {
        LeaveCriticalSection(&csCars);
        return -1;
    }

    int id = car_count;
    car_init(&cars[car_count], dir);
    car_count++;

    LeaveCriticalSection(&csCars);
    return id;
}

int main(void) {
    map_init();

    //inicializa o vetor de carros
    car_count = 0;

    //inicializa a critical section
    InitializeCriticalSection(&csCars);

    //cria alguns carros de teste
    spawn_car(DIR_WEST);  //da esquerda para a direita
    spawn_car(DIR_EAST);  //da direita para a esquerda
    spawn_car(DIR_NORTH); //de cima para baixo
    spawn_car(DIR_SOUTH); //de baixo para cima

    //cria a thread que vai atualizar TODOS os carros
    HANDLE hCarsThread = CreateThread(
        NULL,
        0,
        cars_thread,
        NULL,
        0,
        NULL
    );

    if (hCarsThread == NULL) {
        printf("Erro ao criar thread de carros.\n");
        DeleteCriticalSection(&csCars);
        return 1;
    }

    //loop principal: só desenha
    for (int t = 0; t < 300; t++) {
        draw_all();
        Sleep(100); //taxa de atualização da tela
    }

    //sinaliza que o jogo terminou
    game_running = 0;

    //espera a thread de carros terminar
    WaitForSingleObject(hCarsThread, INFINITE);
    CloseHandle(hCarsThread);

    //libera a critical section
    DeleteCriticalSection(&csCars);

    printf("\nFim. Pressione ENTER para sair...\n");
    getchar();
    return 0;
}
