void process_agents() {

    Agent kept_agents[N_AGENTS];
    int kept_count = 0;

    out_up_count = 0;
    out_down_count = 0;

    // processar agentes
    #pragma omp parallel
    {
        // Buffers privados por thread
        Agent kept_local[N_AGENTS];
        Agent up_local[N_AGENTS];
        Agent down_local[N_AGENTS];

        int kept_local_count = 0;
        int up_local_count = 0;
        int down_local_count = 0;

        #pragma omp for
        for (int a = 0; a < local_agents_count; a++) {

            int ax = agents[a].x;
            int ay = agents[a].y;
            float r = local_grid[ay][ax].resource;

            // carga sintética computacional
            run_synthetic_load(r);

            int nx, ny;
            // decide o deslocamento ou permanência no subgrid
            best_neighbor(ax, ay, &nx, &ny);

            agents[a].x = nx;
            agents[a].y = ny;
            agents[a].energy -= 1.0f;
            if (agents[a].energy < 0.0f) agents[a].energy = 0.0f;

            // se o destino é local
            if (ny >= 1 && ny <= local_H) {

                #pragma omp atomic
                local_grid[ny][nx].accumulated_consumption += CONSUME_PER_AGENT;

                kept_local[kept_local_count++] = agents[a];

            } 
            // senão, o agente vai migrar
            else {

                if (ny == 0) { // migra pra CIMA
                    agents[a].y = local_H;
                    up_local[up_local_count++] = agents[a]; // adiciona no buffer

                } else if (ny == local_H + 1) { // migra pra BAIXO
                    agents[a].y = 1;
                    down_local[down_local_count++] = agents[a]; // adiciona no buffer de saída
                }
            }
        }

        // 🔒 Região crítica apenas para juntar resultados
        #pragma omp critical
        {
            for (int i = 0; i < kept_local_count; i++)
                kept_agents[kept_count++] = kept_local[i];

            for (int i = 0; i < up_local_count; i++)
                out_up[out_up_count++] = up_local[i];

            for (int i = 0; i < down_local_count; i++)
                out_down[out_down_count++] = down_local[i];
        }
    }

    // atualiza a lista oficial do processo apenas com os agentes que ficaram 
    for (int i = 0; i < kept_count; i++)
        agents[i] = kept_agents[i];

    local_agents_count = kept_count;
}

void update_local_grid() {

    float regen = (current_season == DRY) ? REGEN_DRY : REGEN_WET;

    // paraleliza o loop externo
    #pragma omp parallel for
    for (int i = 1; i <= local_H; i++) { // iteração apenas no grid local, ignorando halos

        for (int j = 0; j < W; j++) {

            local_grid[i][j].resource += 
                regen - local_grid[i][j].accumulated_consumption;
                
            // garante que os recursos fiquem na faixa entre 0 e 100
            if (local_grid[i][j].resource < 0.0f)
                local_grid[i][j].resource = 0.0f;

            if (local_grid[i][j].resource > 100.0f)
                local_grid[i][j].resource = 100.0f;

            local_grid[i][j].accumulated_consumption = 0.0f;
        }
    }
}