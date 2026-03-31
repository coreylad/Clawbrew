#ifndef CLAWBREW_GAME_H
#define CLAWBREW_GAME_H

#include <tamtypes.h>

/* Screen dimensions */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

/* Game constants */
#define MAX_COLLECTIBLES  8
#define MAX_ENEMIES       6
#define MAX_POWERUPS      2
#define PLAYER_SIZE       32
#define COLLECTIBLE_SIZE  16
#define ENEMY_SIZE        28
#define POWERUP_SIZE      20
#define COLLECTIBLE_SCORE 100
#define POWERUP_DURATION  180  /* frames of powerup effect */

/* Game states */
#define STATE_TITLE    0
#define STATE_PLAYING  1
#define STATE_PAUSED   2
#define STATE_GAMEOVER 3

/* Enemy types */
#define ENEMY_BOUNCER  0   /* Bounces off walls */
#define ENEMY_CHASER   1   /* Slowly pursues player */

/* Powerup types */
#define POWERUP_SPEED  0   /* Temporary speed boost */
#define POWERUP_SHIELD 1   /* Temporary invincibility */

/* Player structure */
typedef struct {
    float x;
    float y;
    float speed;
    float base_speed;
    u64 color;
    int shield_active;
    int speed_boost;
    int powerup_timer;
} Player;

/* Collectible (coins/powerups) */
typedef struct {
    float x;
    float y;
    int active;
    u64 color;
} Collectible;

/* Enemy with type support */
typedef struct {
    float x;
    float y;
    float dx;
    float dy;
    int active;
    int type;           /* ENEMY_BOUNCER or ENEMY_CHASER */
    u64 color;
} Enemy;

/* Powerup pickup */
typedef struct {
    float x;
    float y;
    int active;
    int type;           /* POWERUP_SPEED or POWERUP_SHIELD */
    u64 color;
} Powerup;

/* Game state structure */
typedef struct {
    Player player;
    Collectible collectibles[MAX_COLLECTIBLES];
    Enemy enemies[MAX_ENEMIES];
    Powerup powerups[MAX_POWERUPS];
    int game_state;     /* STATE_TITLE, STATE_PLAYING, etc */
    int running;
    int score;
    int high_score;
    int lives;
    int level;
    int frame_count;
    int collectibles_left;
    int title_blink;
    int pause_blink;
    int death_flash;
} GameState;

/* Function prototypes */
void init_pad(void);
void read_input(GameState *state);
void spawn_level(GameState *state);
void update_collectibles(GameState *state);
void update_enemies(GameState *state);
void update_powerups(GameState *state);
int check_collision(float ax, float ay, float asize, float bx, float by, float bsize);

/* Rendering */
void render_title(GSGLOBAL *gsGlobal, GameState *state);
void render_pause(GSGLOBAL *gsGlobal);
void render_gameover(GSGLOBAL *gsGlobal, GameState *state);

#endif /* CLAWBREW_GAME_H */
