#include <stdio.h>
#include <windows.h>
#include "map.h"
#include "car.h"

Car car;
CRITICAL_SECTION csCar;
int game_running = 1;

//criaçao da thread do carro
DWORD WINAPI car_thread(LPVOID arg) {
    (void)arg;  //só pra evitar warning de parametro não usado

    while (game_running) {
        EnterCriticalSection(&csCar);
        //atualiza o carro
        car_update(&car);
        int ativo = car.active;
        LeaveCriticalSection(&csCar);

        if (!ativo) {
            //se o carro saiu da tela, podemos encerrar a thread
            break;
        }

        Sleep(200); //controla a velocidade do carro
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

    //desenha o carro em cima do tmp
    EnterCriticalSection(&csCar);
    car_draw(&car, tmp);
    LeaveCriticalSection(&csCar);

    //limpa tela
    system("cls");

    //imprime
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            putchar(tmp[i][j]);
        }
        putchar('\n');
    }
}

int main(void) {
    //inicializa mapa
    map_init();

    //inicializa carro global
    car_init(&car);

    //inicializa a seçao critica
    InitializeCriticalSection(&csCar);

    //thread que vai atualizar o carro
    HANDLE hCarThread = CreateThread(
        NULL,              //segurança padrão
        0,                 //stack default
        car_thread,        //função da thread
        NULL,              //argumento para a thread
        0,                 //flags
        NULL               //ID da thread (não precisamos)
    );

    if (hCarThread == NULL) {
        printf("Erro ao criar thread do carro.\n");
        DeleteCriticalSection(&csCar);
        return 1;
    }

    //loop principal: só desenha
    for (int t = 0; t < 100; t++) {
        draw_all(); //usa o carro protegido pela critical section
        Sleep(100); //taxa de atualização da tela

        //se o carro já não estiver ativo, podemos parar mais cedo
        EnterCriticalSection(&csCar);
        int ativo = car.active;
        LeaveCriticalSection(&csCar);
        if (!ativo) {
            break;
        }
    }

    //sinaliza que o jogo terminou (caso a thread ainda esteja rodando)
    game_running = 0;

    //espera a thread terminar
    WaitForSingleObject(hCarThread, INFINITE);
    CloseHandle(hCarThread);

    //limpa recursos
    DeleteCriticalSection(&csCar);

    printf("\nFim. Pressione ENTER para sair...\n");
    getchar();
    return 0;
}
