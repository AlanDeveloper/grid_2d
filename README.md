# Seasonal Territory Mobility Simulation

Sequential base implementation of a 2D grid simulation where indigenous family groups move across a territory based on resource availability and seasonal changes.

## Requirements

- GCC
- Make

## How to run

```bash
make all
make run
```

## Parameters

All main parameters are defined at the top of `simulation_base.c` as `#define` constants:

| Parameter         | Default  | Description                  |
|-------------------|----------|------------------------------|
| W / H             | 64 / 64  | Grid dimensions              |
| T                 | 40       | Total simulation cycles      |
| S                 | 10       | Cycles per season            |
| N_AGENTS          | 200      | Number of agents             |
| MAX_COST          | 100      | Max synthetic load per agent |
| REGEN_DRY         | 0.10     | Resource regen rate (dry)    |
| REGEN_WET         | 0.30     | Resource regen rate (wet)    |
| CONSUME_PER_AGENT | 1.0      | Resource consumed per agent  |
| INITIAL_SEASON    | DRY      | Starting season              |

## Project structure

- **Part 1** — sequential base: grid, agents, seasons, main loop
- **Part 2** — MPI: domain partitioning, halo exchange, agent migration between processes
- **Part 3** — OpenMP: intra-node parallelism, performance metrics
