#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 64
#define H 64
#define T 40
#define S 10
#define N_AGENTS 200
#define MAX_COST 100
#define REGEN_DRY 0.05f
#define REGEN_WET 0.15f
#define CONSUME_PER_AGENT 2.0f

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

Cell grid[H][W];
Agent agents[N_AGENTS];
Season current_season;

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

void init_grid() {
    srand(42);
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            grid[i][j].type = f_type(j, i);
            grid[i][j].resource = f_resource(grid[i][j].type);
            grid[i][j].accessible = f_accessible(grid[i][j].type, DRY);
            grid[i][j].accumulated_consumption = 0.0f;
        }
    }
}

void init_agents() {
    int created = 0;
    while (created < N_AGENTS) {
        int x = rand() % W;
        int y = rand() % H;
        if (grid[y][x].accessible) {
            agents[created].x = x;
            agents[created].y = y;
            agents[created].energy = 100.0f;
            created++;
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

int main() {
    current_season = DRY;
    init_grid();
    init_agents();

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

    return 0;
}
