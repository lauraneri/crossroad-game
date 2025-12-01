#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include "map.h"
#include "car.h"

#define COLOR_DEFAULT 7   //cinza
#define COLOR_RED     12  //vermelho claro
#define COLOR_GREEN   10  //verde claro
#define COLOR_YELLOW  14  //amarelo

#define MAX_CARS 100

Car cars[MAX_CARS];
int car_count = 0;

//car_at[row][col] = id do carro naquela célula, ou -1 se vazia
int car_at[MAP_ROWS][MAP_COLS];

CRITICAL_SECTION csCars; //protege o vetor cars[]
CRITICAL_SECTION csLights; //protege acesso aos semáforos

int game_running = 1; //indica se o jogo está rodando
int game_over_congestion = 0; //indica se terminou por congestionamento

//estado lógico dos semáforos de trânsito
typedef enum { LIGHT_RED, LIGHT_GREEN } LightState;

LightState light_ns; //semáforo para carros N/S
LightState light_ew; //semáforo para carros L/O

HANDLE cruzamento_sem; //semáforo para a área crítica do centro
HANDLE hConsole;

void set_color(WORD color) {
    SetConsoleTextAttribute(hConsole, color);
}

//verifica se há congestionamento (todas as celulas da borda ate o cruzamento estão ocupadas)
int check_congestion(void) {
    int mid_row = MAP_ROWS / 2;
    int mid_col = MAP_COLS / 2;
    int full;
    
    EnterCriticalSection(&csCars);

    //CIMA
    full = 1;
    for (int r = 0; r < mid_row; r++) {
        if (car_at[r][mid_col] == -1) {
            full = 0;
            break;
        }
    }
    if (full) {
        LeaveCriticalSection(&csCars);
        return 1;
    }

    //BAIXO
    full = 1;
    for (int r = MAP_ROWS - 1; r > mid_row; r--) {
        if (car_at[r][mid_col] == -1) {
            full = 0;
            break;
        }
    }
    if (full) {
        LeaveCriticalSection(&csCars);
        return 1;
    }

    //ESQUERDA
    full = 1;
    for (int c = 0; c < mid_col; c++) {
        if (car_at[mid_row][c] == -1) {
            full = 0;
            break;
        }
    }
    if (full) {
        LeaveCriticalSection(&csCars);
        return 1;
    }

    //DIREITA
    full = 1;
    for (int c = MAP_COLS - 1; c > mid_col; c--) {
        if (car_at[mid_row][c] == -1) {
            full = 0;
            break;
        }
    }
    if (full) {
        LeaveCriticalSection(&csCars);
        return 1;
    }

    LeaveCriticalSection(&csCars);
    return 0;   // não está congestionado ainda
}

//thread que atualiza o movimento dos carros
DWORD WINAPI cars_thread(LPVOID arg) {
    (void)arg;

    int mid_row = MAP_ROWS / 2;
    int mid_col = MAP_COLS / 2;

    while (game_running) {
        for (int i = 0; i < car_count; i++) {

            //copia estado do carro
            EnterCriticalSection(&csCars);
            Car c = cars[i];
            LeaveCriticalSection(&csCars);

            if (!c.active) continue;

            //decide semaforo lógico
            int can_go = 0;
            EnterCriticalSection(&csLights);
            if (c.dir == DIR_NORTH || c.dir == DIR_SOUTH) {
                can_go = (light_ns == LIGHT_GREEN);
            } else {
                can_go = (light_ew == LIGHT_GREEN);
            }
            LeaveCriticalSection(&csLights);

            int next_row, next_col;
            car_next_position(&c, &next_row, &next_col);

            int at_center = (c.row == mid_row && c.col == mid_col);
            int next_is_center = (next_row == mid_row && next_col == mid_col);

            //função helper inline: move se célula alvo livre
            //(não declarada fora pra simplificar a explicação)
            if (!at_center && next_is_center) {
                //tentando ENTRAR no centro
                if (!can_go) {
                    //sinal vermelho, entao não anda
                    continue;
                }

                //pega região crítica
                WaitForSingleObject(cruzamento_sem, INFINITE);

                EnterCriticalSection(&csCars);

                //pega dnv estado real do carro pq pode ter mudado
                if (!cars[i].active) {
                    LeaveCriticalSection(&csCars);
                    ReleaseSemaphore(cruzamento_sem, 1, NULL);
                    continue;
                }

                int r = cars[i].row;
                int ccol = cars[i].col;
                car_next_position(&cars[i], &next_row, &next_col);

                //verifica se ainda esta indo pro centro
                if (!(next_row == mid_row && next_col == mid_col)) {
                    //algo mudou, não entra no centro
                    LeaveCriticalSection(&csCars);
                    ReleaseSemaphore(cruzamento_sem, 1, NULL);
                    continue;
                }

                //checa se a celula do centro está livre (teoricamente sempre)
                if (car_at[next_row][next_col] == -1) {
                    //desocupa posição antiga
                    if (r >= 0 && r < MAP_ROWS && ccol >= 0 && ccol < MAP_COLS) {
                        if (car_at[r][ccol] == i) car_at[r][ccol] = -1;
                    }

                    //move para o centro
                    cars[i].row = next_row;
                    cars[i].col = next_col;
                    cars[i].holding_sem = 1;
                    car_at[next_row][next_col] = i;
                } else {
                    //centro ocupado (nao deve acontecer, mas por segurança)
                    cars[i].holding_sem = 0;
                    ReleaseSemaphore(cruzamento_sem, 1, NULL);
                }

                LeaveCriticalSection(&csCars);
            }
            else if (at_center) {
                //regras pra quem ja ta no centro
                int soltou = 0;

                EnterCriticalSection(&csCars);

                if (!cars[i].active) {
                    LeaveCriticalSection(&csCars);
                    continue;
                }

                int r = cars[i].row;
                int ccol = cars[i].col;

                //calcula proxima posição com base na direção
                car_next_position(&cars[i], &next_row, &next_col);

                //desocupa sempre o centro (posição atual)
                if (r >= 0 && r < MAP_ROWS && ccol >= 0 && ccol < MAP_COLS) {
                    if (car_at[r][ccol] == i) {
                        car_at[r][ccol] = -1;
                    }
                }

                //se saiu da tela, marca carro como inativo
                if (next_row < 0 || next_row >= MAP_ROWS ||
                    next_col < 0 || next_col >= MAP_COLS) {

                    cars[i].active = 0;
                } else {
                    //ignora se a célula de saída está ocupada ou não, simplesmente colocamos o carro lá
                    int other = car_at[next_row][next_col];
                    if (other != -1 && other != i) {
                        cars[other].active = 0;
                    }

                    cars[i].row = next_row;
                    cars[i].col = next_col;
                    car_at[next_row][next_col] = i;
                }

                soltou = cars[i].holding_sem;
                cars[i].holding_sem = 0;

                LeaveCriticalSection(&csCars);

                if (soltou) {
                    ReleaseSemaphore(cruzamento_sem, 1, NULL);
                }
            }
            else {
                //movimento normal longe do centro
                EnterCriticalSection(&csCars);

                if (!cars[i].active) {
                    LeaveCriticalSection(&csCars);
                    continue;
                }

                int r = cars[i].row;
                int ccol = cars[i].col;
                car_next_position(&cars[i], &next_row, &next_col);

                //saiu da tela?
                if (next_row < 0 || next_row >= MAP_ROWS ||
                    next_col < 0 || next_col >= MAP_COLS) {

                    if (car_at[r][ccol] == i) car_at[r][ccol] = -1;
                    cars[i].active = 0;

                } else if (car_at[next_row][next_col] == -1 &&
                           !(next_row == mid_row && next_col == mid_col)) {
                    //celula livre e nao é o centro (o centro é tratado nos casos acima)
                    if (car_at[r][ccol] == i) car_at[r][ccol] = -1;
                    cars[i].row = next_row;
                    cars[i].col = next_col;
                    car_at[next_row][next_col] = i;
                } else {
                    //celula ocupada, entao não anda, formando uma fila de carros
                }

                LeaveCriticalSection(&csCars);
            }
        }

        Sleep(200);
    }

    return 0;
}

//thread que lê entrada do usuário para controlar os semáforos
DWORD WINAPI input_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        if (_kbhit()) { //tem tecla pressionada?
            int ch = _getch(); //lê uma tecla sem eco

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

        Sleep(50); //evita usar 100% de CPU
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
        if (!cars[i].active) continue;
        int r = cars[i].row;
        int c = cars[i].col;
        if (r >= 0 && r < MAP_ROWS && c >= 0 && c < MAP_COLS) {
            tmp[r][c] = 'c';
        }
    }
    LeaveCriticalSection(&csCars);

    //limpa tela
    system("cls");

    //cabeçalho com cores e estados dos semáforos
    EnterCriticalSection(&csLights);

    printf("Controles: ");
    set_color(COLOR_YELLOW);
    printf("[1] N-S verde");
    set_color(COLOR_DEFAULT);
    printf("  ");

    set_color(COLOR_YELLOW);
    printf("[2] L-O verde");
    set_color(COLOR_DEFAULT);
    printf("  ");

    set_color(COLOR_YELLOW);
    printf("[q] sair\n");
    set_color(COLOR_DEFAULT);

    printf("\n");

    //semaforo NS (norte sul)
    printf("N-S (Norte-Sul)  ");
    if (light_ns == LIGHT_GREEN) {
        set_color(COLOR_GREEN);
        printf("[ABERTO]");
    } else {
        set_color(COLOR_RED);
        printf("[FECHADO]");
    }
    set_color(COLOR_DEFAULT);

    printf("    ");

    // semáforo LO (leste oeste)
    printf("L-O (Leste-Oeste) ");
    if (light_ew == LIGHT_GREEN) {
        set_color(COLOR_GREEN);
        printf("[ABERTO << >>]");
    } else {
        set_color(COLOR_RED);
        printf("[FECHADO << >>]");
    }
    set_color(COLOR_DEFAULT);

    printf("\n\n");

    LeaveCriticalSection(&csLights);

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

    //tenta reutilizar slot inativo
    for (int i = 0; i < car_count; i++) {
        if (!cars[i].active) {
            car_init(&cars[i], dir);
            int r = cars[i].row;
            int c = cars[i].col;
            if (car_at[r][c] == -1) {
                car_at[r][c] = i;
            } else {
                //posição inicial ocupada, entao não usa esse carro
                cars[i].active = 0;
                LeaveCriticalSection(&csCars);
                return -1;
            }
            LeaveCriticalSection(&csCars);
            return i;
        }
    }

    //se não há inativos, tenta criar novo
    if (car_count < MAX_CARS) {
        int id = car_count;
        car_init(&cars[id], dir);
        int r = cars[id].row;
        int c = cars[id].col;
        if (car_at[r][c] == -1) {
            car_at[r][c] = id;
            car_count++;
            LeaveCriticalSection(&csCars);
            return id;
        } else {
            cars[id].active = 0;
            LeaveCriticalSection(&csCars);
            return -1;
        }
    }

    LeaveCriticalSection(&csCars);
    return -1; //sem espaço
}

//thread que spawna carros N-S a cada um segundo e meio
DWORD WINAPI spawner_ns_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        //tbm da pra alternar entre criar no norte e no sul
        spawn_car(DIR_NORTH);
        Sleep(1500);//a cada um segundo e meio
    }

    return 0;
}

//thread que spawna carros L-O a cada 1 segundo
DWORD WINAPI spawner_ew_thread(LPVOID arg) {
    (void)arg;

    while (game_running) {
        spawn_car(DIR_WEST);
        Sleep(1000);// a cada segundo
    }

    return 0;
}

int main(void) {
    map_init();

    //pra lidar com cores no console
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    //inicializa mapa de ocupação
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            car_at[i][j] = -1;
        }
    }

    //inicializa o vetor de carros
    car_count = 0;

    //inicializa a critical section
    InitializeCriticalSection(&csCars);
    InitializeCriticalSection(&csLights);

    //estado inicial dos semaforos: N-S verde, L-O vermelho
    EnterCriticalSection(&csLights);
    light_ns = LIGHT_GREEN;
    light_ew = LIGHT_RED;
    LeaveCriticalSection(&csLights);

    //inicializa o semáforo do cruzamento:
    cruzamento_sem = CreateSemaphore(NULL, 1, 1, NULL);

    if (cruzamento_sem == NULL) {
        printf("Erro ao criar semáforo do cruzamento.\n");
        DeleteCriticalSection(&csCars);
        DeleteCriticalSection(&csLights);
        return 1;
    }

    //cria a thread que vai atualizar todos os carros
    HANDLE hCarsThread = CreateThread(NULL, 0, cars_thread, NULL, 0, NULL);
    //thread de input (controle dos semaforos e sair)
    HANDLE hInputThread = CreateThread(NULL, 0, input_thread, NULL, 0, NULL);
    //spawners
    HANDLE hSpawnNS = CreateThread(NULL, 0, spawner_ns_thread, NULL, 0, NULL);
    HANDLE hSpawnEW = CreateThread(NULL, 0, spawner_ew_thread, NULL, 0, NULL);

    if (hCarsThread == NULL || hInputThread == NULL || hSpawnNS == NULL || hSpawnEW == NULL) {
        printf("Erro ao criar alguma thread.\n");

        //avisa as threads que existem para pararem
        game_running = 0;

        //espera e fecha somente as threads que foram criadas com sucesso
        if (hCarsThread != NULL) {
            WaitForSingleObject(hCarsThread, INFINITE);
            CloseHandle(hCarsThread);
        }
        if (hInputThread != NULL) {
            WaitForSingleObject(hInputThread, INFINITE);
            CloseHandle(hInputThread);
        }
        if (hSpawnNS != NULL) {
            WaitForSingleObject(hSpawnNS, INFINITE);
            CloseHandle(hSpawnNS);
        }
        if (hSpawnEW != NULL) {
            WaitForSingleObject(hSpawnEW, INFINITE);
            CloseHandle(hSpawnEW);
        }

        //fecha o semaforo do cruzamento se foi criado antes
        if (cruzamento_sem != NULL) {
            CloseHandle(cruzamento_sem);
        }

        //destroi as critical sections
        DeleteCriticalSection(&csCars);
        DeleteCriticalSection(&csLights);

        return 1;
    }

    //loop principal, só desenha enquanto o jogo ainda é executado
    while (game_running) {
        draw_all();

        //verifica congestionamento depois de desenhar
        if (check_congestion()) {
            game_over_congestion = 1;
            game_running = 0;
            break;
        }

        Sleep(200); //taxa de atualização da tela
    }

    //acabou o jogo, limpa tela e mostra mensagem
    system("cls");

    if (game_over_congestion) {
        printf("=== GAME OVER ===\n\n");
        printf("Voce causou um congestionamento!!!\n");
        printf("Tente controlar melhor os semaforos da proxima vez!\n\n");
        printf("=================\n");
    } else {
        printf("=== JOGO ENCERRADO ===\n\n");
        printf("Voce saiu do jogo!\n\n");
        printf("=================\n");
    }

    //garante que as threads vao sair
    WaitForSingleObject(hCarsThread, INFINITE);
    WaitForSingleObject(hInputThread, INFINITE);
    WaitForSingleObject(hSpawnNS, INFINITE);
    WaitForSingleObject(hSpawnEW, INFINITE);

    //fecha os handles das threads
    CloseHandle(hCarsThread);
    CloseHandle(hInputThread);
    CloseHandle(hSpawnNS);
    CloseHandle(hSpawnEW);
    CloseHandle(cruzamento_sem);

    //libera a critical section
    DeleteCriticalSection(&csCars);
    DeleteCriticalSection(&csLights);

    getchar();
    return 0;
}
