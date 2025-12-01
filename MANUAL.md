# Manual do Jogo – Cruzamento Concorrente

## 1. Visão geral do jogo

Este jogo simula um **cruzamento de trânsito** visto de cima, com carros representados pela letra `c` se movendo nas vias, um cruzamento central (região crítica), dois **semáforos de trânsito lógicos** (um para a via **Norte–Sul (NS)** e outro para a via **Leste–Oeste (L-O)**) e várias **threads** rodando ao mesmo tempo, coordenadas por **semáforos** e **regiões críticas**.

O objetivo do projeto não é “ganhar” o jogo, mas **demonstrar na prática** os conceitos de criação e uso de **threads**, sincronização com **semáforos**, exclusão mútua em **regiões críticas** e acesso concorrente a recursos compartilhados.

---

## 2. Como instalar

### 2.1. Sistema operacional

- Foi testado em Windows 11

### 2.2. Ferramentas necessárias

Você precisa de:

- Um **compilador C** que suporte `windows.h`, por exemplo:
  - **MinGW-w64 (gcc)**  
    - Ao instalar, certifique-se de que o diretório `bin` do MinGW (`.../mingw64/bin`) esteja no `PATH`.
- Um terminal, por exemplo:
  - **PowerShell**, ou
  - **Git Bash**, ou
  - **CMD**.

### 2.3. Arquivos do projeto

Na pasta do projeto devem existir, no mínimo:

- `main.c`
- `map.c`, `map.h`
- `car.c`, `car.h`
- `MANUAL.md` (este documento)

O código foi organizado em múltiplos arquivos para separar responsabilidades:

- `map.*` - tudo relacionado ao mapa (tamanho, desenho das vias, cruzamento);
- `car.*` - definição da estrutura `Car`, direção, posição, inicialização;
- `main.c` - lógica do jogo, criação de threads, semáforos, loop principal, desenho, etc.

---

## 3. Como compilar

> **Importante:**  
> Os comandos abaixo assumem que você já está na pasta do projeto, onde ficam `main.c`, `map.c` e `car.c`.

### 3.1. Compilando com `gcc` (MinGW-w64)

No **PowerShell** ou **Git Bash**, execute:

```bash
gcc main.c map.c car.c -Wall -Wextra -std=c11 -o cruzamento.exe
```
Se o comando não for encontrado (gcc: command not found), significa que o MinGW não está instalado ou não está no seu PATH.

## 4. Como executar o jogo

Após compilar, o arquivo `cruzamento.exe` será gerado na mesma pasta.

### 4.1. Comando para executar

```bash
.\cruzamento.exe
```

O jogo vai abrir no console, mostrando:

- um cabeçalho com os controles e o estado dos semáforos, e

- o **mapa** do cruzamento, com os carros (`c`) se movimentando.

Para sair do jogo, use a tecla `q` (explicado na próxima seção).

---

## 5. Como jogar

### 5.1. Controles

No topo da tela aparecem os controles, por exemplo:

- `1` → abre o semáforo **Norte–Sul (NS)** e fecha o Leste–Oeste (L-O);

- `2` → abre o semáforo **Leste–Oeste (L-O)** e fecha o Norte–Sul (NS);

- `q` → encerra o jogo.

Em detalhes:

- Tecla `1`

    - N-S fica **ABERTO (VERDE)**;

    - L-O fica **FECHADO (VERMELHO)**.

- Tecla `2`

    - L-O fica **ABERTO (VERDE)**;

    - N-S fica **FECHADO (VERMELHO)**.

- Tecla `q`

    - Define uma variável global `game_running = 0`, sinalizando para todas as threads que o jogo deve terminar;

    - A thread principal finaliza o loop de desenho e exibe uma mensagem de encerramento.

### 5.2. O que aparece na tela

No console, você verá:

- Um cabeçalho, com:

    - os controles (`[1] N-S verde`, `[2] L-O verde`, `[q] sair`);

    - o estado de cada semáforo, claramente visível:

        - `N-S (Norte-Sul) [ABERTO]` em verde ou `[FECHADO]` em vermelho;

        - `L-O (Leste-Oeste) [ABERTO]/[FECHADO]` também em cores.

- Abaixo, o mapa:

    - uma via horizontal;

    - uma via vertical;

    - o cruzamento central (região crítica);

    - carros `c` se movendo de acordo com sua direção (N, S, L, O).

### 5.3. Dinâmica do jogo

1- Threads geradoras de carros criam novos veículos nas bordas das vias após certos intervalos de tempo.

2- A thread de simulação (`cars_thread`) atualiza as posições dos carros:

- carros que vêm de cima descem;

- de baixo sobem;

- da esquerda vão para a direita;

- da direita vão para a esquerda.

3- Ao se aproximar do cruzamento:

- se o semáforo da sua direção estiver vermelho, o carro para antes do centro;

- se estiver verde, o carro pode tentar entrar no cruzamento (desde que o semáforo de SO do centro esteja livre).

4- Dentro do cruzamento, apenas um carro por vez é permitido:

- isso é controlado por um semáforo de SO (cruzamento_sem);

- quando um carro sai do centro, ele libera o semáforo para outro entrar.

O jogador deve alternar os semáforos com 1 e 2 para:

- manter o fluxo de trânsito;

- evitar que muitos carros fiquem acumulados em uma só direção.

## 6. Threads utilizadas e por que foram usadas

O jogo foi projetado para ser um exemplo prático de concorrência, semelhante ao que ocorre em Sistemas Operacionais.

### 6.1. Thread principal (função main)

- É a thread que inicia o programa.

- Responsável por:

    - fazer a inicialização das estruturas (map_init, vetores de carros, semáforos, etc.);

    - criar as outras threads (CreateThread);

    - executar o loop de desenho:

    ```c
    while (game_running) {
        draw_all();
        Sleep(...);
    }
    ```
- Papel conceitual:
    - age como um "loop de interface", desenhando o estado atual do sistema.

### 6.2. Thread de simulação dos carros (`cars_thread`)

- Executa um laço infinito (enquanto game_running for verdadeiro):

    - para cada carro ativo no vetor `cars[]`:

        - calcula a próxima posição;

        - verifica se deve parar, entrar no cruzamento ou continuar andando;

        - atualiza `cars[i].row`, `cars[i].col`, `cars[i].active` e a matriz `car_at[row][col]`.

- Usa `CRITICAL_SECTION` para proteger o acesso às estruturas compartilhadas.

- Papel conceitual:

    - representa um “núcleo de simulação”, semelhante ao que um SO faria gerenciando estados de vários processos (aqui, carros).

### 6.3. Thread de entrada do jogador (`input_thread`)

- Fica em loop chamando `_kbhit()` / `_getch()`:

    - se o jogador apertar `1` ou `2`, a thread atualiza os semáforos lógicos `light_ns` e `light_ew`;

    - se o jogador apertar `q`, define `game_running = 0` para terminar o jogo.

- Protege o acesso a `light_ns` e `light_ew` com `csLights`.

- Papel conceitual:

    - trata entrada assíncrona do usuário, sem bloquear a simulação nem a renderização.

### 6.4. Threads geradoras de carros (`spawner_ns_thread` e `spawner_ew_thread`)

- Cada uma roda em loop, por exemplo:

```c
while (game_running) {
    spawn_car(DIR_NORTH); //ou DIR_SOUTH, DIR_WEST, DIR_EAST
    Sleep(2000); //intervalo entre novos carros
}
```

- Chamam `spawn_car(...)`, que inicializa um novo `Car` em uma posição de borda e marca sua presença na matriz car_at.

- Papel conceitual:

    - representam “fontes de processos” que chegam ao sistema ao longo do tempo (trânsito contínuo).

## 7. Uso de semáforos e exclusão mútua

O projeto utiliza dois mecanismos de sincronização principais:

    1- `CRITICAL_SECTION` → exclusão mútua (mutex) para acesso a dados compartilhados.

    2- **Semáforo binário** (`cruzamento_sem`) → controle da região crítica central.

Além disso, há **semáforos lógicos de trânsito** (variáveis) para decidir qual direção está aberta.

### 7.1. `CRITICAL_SECTION` – proteção das estruturas de dados

Duas `CRITICAL_SECTION` são utilizadas:

- `csCars`
    Protege:

    - o vetor `cars[]`, que guarda a posição, direção, estado e flag `holding_sem` de cada carro;

    - a matriz `car_at[row][col]`, que indica qual carro (se algum) ocupa uma determinada célula do mapa.

- `csLights`
    Protege:

    - `light_ns` (estado lógico do semáforo Norte–Sul);

    - `light_ew` (estado lógico do semáforo Leste–Oeste).

Exemplo de uso em pseudocódigo:

```c
// Atualização dos carros
EnterCriticalSection(&csCars);
// lê e escreve em cars[] e car_at[][]
LeaveCriticalSection(&csCars);

// Atualização dos semáforos lógicos
EnterCriticalSection(&csLights);
light_ns = LIGHT_GREEN;
light_ew = LIGHT_RED;
LeaveCriticalSection(&csLights);
```

**Motivação de SO:**

- Evitar **race conditions** onde duas threads (por exemplo, `cars_thread` e um spawner) mexeriam simultaneamente nas mesmas estruturas;

- Garantir que, dentro de cada região crítica, **somente uma thread** execute o código que acessa aquelas variáveis.

### 7.2. Semáforo binário do cruzamento (`cruzamento_sem`)

O centro do cruzamento é um recurso compartilhado: **apenas um carro** por vez pode estar no centro.

Para modelar isso, é usado um semáforo da API do Windows:

```c
HANDLE cruzamento_sem = CreateSemaphore(NULL, 1, 1, NULL);
```

- Valor inicial: 1 → cruzamento livre.

- Quando um carro entra no centro, o semáforo é decrementado para 0.

- Quando o carro sai do centro, o semáforo volta a 1.

Trechos típicos de uso:

**Entrada no cruzamento:**

```c
if (carro está prestes a entrar no centro && semáforo lógico da direção está VERDE) {
    // tenta adquirir o semáforo da região crítica
    WaitForSingleObject(cruzamento_sem, INFINITE);

    // se passou daqui, o cruzamento está reservado para este carro
    // atualiza a posição para o centro
    cars[i].holding_sem = 1;
}
```

**Saída do cruzamento:**

```c
if (carro está no centro e vai sair) {
    // atualiza posição para fora do centro
    soltou = cars[i].holding_sem;
    cars[i].holding_sem = 0;

    if (soltou) {
        ReleaseSemaphore(cruzamento_sem, 1, NULL);
    }
}
```

**Motivação de SO:**

- `WaitForSingleObject` representa a operação típica `P()` ou `wait()` de semáforo:

    - se o recurso estiver livre (semáforo 1), o carro passa e o valor vira 0;

    - se estiver ocupado (0), a thread fica bloqueada até outro carro liberar.

- `ReleaseSemaphore` representa `V()` ou `signal()`:

    - o carro que estava usando o recurso termina, incrementa o semáforo e possibilita que outro entre.

Isso garante **exclusão mútua** no centro do cruzamento, independentemente de quantas threads tentem colocar carros lá.

### 7.3. Semáforos de trânsito lógicos

Além do semáforo de SO, o projeto implementa semáforos de trânsito lógicos:

- `light_ns` e `light_ew`, com valores `LIGHT_RED` ou `LIGHT_GREEN`.

Uso:

- A **thread de input** altera esses valores com base nas teclas (1 e 2).

- A **thread de simulação dos carros** lê esses valores para decidir se um carro pode tentar entrar no centro, ou se deve ficar parado antes do cruzamento.

Essas variáveis implementam a **política de quem tem prioridade** (qual eixo está aberto), enquanto o semáforo binário de SO (`cruzamento_sem`) é o **mecanismo de exclusão mútua** que efetivamente limita a um carro por vez na região crítica.

## 8. Decisões de projeto e simplificações

- O jogo é feito em modo texto, usando `printf` e caracteres simples, para focar nos conceitos de SO em vez de interface gráfica.

- O trânsito é **simplificado**:

    - não há física realista;

    - a colisão e filas são modeladas de maneira básica;

    - em alguns casos extremos, carros podem “sumir” ou ser substituídos ao evitar travamentos definitivos do cruzamento.

- Essas simplificações são intencionais:

    - o objetivo principal é mostrar **threads**, **semáforos** e **exclusão mútua** em ação, e não fazer um simulador completo de trânsito.

Mesmo assim, o jogo demonstra:

- como múltiplas threads podem compartilhar e atualizar o mesmo estado global;

- como `regiões críticas` são protegidas com mutex (`CRITICAL_SECTION`);

- como um `semáforo binário` pode representar um recurso compartilhado (o centro do cruzamento);

- como a lógica de semáforos de trânsito é análoga a políticas de escalonamento e controle de acesso a recursos em Sistemas Operacionais.