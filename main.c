#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include "map.h"
#include "car.h"

#define MAX_CARS 100

Car cars[MAX_CARS];
int car_count = 0;

CRITICAL_SECTION csCars; //protege o vetor cars[]
int game_running = 1;

//estado lógico dos semáforos de trânsito
typedef enum { LIGHT_RED, LIGHT_GREEN } LightState;

LightState light_ns; //semáforo para carros N/S
LightState light_ew; //semáforo para carros L/O

CRITICAL_SECTION csLights; //protege acesso aos semáforos

HANDLE cruzamento_sem; //semáforo para a área crítica do centro

//thread que atualiza o movimento dos carros
DWORD WINAPI cars_thread(LPVOID arg) {
    (void)arg;

    int mid_row = MAP_ROWS/2;
    int mid_col = MAP_COLS/2;

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
            int can_go = 0;  //se o semáforo da direção está verde

            //verifica em qual semáforo esse carro depende
            EnterCriticalSection(&csLights);
            if (c.dir == DIR_NORTH || c.dir == DIR_SOUTH) {
                can_go = (light_ns == LIGHT_GREEN);
            } else {
                can_go = (light_ew == LIGHT_GREEN);
            }
            LeaveCriticalSection(&csLights);

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

            //regra de movimento:
            //se está prestes a entrar no centro:
            if (approaching_center) {
                if (can_go) {
                    //semáforo da direção está verde -> tenta entrar na região crítica
                    WaitForSingleObject(cruzamento_sem, INFINITE);

                    EnterCriticalSection(&csCars);
                    car_update(&cars[i]); //entra no centro
                    LeaveCriticalSection(&csCars);
                } else {
                    //semáforo vermelho -> NÃO anda (fica parado)
                    //(aqui da pra contar tempo de espera, etc.)
                }
            }
            else if (in_center) {
                //já está no centro, o próximo passo é sair
                EnterCriticalSection(&csCars);
                car_update(&cars[i]); // sai do centro
                LeaveCriticalSection(&csCars);

                //libera a região crítica
                ReleaseSemaphore(cruzamento_sem, 1, NULL);
            }
            else {
                //longe do centro -> movimento normal
                EnterCriticalSection(&csCars);
                car_update(&cars[i]);
                LeaveCriticalSection(&csCars);
            }
        }

        Sleep(200); //velocidade geral dos carros
    }

    return 0;
}

//thread que lê entrada do usuário para controlar os semáforos
DWORD WINAPI input_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        if (_kbhit()) {          //tem tecla pressionada?
            int ch = _getch();   //lê uma tecla (sem eco)

            if (ch == 'q' || ch == 'Q') {
                game_running = 0; //sinaliza pra todo mundo encerrar
                break;
            } else if (ch == '1') {
                //NS verde, LO vermelho
                EnterCriticalSection(&csLights);
                light_ns = LIGHT_GREEN;
                light_ew = LIGHT_RED;
                LeaveCriticalSection(&csLights);
            } else if (ch == '2') {
                //LO verde, NS vermelho
                EnterCriticalSection(&csLights);
                light_ns = LIGHT_RED;
                light_ew = LIGHT_GREEN;
                LeaveCriticalSection(&csLights);
            }
        }

        Sleep(50);  //evita usar 100% de CPU
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

    //imprime informações de status (semáforos)
    EnterCriticalSection(&csLights);
    printf("N-S: %s | L-O: %s\n",
           light_ns == LIGHT_GREEN ? "VERDE" : "VERMELHO",
           light_ew == LIGHT_GREEN ? "VERDE" : "VERMELHO");
    LeaveCriticalSection(&csLights);

    printf("Controles: [1] N-S verde  [2] L-O verde  [q] sair\n\n");

    //imprime o mapa
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

    //tenta achar slot de carro inativo pra reutilizar
    for (int i = 0; i < car_count; i++) {
        if (!cars[i].active) {
            car_init(&cars[i], dir);
            LeaveCriticalSection(&csCars);
            return i;
        }
    }

    //se não tiver nenhum inativo, tenta usar uma nova posição
    if (car_count < MAX_CARS) {
        int id = car_count;
        car_init(&cars[car_count], dir);
        car_count++;
        LeaveCriticalSection(&csCars);
        return id;
    }

    //sem espaço
    LeaveCriticalSection(&csCars);
    return -1;
}

//thread que spawna carros N-S a cada 2 segundos
DWORD WINAPI spawner_ns_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        //tbm da pra alternar entre criar no norte e no sul
        spawn_car(DIR_NORTH);
        Sleep(2000);//a cada 2 segundos

        //se quisermos também do sul:
        //spawn_car(DIR_SOUTH);
        //Sleep(2000);
    }

    return 0;
}

//thread que spawna carros L-O a cada 2.5 segundos
DWORD WINAPI spawner_ew_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        spawn_car(DIR_WEST);
        Sleep(2500);// a cada 2.5 segundos

        //direção contraria:
        //spawn_car(DIR_EAST);
        //Sleep(2500);
    }

    return 0;
}

int main(void) {
    map_init();

    //inicializa o vetor de carros
    car_count = 0;

    //inicializa a critical section
    InitializeCriticalSection(&csCars);
    InitializeCriticalSection(&csLights);

    //estado inicial dos semáforos: N-S verde, L-O vermelho
    EnterCriticalSection(&csLights);
    light_ns = LIGHT_GREEN;
    light_ew = LIGHT_RED;
    LeaveCriticalSection(&csLights);

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
        DeleteCriticalSection(&csLights);
        return 1;
    }

    //cria alguns carros de teste
    // spawn_car(DIR_WEST);  //da esquerda para a direita
    // spawn_car(DIR_EAST);  //da direita para a esquerda
    // spawn_car(DIR_NORTH); //de cima para baixo
    // spawn_car(DIR_SOUTH); //de baixo para cima

    //cria a thread que vai atualizar todos os carros
    HANDLE hCarsThread = CreateThread(NULL, 0, cars_thread, NULL, 0, NULL);
    //thread de input (controle dos semáforos e sair)
    HANDLE hInputThread = CreateThread(NULL, 0, input_thread, NULL, 0, NULL);

    HANDLE hSpawnNS = CreateThread(NULL, 0, spawner_ns_thread, NULL, 0, NULL);
    HANDLE hSpawnEW = CreateThread(NULL, 0, spawner_ew_thread, NULL, 0, NULL);
    if (hCarsThread == NULL || hInputThread == NULL || hSpawnNS == NULL || hSpawnEW == NULL) {
        printf("Erro ao criar alguma thread.\n");
        game_running = 0;
        //VERIFICAR DAQUI PRA BAIXO wait for single object e close handle
        WaitForSingleObject(hCarsThread, INFINITE);
        WaitForSingleObject(hInputThread, INFINITE);
        CloseHandle(hCarsThread);
        CloseHandle(hInputThread);
        CloseHandle(cruzamento_sem);
        DeleteCriticalSection(&csCars);
        DeleteCriticalSection(&csLights);
        return 1;
}

    //loop principal, só desenha enquanto o jogo ainda é executado
    while (game_running) {
        draw_all();
        Sleep(100); //taxa de atualização da tela
    }

    //acabou o jogo: limpa tela e mostra mensagem
    system("cls");
    printf("=== JOGO ENCERRADO ===\n");
    printf("Obrigado por jogar o cruzamento :)\n");
    printf("Pressione ENTER para sair...\n");

    //garante que as threads vao sair
    WaitForSingleObject(hCarsThread, INFINITE);
    WaitForSingleObject(hInputThread, INFINITE);
    WaitForSingleObject(hSpawnNS, INFINITE);
    WaitForSingleObject(hSpawnEW, INFINITE);

    CloseHandle(hCarsThread);
    CloseHandle(hInputThread);
    CloseHandle(hSpawnNS);
    CloseHandle(hSpawnEW);
    CloseHandle(cruzamento_sem); //fecha o handle do semáforo

    //libera a critical section
    DeleteCriticalSection(&csCars);
    DeleteCriticalSection(&csLights);

    getchar();
    return 0;
}
