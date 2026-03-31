/*
 * Clawbrew - PS2 Homebrew Game
 * Built with ps2sdk, gsKit, libpad
 * 
 * Community-driven PS2 homebrew project
 * https://github.com/coreylad/Clawbrew
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <draw.h>
#include <pad.h>
#include "game.h"

/* Screen dimensions */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

/* Colors (RGBA) */
#define COLOR_BLACK    GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00)
#define COLOR_WHITE    GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00)
#define COLOR_RED      GS_SETREG_RGBAQ(0xFF, 0x00, 0x00, 0x80, 0x00)
#define COLOR_GREEN    GS_SETREG_RGBAQ(0x00, 0xFF, 0x00, 0x80, 0x00)
#define COLOR_BLUE     GS_SETREG_RGBAQ(0x00, 0x00, 0xFF, 0x80, 0x00)
#define COLOR_YELLOW   GS_SETREG_RGBAQ(0xFF, 0xFF, 0x00, 0x80, 0x00)
#define COLOR_CYAN     GS_SETREG_RGBAQ(0x00, 0xFF, 0xFF, 0x80, 0x00)
#define COLOR_ORANGE   GS_SETREG_RGBAQ(0xFF, 0x88, 0x00, 0x80, 0x00)
#define COLOR_PURPLE   GS_SETREG_RGBAQ(0xAA, 0x00, 0xFF, 0x80, 0x00)
#define COLOR_DARKGRAY GS_SETREG_RGBAQ(0x33, 0x33, 0x33, 0x80, 0x00)
#define COLOR_DARKBLUE GS_SETREG_RGBAQ(0x00, 0x00, 0x44, 0x80, 0x00)

/* Initialize the graphics synthesizer */
GSGLOBAL *init_gs(void) {
    GSGLOBAL *gsGlobal = gsKit_init_global();

    gsGlobal->Mode = GS_MODE_NTSC;
    gsGlobal->Interlace = GS_INTERLACED;
    gsGlobal->Field = GS_FIELD;
    gsGlobal->Width = SCREEN_WIDTH;
    gsGlobal->Height = SCREEN_HEIGHT;
    gsGlobal->PSM = GS_PSMCT32;
    gsGlobal->PSMZ = GS_PSMZ32;
    gsGlobal->Dithering = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;
    gsGlobal->ZBuffering = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_clear(gsGlobal, COLOR_BLACK);

    return gsGlobal;
}

/* Initialize controller */
void init_pad(void) {
    padInit(0);
    padPortOpen(0, 0, NULL);
    
    /* Wait for pad to be ready */
    while (!padGetState(0, 0)) {
        nopdelay();
    }
}

/* AABB collision check */
int check_collision(float ax, float ay, float asize, float bx, float by, float bsize) {
    return (ax < bx + bsize) && (ax + asize > bx) &&
           (ay < by + bsize) && (ay + asize > by);
}

/* Read controller input */
void read_input(GameState *state) {
    struct padButtonStatus buttons;
    
    if (padRead(0, 0, &buttons) != 0) {
        int paddata = 0xffff ^ buttons.btns;
        
        /* D-pad movement */
        if (paddata & PAD_UP)    state->player.y -= state->player.speed;
        if (paddata & PAD_DOWN)  state->player.y += state->player.speed;
        if (paddata & PAD_LEFT)  state->player.x -= state->player.speed;
        if (paddata & PAD_RIGHT) state->player.x += state->player.speed;
        
        /* Start to quit */
        if (paddata & PAD_START) state->running = 0;
        
        /* Keep player on screen */
        if (state->player.x < 0) state->player.x = 0;
        if (state->player.y < 0) state->player.y = 0;
        if (state->player.x > SCREEN_WIDTH - PLAYER_SIZE) state->player.x = SCREEN_WIDTH - PLAYER_SIZE;
        if (state->player.y > SCREEN_HEIGHT - PLAYER_SIZE) state->player.y = SCREEN_HEIGHT - PLAYER_SIZE;
    }
}

/* Spawn a level with collectibles and enemies */
void spawn_level(GameState *state) {
    int i;
    int cx, cy;
    int spacing = SCREEN_WIDTH / (MAX_COLLECTIBLES + 1);
    
    /* Place collectibles in a pattern that changes per level */
    for (i = 0; i < MAX_COLLECTIBLES; i++) {
        state->collectibles[i].x = spacing * (i + 1) - COLLECTIBLE_SIZE / 2;
        state->collectibles[i].y = 80 + ((i * 37 + state->level * 53) % (SCREEN_HEIGHT - 160));
        state->collectibles[i].active = 1;
        
        /* Alternate colors */
        if (i % 3 == 0) state->collectibles[i].color = COLOR_YELLOW;
        else if (i % 3 == 1) state->collectibles[i].color = COLOR_CYAN;
        else state->collectibles[i].color = COLOR_ORANGE;
    }
    state->collectibles_left = MAX_COLLECTIBLES;
    
    /* Place enemies - more enemies on higher levels */
    for (i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].x = 100 + (i * 140);
        state->enemies[i].y = 120 + (i * 80) % (SCREEN_HEIGHT - 200);
        state->enemies[i].dx = (i % 2 == 0) ? 2.0f + state->level * 0.5f : 0;
        state->enemies[i].dy = (i % 2 == 1) ? 2.0f + state->level * 0.5f : 0;
        state->enemies[i].active = 1;
        state->enemies[i].color = (i % 2 == 0) ? COLOR_RED : COLOR_PURPLE;
    }
}

/* Update collectible state */
void update_collectibles(GameState *state) {
    int i;
    for (i = 0; i < MAX_COLLECTIBLES; i++) {
        if (!state->collectibles[i].active) continue;
        
        if (check_collision(state->player.x, state->player.y, PLAYER_SIZE,
                            state->collectibles[i].x, state->collectibles[i].y, COLLECTIBLE_SIZE)) {
            state->collectibles[i].active = 0;
            state->score += COLLECTIBLE_SCORE;
            state->collectibles_left--;
        }
    }
}

/* Update enemy positions and check collisions with player */
void update_enemies(GameState *state) {
    int i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].active) continue;
        
        /* Move enemy */
        state->enemies[i].x += state->enemies[i].dx;
        state->enemies[i].y += state->enemies[i].dy;
        
        /* Bounce off walls */
        if (state->enemies[i].x <= 0 || state->enemies[i].x >= SCREEN_WIDTH - ENEMY_SIZE) {
            state->enemies[i].dx = -state->enemies[i].dx;
        }
        if (state->enemies[i].y <= 0 || state->enemies[i].y >= SCREEN_HEIGHT - ENEMY_SIZE) {
            state->enemies[i].dy = -state->enemies[i].dy;
        }
        
        /* Clamp to screen */
        if (state->enemies[i].x < 0) state->enemies[i].x = 0;
        if (state->enemies[i].y < 0) state->enemies[i].y = 0;
        
        /* Check collision with player */
        if (check_collision(state->player.x, state->player.y, PLAYER_SIZE,
                            state->enemies[i].x, state->enemies[i].y, ENEMY_SIZE)) {
            state->lives--;
            /* Reset player position */
            state->player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
            state->player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
            
            if (state->lives <= 0) {
                state->running = 0;
            }
        }
    }
}

/* Render a single HUD digit using sprites (0-9) */
void render_digit(GSGLOBAL *gsGlobal, int digit, float x, float y, u64 color) {
    /* Simple 5x7 pixel font using sprite blocks */
    /* Each digit is a 12x16 block for visibility */
    static const unsigned char font[][7] = {
        {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C}, /* 0 */
        {0x18,0x38,0x18,0x18,0x18,0x18,0x7E}, /* 1 */
        {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E}, /* 2 */
        {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C}, /* 3 */
        {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C}, /* 4 */
        {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C}, /* 5 */
        {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C}, /* 6 */
        {0x7E,0x06,0x0C,0x18,0x18,0x18,0x18}, /* 7 */
        {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C}, /* 8 */
        {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38}, /* 9 */
    };
    int row, col;
    
    if (digit < 0 || digit > 9) return;
    
    for (row = 0; row < 7; row++) {
        for (col = 0; col < 8; col++) {
            if (font[digit][row] & (0x80 >> col)) {
                gsKit_prim_sprite(gsGlobal,
                    x + col * 2, y + row * 2,
                    x + col * 2 + 2, y + row * 2 + 2,
                    1, color);
            }
        }
    }
}

/* Render a number as digit sprites */
void render_number(GSGLOBAL *gsGlobal, int number, float x, float y, u64 color) {
    char buf[12];
    int i, len;
    float dx;
    
    /* Convert to string */
    if (number == 0) {
        render_digit(gsGlobal, 0, x, y, color);
        return;
    }
    
    /* Count digits */
    len = 0;
    {
        int tmp = number;
        while (tmp > 0) { len++; tmp /= 10; }
    }
    
    /* Render each digit right to left positioning */
    dx = x;
    {
        int tmp = number;
        int digits[12];
        int dlen = 0;
        
        while (tmp > 0) {
            digits[dlen++] = tmp % 10;
            tmp /= 10;
        }
        
        /* Render left to right */
        for (i = dlen - 1; i >= 0; i--) {
            render_digit(gsGlobal, digits[i], dx, y, color);
            dx += 18;
        }
    }
}

/* Render the player as a colored square */
void render_player(GSGLOBAL *gsGlobal, Player *player) {
    gsKit_prim_sprite(gsGlobal, 
        player->x, player->y,
        player->x + PLAYER_SIZE, player->y + PLAYER_SIZE,
        1, player->color);
    
    /* Inner detail - smaller square */
    gsKit_prim_sprite(gsGlobal,
        player->x + 6, player->y + 6,
        player->x + PLAYER_SIZE - 6, player->y + PLAYER_SIZE - 6,
        1, COLOR_WHITE);
}

/* Render a collectible as a small diamond shape */
void render_collectible(GSGLOBAL *gsGlobal, Collectible *c) {
    float cx, cy;
    
    if (!c->active) return;
    
    cx = c->x + COLLECTIBLE_SIZE / 2;
    cy = c->y + COLLECTIBLE_SIZE / 2;
    
    /* Diamond using two triangles approximated as small sprites */
    gsKit_prim_sprite(gsGlobal,
        cx - COLLECTIBLE_SIZE/2, cy,
        cx, cy + COLLECTIBLE_SIZE/2,
        1, c->color);
    gsKit_prim_sprite(gsGlobal,
        cx, cy,
        cx + COLLECTIBLE_SIZE/2, cy + COLLECTIBLE_SIZE/2,
        1, c->color);
    gsKit_prim_sprite(gsGlobal,
        cx - COLLECTIBLE_SIZE/2, cy - COLLECTIBLE_SIZE/2,
        cx, cy,
        1, c->color);
    gsKit_prim_sprite(gsGlobal,
        cx, cy - COLLECTIBLE_SIZE/2,
        cx + COLLECTIBLE_SIZE/2, cy,
        1, c->color);
}

/* Render an enemy as a red/purple square */
void render_enemy(GSGLOBAL *gsGlobal, Enemy *e) {
    if (!e->active) return;
    
    gsKit_prim_sprite(gsGlobal,
        e->x, e->y,
        e->x + ENEMY_SIZE, e->y + ENEMY_SIZE,
        1, e->color);
    
    /* Eyes - two small white squares */
    gsKit_prim_sprite(gsGlobal,
        e->x + 5, e->y + 8,
        e->x + 10, e->y + 13,
        1, COLOR_WHITE);
    gsKit_prim_sprite(gsGlobal,
        e->x + ENEMY_SIZE - 10, e->y + 8,
        e->x + ENEMY_SIZE - 5, e->y + 13,
        1, COLOR_WHITE);
}

/* Draw the HUD: score, lives, level */
void render_hud(GSGLOBAL *gsGlobal, GameState *state) {
    int i;
    
    /* Score label area */
    gsKit_prim_sprite(gsGlobal, 10, 8, 70, 26, 1, COLOR_DARKGRAY);
    
    /* Score number */
    render_number(gsGlobal, state->score, 75, 8, COLOR_WHITE);
    
    /* Lives - small squares */
    for (i = 0; i < state->lives; i++) {
        gsKit_prim_sprite(gsGlobal,
            SCREEN_WIDTH - 30 - (i * 25), 10,
            SCREEN_WIDTH - 14 - (i * 25), 26,
            1, COLOR_GREEN);
    }
    
    /* Level indicator */
    render_number(gsGlobal, state->level, SCREEN_WIDTH / 2.0f - 8, 8, COLOR_CYAN);
}

/* Draw a border around the play area */
void render_border(GSGLOBAL *gsGlobal) {
    /* Top */
    gsKit_prim_sprite(gsGlobal, 0, 0, SCREEN_WIDTH, 4, 1, COLOR_DARKGRAY);
    /* Bottom */
    gsKit_prim_sprite(gsGlobal, 0, SCREEN_HEIGHT - 4, SCREEN_WIDTH, SCREEN_HEIGHT, 1, COLOR_DARKGRAY);
    /* Left */
    gsKit_prim_sprite(gsGlobal, 0, 0, 4, SCREEN_HEIGHT, 1, COLOR_DARKGRAY);
    /* Right */
    gsKit_prim_sprite(gsGlobal, SCREEN_WIDTH - 4, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1, COLOR_DARKGRAY);
}

/* Main game loop */
int main(int argc, char *argv[]) {
    GSGLOBAL *gsGlobal;
    GameState state;
    int i;
    
    /* Initialize game state */
    state.player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
    state.player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
    state.player.speed = 4.0f;
    state.player.color = COLOR_GREEN;
    state.running = 1;
    state.score = 0;
    state.lives = 3;
    state.level = 1;
    state.frame_count = 0;
    
    /* Initialize hardware */
    gsGlobal = init_gs();
    init_pad();
    
    printf("Clawbrew PS2 Homebrew Game\n");
    printf("D-pad to move, collect items, avoid enemies\n");
    printf("START to quit\n");
    
    /* Spawn first level */
    spawn_level(&state);
    
    /* Main loop */
    while (state.running) {
        state.frame_count++;
        
        /* Read input */
        read_input(&state);
        
        /* Update game logic */
        update_collectibles(&state);
        update_enemies(&state);
        
        /* Check level complete */
        if (state.collectibles_left <= 0) {
            state.level++;
            state.player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
            state.player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
            spawn_level(&state);
        }
        
        /* Clear screen */
        gsKit_clear(gsGlobal, COLOR_DARKBLUE);
        
        /* Render border */
        render_border(gsGlobal);
        
        /* Render collectibles */
        for (i = 0; i < MAX_COLLECTIBLES; i++) {
            render_collectible(gsGlobal, &state.collectibles[i]);
        }
        
        /* Render enemies */
        for (i = 0; i < MAX_ENEMIES; i++) {
            render_enemy(gsGlobal, &state.enemies[i]);
        }
        
        /* Render player */
        render_player(gsGlobal, &state.player);
        
        /* Render HUD */
        render_hud(gsGlobal, &state);
        
        /* Swap buffers */
        gsKit_queue_exec(gsGlobal);
        gsKit_finish();
        gsKit_sync_flip(gsGlobal);
    }
    
    /* Game over - show final score briefly */
    printf("Game Over! Score: %d, Level: %d\n", state.score, state.level);
    
    /* Cleanup */
    gsKit_deinit_global(gsGlobal);
    padPortClose(0, 0);
    
    return 0;
}
