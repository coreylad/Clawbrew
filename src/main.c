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

/* Screen dimensions */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 448

/* Colors (RGBA) */
#define COLOR_BLACK  GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00)
#define COLOR_WHITE  GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00)
#define COLOR_RED    GS_SETREG_RGBAQ(0xFF, 0x00, 0x00, 0x80, 0x00)
#define COLOR_GREEN  GS_SETREG_RGBAQ(0x00, 0xFF, 0x00, 0x80, 0x00)
#define COLOR_BLUE   GS_SETREG_RGBAQ(0x00, 0x00, 0xFF, 0x80, 0x00)

/* Player state */
typedef struct {
    float x;
    float y;
    float speed;
    u64 color;
} Player;

/* Game state */
typedef struct {
    Player player;
    int running;
    int score;
} GameState;

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
        if (state->player.x > SCREEN_WIDTH - 32) state->player.x = SCREEN_WIDTH - 32;
        if (state->player.y > SCREEN_HEIGHT - 32) state->player.y = SCREEN_HEIGHT - 32;
    }
}

/* Render the player */
void render_player(GSGLOBAL *gsGlobal, Player *player) {
    gsKit_prim_sprite(gsGlobal, 
        player->x, player->y,
        player->x + 32, player->y + 32,
        1, player->color);
}

/* Main game loop */
int main(int argc, char *argv[]) {
    GSGLOBAL *gsGlobal;
    GameState state;
    
    /* Initialize game state */
    state.player.x = SCREEN_WIDTH / 2.0f - 16;
    state.player.y = SCREEN_HEIGHT / 2.0f - 16;
    state.player.speed = 4.0f;
    state.player.color = COLOR_GREEN;
    state.running = 1;
    state.score = 0;
    
    /* Initialize hardware */
    gsGlobal = init_gs();
    init_pad();
    
    printf("Clawbrew PS2 Homebrew Game\n");
    printf("D-pad to move, START to quit\n");
    
    /* Main loop */
    while (state.running) {
        /* Read input */
        read_input(&state);
        
        /* Clear screen */
        gsKit_clear(gsGlobal, COLOR_BLACK);
        
        /* Render */
        render_player(gsGlobal, &state.player);
        
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
