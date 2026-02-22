#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpih.>

#define W 64
#define H 64
#define T 40
#define S 10
#define N_AGENTS 200
#define MAX_COST 100
#define REGEN_DRY 0.10f
#define REGEN_WET 0.30f
#define CONSUME_PER_AGENT 1.0f
#define INITIAL_SEASON DRY

typedef enum { DRY = 0, WET = 1 } Season;
typedef enum { VILLAGE = 0, FISHING, GATHERING, FARMLAND, RESTRICTED } CellType;

typedef struct {
    CellType type;
    float resource;
    int accessible;
    float accumulated_consumption;
} Cell;

typedef struct {
    int x, y;
    float energy;
} Agent;

CellType f_type(int gx, int gy) {
    int v = (gx * 7 + gy * 13) % 5;
    return (CellType) v;
}

float f_resource(CellType type) {
    switch (type) {
        case VILLAGE:    return 80.0f;
        case FISHING:    return 60.0f;
        case GATHERING:  return 50.0f;
        case FARMLAND:   return 70.0f;
        case RESTRICTED: return 0.0f;
        default:         return 0.0f;
    }
}

int f_accessible(CellType type, Season s) {
    if (type == RESTRICTED) return 0;
    if (type == FISHING && s == DRY) return 0;
    return 1;
}

void init_local_grid() {
    srand(42);

    // iteração de 1 até local_H, sendo 0 e local_h + 1 os halos
    for (int i = 0; i <= local_H; i++) {
        for (int j = 0; j < W; j++) {
            int gx = j;                     // coordenada x é igual à global
            int gy = offsetY + (i - 1);     // mapeia a coluna local para a coluna global
            
            // preenche as células locais de acordo com as regras globais
            local_grid[i][j].type = f_type(gx, gy);
            local_grid[i][j].resource = f_resource(grid[i][j].type);
            local_grid[i][j].accessible = f_accessible(grid[i][j].type, INITIAL_SEASON);
            local_grid[i][j].accumulated_consumption = 0.0f;
        }
    }
}

void init_local_agents() {
    // inicialização do número de agentes do processo atual
    local_agents_count = 0;

    // todos os processos iteram sobre o número total de agentes
    for (int a = 0; a < N_AGENTS; a++) {
        // gera as mesmas coordenadas globais em todos os processos
        int global_x = rand() % W;
        int global_y = rand() % H;

        // verifica se as coordenadas geradas estão no subgrid do atual processo
        if(global_y >= offsetY && global_y < offsetY + local_H){

            // converte a coordenada global de Y para a local
            // soma 1 para considerar o halo
            int local_y = (global_y - offsetY) + 1;

            // atualização do grid local
            local_grid[local_y][global_x].type = VILLAGE;
            local_grid[local_y][global_x].resource = f_resource(VILLAGE);
            local_grid[local_y][global_x].accessible = f_accessible(VILLAGE, DRY);
            
            // salva o agente no vetor local do processo
            agents[local_agents_count].x = global_x;
            agents[local_agents_count].y = local_y;
            agents[local_agents_count].energy = 100.0f;

            // incrementa o número de agentes locais
            local_agents_count++;
        }
    }
}

void run_synthetic_load(float resource) {
    int cost = (int)(resource * MAX_COST / 100.0f);
    if (cost > MAX_COST) cost = MAX_COST;
    volatile float acc = 1.0f;
    for (int c = cost; c >= 0; c--) {
        acc = acc * 1.00001f + 0.00001f;
    }
    (void)acc;
}

void update_season(int t) {
    if (t > 0 && t % S == 0) {
        current_season = (current_season == DRY) ? WET : DRY;
        for (int i = 0; i < H; i++)
            for (int j = 0; j < W; j++)
                grid[i][j].accessible = f_accessible(grid[i][j].type, current_season);
    }
}

int best_neighbor(int ax, int ay, int *nx, int *ny) {
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    float best = grid[ay][ax].resource;
    *nx = ax;
    *ny = ay;
    for (int d = 0; d < 4; d++) {
        int vx = ax + dx[d];
        int vy = ay + dy[d];
        if (vx < 0 || vx >= W || vy < 0 || vy >= H) continue;
        if (!grid[vy][vx].accessible) continue;
        if (grid[vy][vx].resource > best) {
            best = grid[vy][vx].resource;
            *nx = vx;
            *ny = vy;
        }
    }
    return (*nx != ax || *ny != ay);
}

void process_agents() {
    for (int a = 0; a < N_AGENTS; a++) {
        int ax = agents[a].x;
        int ay = agents[a].y;
        float r = grid[ay][ax].resource;

        run_synthetic_load(r);

        int nx, ny;
        best_neighbor(ax, ay, &nx, &ny);
        agents[a].x = nx;
        agents[a].y = ny;

        grid[ny][nx].accumulated_consumption += CONSUME_PER_AGENT;
        agents[a].energy -= 1.0f;
        if (agents[a].energy < 0.0f) agents[a].energy = 0.0f;
    }
}

void update_grid() {
    float regen = (current_season == DRY) ? REGEN_DRY : REGEN_WET;
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            grid[i][j].resource += regen - grid[i][j].accumulated_consumption;
            if (grid[i][j].resource < 0.0f) grid[i][j].resource = 0.0f;
            if (grid[i][j].resource > 100.0f) grid[i][j].resource = 100.0f;
            grid[i][j].accumulated_consumption = 0.0f;
        }
    }
}

float avg_resource() {
    float total = 0.0f;
    for (int i = 0; i < H; i++)
        for (int j = 0; j < W; j++)
            total += grid[i][j].resource;
    return total / (W * H);
}

float avg_energy() {
    float total = 0.0f;
    for (int a = 0; a < N_AGENTS; a++)
        total += agents[a].energy;
    return total / N_AGENTS;
}

// variáveis globais MPI
int rank        /**< Identificador do processo atual do MPI. */
int size;       /**< Número total de processos MPI rodando. */
int local_H;    /**< Altura da fatia do subgrid local do processo MPI. */
int offsetY;    /**< Deslocamento vertical global onde começa a atual fatia. */

/** Matriz de alocação dinâmica da fatia do grid global.
 * Deve incluir 2 linhas extras para a troca de bordas (halo).
 */
Cell local_grid**;
Agent agents[N_AGENTS];
int local_agents_count = 0;
Season current_season;

int main(int argc, char** argv) {

    // inicialização do MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // particionando o grid global em fatias horizontais
    local_H = H / size;
    offsetY = rank * local_H;

    // alocação do subgrid local com halo em cima e embaixo
    local_grid = (Cell **)malloc((local_H + 2) * sizeof(Cell *));
    for(int i = 0; i < local_H + 2; i++){
        local_grid[i] = (Cell *)malloc(W * sizeof(Cell));
    }

    current_season = INITIAL_SEASON;

    init_local_grid();
    init_local_ agents();

    printf("Cycle | Season | Avg Resource | Avg Energy\n");
    printf("------+--------+--------------+-----------\n");

    for (int t = 0; t < T; t++) {
        update_season(t);
        process_agents();
        update_grid();

        if (t % 5 == 0) {
            printf("  %3d | %s  |    %.2f      |   %.2f\n",
                t,
                current_season == DRY ? "DRY " : "WET ",
                avg_resource(),
                avg_energy());
        }
    }

    // finalização do MPI
    MPI_Finalize();
    return 0;
}
