#include <stdio.h>
#include <windows.h>
#include "map.h"
#include "car.h"

#define MAX_CARS 100

Car cars[MAX_CARS];
int car_count = 0;

CRITICAL_SECTION csCars; //protege o vetor cars[]
int game_running = 1;

HANDLE cruzamento_sem;   //semáforo para a área crítica do centro

//thread que atualiza o movimento dos carros
DWORD WINAPI cars_thread(LPVOID arg) {
    (void)arg;

    int mid_row = MAP_ROWS / 2;
    int mid_col = MAP_COLS / 2;

    while (game_running) {
        for (int i = 0; i < car_count; i++) {

            //copia estado atual do carro para variáveis locais
            EnterCriticalSection(&csCars);
            Car c = cars[i];   //cópia
            LeaveCriticalSection(&csCars);

            if (!c.active) {
                continue;
            }

            int approaching_center = 0;
            int in_center = 0;

            //detectar se está perto do centro/no centro
            switch (c.dir) {
                case DIR_WEST:  //andando esquerda pra direita
                    if (c.row == mid_row && c.col == mid_col - 1) {
                        approaching_center = 1;   //está logo antes do centro
                    } else if (c.row == mid_row && c.col == mid_col) {
                        in_center = 1;
                    }
                    break;

                case DIR_EAST:  //direita pra esquerda
                    if (c.row == mid_row && c.col == mid_col + 1) {
                        approaching_center = 1;
                    } else if (c.row == mid_row && c.col == mid_col) {
                        in_center = 1;
                    }
                    break;

                case DIR_NORTH: //cima pra baixo
                    if (c.col == mid_col && c.row == mid_row - 1) {
                        approaching_center = 1;
                    } else if (c.col == mid_col && c.row == mid_row) {
                        in_center = 1;
                    }
                    break;

                case DIR_SOUTH: //baixo pra cima
                    if (c.col == mid_col && c.row == mid_row + 1) {
                        approaching_center = 1;
                    } else if (c.col == mid_col && c.row == mid_row) {
                        in_center = 1;
                    }
                    break;
            }

            //se está prestes a entrar no centro:
            if (approaching_center) {
                //espera semáforo do cruzamento (região crítica)
                WaitForSingleObject(cruzamento_sem, INFINITE);

                //agora pode fazer o passo (vai entrar no centro)
                EnterCriticalSection(&csCars);
                car_update(&cars[i]);
                LeaveCriticalSection(&csCars);
            }
            //se está no centro, o próximo passo é sair:
            else if (in_center) {
                //faz o passo (sair do centro)
                EnterCriticalSection(&csCars);
                car_update(&cars[i]);
                LeaveCriticalSection(&csCars);

                //libera o semáforo para outro carro poder entrar
                ReleaseSemaphore(cruzamento_sem, 1, NULL);
            }
            //caso normal (longe do centro):
            else {
                EnterCriticalSection(&csCars);
                car_update(&cars[i]);
                LeaveCriticalSection(&csCars);
            }
        }

        Sleep(200); //controla a velocidade dos carros
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

    //inicializa o semáforo do cruzamento:
    cruzamento_sem = CreateSemaphore(
        NULL,  //atributos de segurança padrão
        1,     //valor inicial = 1 (semaforo binário)
        1,     //valor máximo = 1
        NULL   //sem nome
    );

    if (cruzamento_sem == NULL) {
        printf("Erro ao criar semáforo do cruzamento.\n");
        DeleteCriticalSection(&csCars);
        return 1;
    }

    //cria alguns carros de teste
    spawn_car(DIR_WEST);  //da esquerda para a direita
    spawn_car(DIR_EAST);  //da direita para a esquerda
    spawn_car(DIR_NORTH); //de cima para baixo
    spawn_car(DIR_SOUTH); //de baixo para cima

    //cria a thread que vai atualizar todos os carros
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
    CloseHandle(cruzamento_sem);  //fecha o handle do semáforo

    //libera a critical section
    DeleteCriticalSection(&csCars);

    printf("\nFim. Pressione ENTER para sair...\n");
    getchar();
    return 0;
}
