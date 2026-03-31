#ifndef CLAWBREW_GAME_H
#define CLAWBREW_GAME_H

#include <tamtypes.h>

/* Screen dimensions */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

/* Player structure */
typedef struct {
    float x;
    float y;
    float speed;
    u64 color;
} Player;

/* Game state structure */
typedef struct {
    Player player;
    int running;
    int score;
} GameState;

/* Function prototypes */
void init_pad(void);
void read_input(GameState *state);

#endif /* CLAWBREW_GAME_H */
