#include <stdlib.h>
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"

#define XRES 800
#define YRES 600

void initSDL(void);
void SDL_print(char *label,int x, int y,int angle);
void SDL_display_lines(struct display_list *list,int fill,int line, int linewidth,int alpha);

struct button_item{
	int x;
	int y;
	int w;
	int h;
	int keybind;
	char * glyph;
};

struct SDL_osd_content{
	double * speed;
	int next_angle;
	char command_1 [256];
	char command_2 [256];
};

SDL_Surface* affichage;
