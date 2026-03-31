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
#define COLOR_SHIELD   GS_SETREG_RGBAQ(0x00, 0xAA, 0xFF, 0x80, 0x00)
#define COLOR_POWERUP  GS_SETREG_RGBAQ(0xFF, 0x44, 0xFF, 0x80, 0x00)
#define COLOR_FLASH    GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00)

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
    static int prev_start = 0;  /* debounce */
    
    if (padRead(0, 0, &buttons) != 0) {
        int paddata = 0xffff ^ buttons.btns;
        
        /* Title screen: any button starts game */
        if (state->game_state == STATE_TITLE) {
            if (paddata & PAD_CROSS) {
                state->game_state = STATE_PLAYING;
                state->score = 0;
                state->lives = 3;
                state->level = 1;
                state->player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
                state->player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
                state->player.speed = state->player.base_speed;
                state->player.shield_active = 0;
                state->player.speed_boost = 0;
                state->player.powerup_timer = 0;
                spawn_level(state);
            }
            return;
        }
        
        /* Game over: X to return to title */
        if (state->game_state == STATE_GAMEOVER) {
            if (paddata & PAD_CROSS) {
                state->game_state = STATE_TITLE;
            }
            return;
        }
        
        /* Pause toggle */
        if ((paddata & PAD_START) && !prev_start) {
            if (state->game_state == STATE_PLAYING) {
                state->game_state = STATE_PAUSED;
            } else if (state->game_state == STATE_PAUSED) {
                state->game_state = STATE_PLAYING;
            }
            prev_start = 1;
            return;
        }
        prev_start = (paddata & PAD_START) ? 1 : 0;
        
        /* Only process movement when playing */
        if (state->game_state != STATE_PLAYING) return;
        
        /* D-pad movement */
        if (paddata & PAD_UP)    state->player.y -= state->player.speed;
        if (paddata & PAD_DOWN)  state->player.y += state->player.speed;
        if (paddata & PAD_LEFT)  state->player.x -= state->player.speed;
        if (paddata & PAD_RIGHT) state->player.x += state->player.speed;
        
        /* Keep player on screen */
        if (state->player.x < 0) state->player.x = 0;
        if (state->player.y < 0) state->player.y = 0;
        if (state->player.x > SCREEN_WIDTH - PLAYER_SIZE) state->player.x = SCREEN_WIDTH - PLAYER_SIZE;
        if (state->player.y > SCREEN_HEIGHT - PLAYER_SIZE) state->player.y = SCREEN_HEIGHT - PLAYER_SIZE;
    }
}

/* Spawn a level with collectibles, enemies, and powerups */
void spawn_level(GameState *state) {
    int i;
    int spacing = SCREEN_WIDTH / (MAX_COLLECTIBLES + 1);
    float spd;
    
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
    
    /* Base enemy speed scales with level */
    spd = 1.5f + state->level * 0.4f;
    if (spd > 4.0f) spd = 4.0f;
    
    /* Place enemies - mix of bouncers and chasers */
    for (i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].x = 100 + (i * 120);
        state->enemies[i].y = 120 + (i * 70) % (SCREEN_HEIGHT - 200);
        state->enemies[i].active = 1;
        
        /* Chasers appear on level 2+ */
        if (i >= 4 && state->level >= 2) {
            state->enemies[i].type = ENEMY_CHASER;
            state->enemies[i].dx = spd * 0.6f;
            state->enemies[i].dy = spd * 0.6f;
            state->enemies[i].color = COLOR_PURPLE;
        } else {
            state->enemies[i].type = ENEMY_BOUNCER;
            state->enemies[i].dx = (i % 2 == 0) ? spd : 0;
            state->enemies[i].dy = (i % 2 == 1) ? spd : 0;
            state->enemies[i].color = COLOR_RED;
        }
    }
    
    /* Place powerups - 1 per level (starting level 2) */
    for (i = 0; i < MAX_POWERUPS; i++) {
        state->powerups[i].active = 0;
    }
    
    if (state->level >= 2) {
        state->powerups[0].x = 200 + (state->level * 73) % (SCREEN_WIDTH - 400);
        state->powerups[0].y = 150 + (state->level * 41) % (SCREEN_HEIGHT - 300);
        state->powerups[0].active = 1;
        state->powerups[0].type = (state->level % 2 == 0) ? POWERUP_SHIELD : POWERUP_SPEED;
        state->powerups[0].color = (state->powerups[0].type == POWERUP_SHIELD) ? COLOR_SHIELD : COLOR_POWERUP;
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
    float dx, dy, dist;
    
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].active) continue;
        
        if (state->enemies[i].type == ENEMY_CHASER) {
            /* Chaser: slowly drift toward player */
            dx = state->player.x - state->enemies[i].x;
            dy = state->player.y - state->enemies[i].y;
            dist = dx * dx + dy * dy;
            
            if (dist > 100.0f) {
                /* Normalize and move toward player */
                float speed = state->enemies[i].dx;
                state->enemies[i].x += (dx / 20.0f) * speed * 0.3f;
                state->enemies[i].y += (dy / 20.0f) * speed * 0.3f;
            }
        } else {
            /* Bouncer: standard wall-bounce */
            state->enemies[i].x += state->enemies[i].dx;
            state->enemies[i].y += state->enemies[i].dy;
            
            /* Bounce off walls */
            if (state->enemies[i].x <= 0 || state->enemies[i].x >= SCREEN_WIDTH - ENEMY_SIZE) {
                state->enemies[i].dx = -state->enemies[i].dx;
            }
            if (state->enemies[i].y <= 0 || state->enemies[i].y >= SCREEN_HEIGHT - ENEMY_SIZE) {
                state->enemies[i].dy = -state->enemies[i].dy;
            }
        }
        
        /* Clamp to screen */
        if (state->enemies[i].x < 0) state->enemies[i].x = 0;
        if (state->enemies[i].y < 0) state->enemies[i].y = 0;
        
        /* Check collision with player */
        if (check_collision(state->player.x, state->player.y, PLAYER_SIZE,
                            state->enemies[i].x, state->enemies[i].y, ENEMY_SIZE)) {
            if (state->player.shield_active) {
                /* Shield absorbs hit, enemy bounces back */
                state->enemies[i].dx = -state->enemies[i].dx;
                state->enemies[i].dy = -state->enemies[i].dy;
                state->player.shield_active = 0;
                state->player.powerup_timer = 0;
            } else {
                state->lives--;
                state->death_flash = 15; /* flash white for 15 frames */
                state->player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
                state->player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
                
                if (state->lives <= 0) {
                    state->game_state = STATE_GAMEOVER;
                    if (state->score > state->high_score) {
                        state->high_score = state->score;
                    }
                }
            }
        }
    }
    
    /* Handle death flash */
    if (state->death_flash > 0) {
        state->death_flash--;
    }
    
    /* Handle powerup timer */
    if (state->player.powerup_timer > 0) {
        state->player.powerup_timer--;
        if (state->player.powerup_timer <= 0) {
            state->player.shield_active = 0;
            state->player.speed_boost = 0;
            state->player.speed = state->player.base_speed;
            state->player.color = COLOR_GREEN;
        }
    }
}

/* Update powerup pickups */
void update_powerups(GameState *state) {
    int i;
    
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].active) continue;
        
        if (check_collision(state->player.x, state->player.y, PLAYER_SIZE,
                            state->powerups[i].x, state->powerups[i].y, POWERUP_SIZE)) {
            state->powerups[i].active = 0;
            state->player.powerup_timer = POWERUP_DURATION;
            
            if (state->powerups[i].type == POWERUP_SPEED) {
                state->player.speed_boost = 1;
                state->player.speed = state->player.base_speed * 1.5f;
                state->player.color = COLOR_POWERUP;
            } else {
                state->player.shield_active = 1;
                state->player.color = COLOR_SHIELD;
            }
        }
    }
}

/* Render a powerup pickup as a pulsing shape */
void render_powerup(GSGLOBAL *gsGlobal, Powerup *p, int frame) {
    float pulse;
    int offset;
    
    if (!p->active) return;
    
    /* Pulsing effect */
    pulse = (frame % 60 < 30) ? 0 : 2;
    offset = (int)pulse;
    
    gsKit_prim_sprite(gsGlobal,
        p->x - offset, p->y - offset,
        p->x + POWERUP_SIZE + offset, p->y + POWERUP_SIZE + offset,
        1, p->color);
    
    /* Inner glow */
    gsKit_prim_sprite(gsGlobal,
        p->x + 4, p->y + 4,
        p->x + POWERUP_SIZE - 4, p->y + POWERUP_SIZE - 4,
        1, COLOR_WHITE);
}

/* Render the title screen */
void render_title(GSGLOBAL *gsGlobal, GameState *state) {
    int i;
    int y;
    
    /* Draw title as large colored squares spelling CLAWBREW */
    /* C */
    gsKit_prim_sprite(gsGlobal, 180, 100, 220, 140, 1, COLOR_CYAN);
    gsKit_prim_sprite(gsGlobal, 180, 140, 200, 200, 1, COLOR_CYAN);
    gsKit_prim_sprite(gsGlobal, 180, 200, 220, 240, 1, COLOR_CYAN);
    /* L */
    gsKit_prim_sprite(gsGlobal, 240, 100, 260, 240, 1, COLOR_YELLOW);
    gsKit_prim_sprite(gsGlobal, 240, 220, 300, 240, 1, COLOR_YELLOW);
    /* A */
    gsKit_prim_sprite(gsGlobal, 320, 100, 340, 240, 1, COLOR_ORANGE);
    gsKit_prim_sprite(gsGlobal, 340, 100, 400, 120, 1, COLOR_ORANGE);
    gsKit_prim_sprite(gsGlobal, 340, 160, 400, 180, 1, COLOR_ORANGE);
    gsKit_prim_sprite(gsGlobal, 380, 100, 400, 240, 1, COLOR_ORANGE);
    /* W */
    gsKit_prim_sprite(gsGlobal, 420, 100, 440, 240, 1, COLOR_GREEN);
    gsKit_prim_sprite(gsGlobal, 470, 100, 490, 240, 1, COLOR_GREEN);
    gsKit_prim_sprite(gsGlobal, 420, 200, 490, 240, 1, COLOR_GREEN);
    gsKit_prim_sprite(gsGlobal, 450, 140, 470, 200, 1, COLOR_GREEN);
    
    /* Blink "PRESS X TO START" */
    state->title_blink++;
    if ((state->title_blink / 30) % 2 == 0) {
        /* Draw "PRESS X" as small squares */
        for (i = 0; i < 7; i++) {
            gsKit_prim_sprite(gsGlobal,
                250 + i * 18, 300, 250 + i * 18 + 14, 314, 1, COLOR_WHITE);
        }
    }
    
    /* High score */
    if (state->high_score > 0) {
        render_number(gsGlobal, state->high_score, 300, 350, COLOR_YELLOW);
    }
    
    /* Floating decorative enemies in background */
    for (i = 0; i < 6; i++) {
        y = 280 + ((i * 67 + state->frame_count / 2) % 100);
        gsKit_prim_sprite(gsGlobal,
            50 + i * 100, y, 50 + i * 100 + 20, y + 20, 1, COLOR_RED);
    }
}

/* Render the pause overlay */
void render_pause(GSGLOBAL *gsGlobal) {
    /* Semi-transparent overlay */
    gsKit_prim_sprite(gsGlobal, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1,
        GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x40, 0x00));
    
    /* PAUSE text */
    gsKit_prim_sprite(gsGlobal, 240, 180, 280, 220, 1, COLOR_WHITE);
    gsKit_prim_sprite(gsGlobal, 240, 180, 320, 200, 1, COLOR_WHITE);
    gsKit_prim_sprite(gsGlobal, 300, 180, 320, 220, 1, COLOR_WHITE);
    gsKit_prim_sprite(gsGlobal, 300, 200, 320, 220, 1, COLOR_WHITE);
    gsKit_prim_sprite(gsGlobal, 240, 200, 280, 220, 1, COLOR_WHITE);
    
    /* "START TO RESUME" hint */
    gsKit_prim_sprite(gsGlobal, 270, 250, 370, 260, 1, COLOR_DARKGRAY);
}

/* Render the game over screen */
void render_gameover(GSGLOBAL *gsGlobal, GameState *state) {
    int i;
    
    /* Background */
    gsKit_clear(gsGlobal, COLOR_BLACK);
    
    /* GAME OVER text */
    for (i = 0; i < 8; i++) {
        gsKit_prim_sprite(gsGlobal,
            180 + i * 32, 140, 180 + i * 32 + 28, 180, 1, COLOR_RED);
    }
    
    /* Final score */
    render_number(gsGlobal, state->score, 280, 220, COLOR_WHITE);
    
    /* High score */
    render_number(gsGlobal, state->high_score, 280, 270, COLOR_YELLOW);
    
    /* Blink "PRESS X" */
    state->title_blink++;
    if ((state->title_blink / 30) % 2 == 0) {
        for (i = 0; i < 7; i++) {
            gsKit_prim_sprite(gsGlobal,
                250 + i * 18, 330, 250 + i * 18 + 14, 344, 1, COLOR_WHITE);
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
    memset(&state, 0, sizeof(GameState));
    state.player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
    state.player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
    state.player.base_speed = 4.0f;
    state.player.speed = 4.0f;
    state.player.color = COLOR_GREEN;
    state.running = 1;
    state.game_state = STATE_TITLE;
    state.score = 0;
    state.high_score = 0;
    state.lives = 3;
    state.level = 1;
    state.frame_count = 0;
    state.title_blink = 0;
    state.death_flash = 0;
    
    /* Initialize hardware */
    gsGlobal = init_gs();
    init_pad();
    
    printf("Clawbrew v0.3 - PS2 Homebrew Game\n");
    printf("X to start, D-pad to move, START to pause\n");
    
    /* Main loop */
    while (state.running) {
        state.frame_count++;
        
        /* Read input (handles state transitions) */
        read_input(&state);
        
        /* Clear screen */
        if (state.game_state == STATE_TITLE) {
            gsKit_clear(gsGlobal, COLOR_BLACK);
            render_title(gsGlobal, &state);
        } else if (state.game_state == STATE_GAMEOVER) {
            render_gameover(gsGlobal, &state);
        } else {
            /* Playing or paused - render the game world */
            
            if (state.game_state == STATE_PLAYING) {
                /* Update game logic */
                update_collectibles(&state);
                update_enemies(&state);
                update_powerups(&state);
                
                /* Check level complete */
                if (state.collectibles_left <= 0) {
                    state.level++;
                    state.player.x = SCREEN_WIDTH / 2.0f - PLAYER_SIZE / 2;
                    state.player.y = SCREEN_HEIGHT / 2.0f - PLAYER_SIZE / 2;
                    state.player.speed = state.player.base_speed;
                    state.player.shield_active = 0;
                    state.player.speed_boost = 0;
                    state.player.powerup_timer = 0;
                    state.player.color = COLOR_GREEN;
                    spawn_level(&state);
                }
            }
            
            /* Death flash overlay */
            if (state.death_flash > 0) {
                gsKit_clear(gsGlobal, COLOR_FLASH);
            } else {
                gsKit_clear(gsGlobal, COLOR_DARKBLUE);
            }
            
            /* Render border */
            render_border(gsGlobal);
            
            /* Render collectibles */
            for (i = 0; i < MAX_COLLECTIBLES; i++) {
                render_collectible(gsGlobal, &state.collectibles[i]);
            }
            
            /* Render powerups */
            for (i = 0; i < MAX_POWERUPS; i++) {
                render_powerup(gsGlobal, &state.powerups[i], state.frame_count);
            }
            
            /* Render enemies */
            for (i = 0; i < MAX_ENEMIES; i++) {
                render_enemy(gsGlobal, &state.enemies[i]);
            }
            
            /* Render player (with powerup visual) */
            render_player(gsGlobal, &state.player);
            
            /* Shield ring around player */
            if (state.player.shield_active) {
                gsKit_prim_sprite(gsGlobal,
                    state.player.x - 4, state.player.y - 4,
                    state.player.x + PLAYER_SIZE + 4, state.player.y + PLAYER_SIZE + 4,
                    1, COLOR_SHIELD);
            }
            
            /* Render HUD */
            render_hud(gsGlobal, &state);
            
            /* Powerup timer bar */
            if (state.player.powerup_timer > 0) {
                float bar_width = (state.player.powerup_timer / (float)POWERUP_DURATION) * 80.0f;
                gsKit_prim_sprite(gsGlobal,
                    SCREEN_WIDTH / 2.0f - 40, 32,
                    SCREEN_WIDTH / 2.0f - 40 + (int)bar_width, 38,
                    1, state.player.color);
            }
            
            /* Pause overlay */
            if (state.game_state == STATE_PAUSED) {
                render_pause(gsGlobal);
            }
        }
        
        /* Swap buffers */
        gsKit_queue_exec(gsGlobal);
        gsKit_finish();
        gsKit_sync_flip(gsGlobal);
    }
    
    /* Cleanup */
    gsKit_deinit_global(gsGlobal);
    padPortClose(0, 0);
    
    return 0;
}
