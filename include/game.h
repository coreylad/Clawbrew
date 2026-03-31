#ifndef CLAWBREW_GAME_H
#define CLAWBREW_GAME_H

#include <tamtypes.h>

/* Screen dimensions */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

/* Game constants */
#define MAX_COLLECTIBLES  8
#define MAX_ENEMIES       4
#define PLAYER_SIZE       32
#define COLLECTIBLE_SIZE  16
#define ENEMY_SIZE        28
#define COLLECTIBLE_SCORE 100

/* Player structure */
typedef struct {
    float x;
    float y;
    float speed;
    u64 color;
} Player;

/* Collectible (coins/powerups) */
typedef struct {
    float x;
    float y;
    int active;
    u64 color;
} Collectible;

/* Simple enemy that moves back and forth */
typedef struct {
    float x;
    float y;
    float dx;       /* horizontal speed */
    float dy;       /* vertical speed */
    int active;
    u64 color;
} Enemy;

/* Game state structure */
typedef struct {
    Player player;
    Collectible collectibles[MAX_COLLECTIBLES];
    Enemy enemies[MAX_ENEMIES];
    int running;
    int score;
    int lives;
    int level;
    int frame_count;
    int collectibles_left;
} GameState;

/* Function prototypes */
void init_pad(void);
void read_input(GameState *state);
void spawn_level(GameState *state);
void update_collectibles(GameState *state);
void update_enemies(GameState *state);
int check_collision(float ax, float ay, float asize, float bx, float by, float bsize);

#endif /* CLAWBREW_GAME_H */
