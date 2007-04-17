#include <stdlib.h>
#include <stdio.h>

#include "SDL/SDL.h"
#include "SDL/SDL_gfxPrimitives.h"
#include "SDL/SDL_opengl.h"

#include "sdl.h"
#include "gl_fonts.c"

#include "street_name.h"
#include "street.h"
#include "town.h"
#include "block.h"

#include "coord.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

#define PI 3.14159265
#define false 0
#define true 1



struct button_item button_list[]={
	{10,560,32,32,SDLK_PAGEUP,"./gfx/zoomin.png"},
	{50,560,32,32,SDLK_PAGEDOWN,"./gfx/zoomout.png"},
	{670,8,128,16,SDLK_PAGEDOWN,"./gfx/navitw.png"},
	{670,560,32,32,SDLK_HOME,"./gfx/home.png"},
	{705,560,32,32,SDLK_END,"./gfx/home.png"},
};

struct SDL_osd_content GL_osd;

struct search_param {
	struct map_data *map_data;
	char *country;
	GHashTable *country_hash;
	char *town;
	GHashTable *town_hash;
	GHashTable *district_hash;
	char *street;
	GHashTable *street_hash;
	const char *number;
	int number_low, number_high;
	GtkWidget *clist;
	int count;
} search_param2;

struct destination {
	struct town *town;
	struct street_name *street_name;
	struct coord *c;
};

struct search_param *search=&search_param2;

int buttons=5;
int inputting=0;

GLuint textur;

char input_string[256];

int init_GL() {
	 //Set clear color. This one is suitable for day use. We can add a night mode later
 	glClearColor(1.0,0.9,0.7,0);
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, XRES, YRES, 0, -1, 1 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	if( glGetError() != GL_NO_ERROR ) {
		return 0;
	}
	return 1;
}


void initSDL(void){
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Erreur à l'initialisation de la SDL : %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	atexit(SDL_Quit);
	affichage = SDL_SetVideoMode(XRES, YRES, 32, SDL_OPENGL);

	if (affichage == NULL) {
		fprintf(stderr, "Impossible d'activer le mode graphique : %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
 
	SDL_WM_SetCaption("SDL_NavIt", NULL);

	if( init_GL() == 0 ) { 
		printf("OpenGL NOT OK\n");
	}
	
	buildFont( );

}

void Draw_buttons(){

	int button;
	for(button=0;button<buttons;button++){
		Load_texture(button_list[button].glyph );
		
		glLoadIdentity();
	
		glBegin(GL_QUADS);
			glTexCoord2i(0,0);glVertex2i(button_list[button].x,button_list[button].y);
			glTexCoord2i(1,0);glVertex2i(button_list[button].x+button_list[button].w,button_list[button].y);
			glTexCoord2i(1,1);glVertex2i(button_list[button].x+button_list[button].w,button_list[button].y+button_list[button].h);
			glTexCoord2i(0,1);glVertex2i(button_list[button].x,button_list[button].y+button_list[button].h);
		glEnd();
	}
   	glDisable (GL_TEXTURE_2D);
}

void SDL_Draw_Image(char picto[256],int x,int y,int w,int h){

	Load_texture(picto);

	glLoadIdentity();
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);glVertex2i(x,y);
		glTexCoord2i(1,0);glVertex2i(x+w,y);
		glTexCoord2i(1,1);glVertex2i(x+w,y+h);
		glTexCoord2i(0,1);glVertex2i(x,y+h);
	glEnd();

   	glDisable (GL_TEXTURE_2D);
}

void sdl_update(struct vehicle *v, void *t){
// 	struct cursor *this=t;
	struct point pnt;
	struct coord *pos;
	double *dir;
	extern struct container *co;

	if (v) {
	
		pos=vehicle_pos_get(v);	
		dir=vehicle_dir_get(v);
// 		track_update(this->co->track, pos, (int)(*dir));
		if (co->flags->orient_north)
			dir=0;
// 		route_set_position(this->co->route, cursor_pos_get(this->co->cursor));
		if (!transform(co->trans, pos, &pnt)) {
//			cursor_map_reposition(this, pos, dir);
 			transform(co->trans, pos, &pnt);
		}
		if (pnt.x < 0 || pnt.y < 0 || pnt.x >= co->trans->width || pnt.y >= co->trans->height || co->flags->fixedpos_mode) {
//			cursor_map_reposition(this, pos, dir);
 			transform(co->trans, pos, &pnt);
		}
// 		if (cursor_map_reposition_boundary(this, pos, dir, &pnt)){
// 			transform(this->co->trans, pos, &pnt);
// 		}
//  		cursor_draw(this, &pnt, vehicle_speed_get(v), vehicle_dir_get(v));
	}
// 	compass_draw(this->co->compass, this->co);



	if(v){
// 		display_free(co->disp, display_end);

		Empty_Scene();
// 		Start_3D_View();
//  		Full_Redraw(co);
// 		End_3D_View();

		graphics_redraw(co);

		SDL_osd(vehicle_speed_get(v));
		handleEvents();
		Display_Scene();
	}
}

void sdl_gui_new(struct vehicle *v){ //struct container *co
	vehicle_callback_register(v, sdl_update,0);
}

void Empty_Scene(){
	glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
}

void Load_texture(char picto[256]){
	glEnable (GL_TEXTURE_2D);

	glColor4f( (float)(255)/255, (float)(239)/255, (float)(183)/255, 1.0);

	GLuint texture_name;
	SDL_Surface * button_texture;

	glGenTextures (1, &texture_name);
	glBindTexture (GL_TEXTURE_2D, button_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	if(button_texture = IMG_Load (picto )){
// 		printf("Loaded %s\n",picto);
	} else {
		printf("Cannot load texture %s : \n",picto);
	}
	
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, button_texture->w, button_texture->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, button_texture-> pixels);
	SDL_FreeSurface(button_texture);

}


void draw_destination_dialog(){
	
	SDL_Draw_Image("./gfx/zoomin.png",150,10,100,200);
	char tmp_string[256];
  	sprintf(tmp_string,"Input : %s ",input_string);

	glColor4f( 0.2,0.2,0.2, 0.8);

	glBegin( GL_QUADS );
		glVertex3f( 100, 140, 0 );
		glVertex3f( 550, 140, 0 );
		glVertex3f( 550, 500, 0 );
		glVertex3f( 100, 500,0 );
	glEnd();

	glColor4f( 1.0,1.0,1.0, 1.0);
	glRasterPos2f( 120,170 );
	glPrintBig(tmp_string);
	
}


static int puissance2sup(int i)
{
    double logbase2 = log(i) / log(2);
    return round(pow(2.0, ceil(logbase2)));
}
  
 

 
// void SetPosition(int x, int y)
// {
// 	rectdest.x = x;
// 	rectdest.y = y;
// }
 
void SDL_osd(double speed){

	glEnable( GL_LINE_SMOOTH );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	glColor4f( 0.2,0.2,0.2, 0.8);

	//Right Hand Shadow
	glBegin( GL_QUADS );
		glVertex3f( 650, 0, 0 );
		glVertex3f( 800, 0, 0 );
		glVertex3f( 800, 600, 0 );
		glVertex3f( 650, 600,0 );
	glEnd();

	//Bottom Shadow ( Nav instructions)
	glBegin( GL_QUADS );
		glVertex3f( 100, 550, 0 );
		glVertex3f( 650, 550, 0 );
		glVertex3f( 650, 600, 0 );
		glVertex3f( 100, 600,0 );
	glEnd();

	//Bottom Shadow ( Pos. Info)
	glBegin( GL_QUADS );
		glVertex3f( 100, 525, 0 );
		glVertex3f( 650, 525, 0 );
		glVertex3f( 650, 548, 0 );
		glVertex3f( 100, 548,0 );
	glEnd();

 	glDisable( GL_BLEND );

	char osd_data[256];
	sprintf(osd_data,"%.lf",speed);

	glColor3f( 1.0f,1.0f,1.0f);
	glRasterPos2f( 660,50 );
	glPrintBig( "Speed:" );
	glRasterPos2f( 660,80 );
	glPrintBig( osd_data );
	glRasterPos2f( 120,580 );
	glPrintBig( GL_osd.command_1 );


	extern struct container *co;
	int i=0;

	if (co->cursor) {
		struct coord *c;
		struct route_info *rt;
		struct street_str *st;
		struct block_info *blk;
		struct street_name name;
		struct town town;
		c=cursor_pos_get(co->cursor);
		rt=route_find_nearest_street(co->map_data, c);
		st=route_info_get_street(rt);	
		blk=route_info_get_block(rt);
		printf("segid 0x%lx nameid 0x%lx\n", st->segid, st->nameid);
		if(street_name_get_by_id(&name, blk->mdata, st->nameid)){
 			sprintf(osd_data,"(%s %s)",name.name1,name.name2);
		} else {
			sprintf(osd_data,"<?>");
		}
		glRasterPos2f( 125,545 );
		glPrintBig( osd_data );

//  		printf("'%s' '%s' %d\n", name.name1, name.name2, name.segment_count);
 		for (i = 0 ; i < name.segment_count ; i++) {
			if (name.segments[i].segid == st->segid) {
// 				printf("found: 0x%x, 0x%x\n", name.segments[i].country, name.segments[i].segid);
//  				town_get_by_id(&town, co->map_data, name.segments[i].country, name.townassoc);
//  				printf("%s\n", town.name);	
			}
		}
	}



//  	draw_arrow(GL_osd.next_angle);

	glEnable(GL_BLEND);
	Draw_buttons();
	if(inputting){
		draw_destination_dialog();
	}
}

void Start_3D_View(){
	glLoadIdentity();
}




void End_3D_View(){
// 	glEnable(GL_TEXTURE_2D);

if (textur == 0)
  {
   glGenTextures(1, &textur);
   glBindTexture(GL_TEXTURE_2D, textur);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   // Copies the contents of the frame buffer into the texture
   glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 512, 512, 0);
  } else {
   // Copies the contents of the frame buffer into the texture
    glBindTexture(GL_TEXTURE_2D, textur);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 512, 512);
  }

 	glViewport( 0, 0, 800, 600);
	glLoadIdentity();

/*
 	glColor4f( (float)(255)/255, (float)(239)/255, (float)(183)/255, 1);


	int quad_size=512;
	int prof=512;
	glBegin( GL_QUADS );
		glTexCoord2i( 0, 1 );glVertex3i( 1, 1, 0 );
		glTexCoord2i( 0, 0 );glVertex3i( 1-prof, quad_size, 0 );
		glTexCoord2i( 1, 0 );glVertex3i( quad_size+prof, quad_size, 0 );
		glTexCoord2i( 1, 1 );glVertex3i( quad_size, 1, 0 );
	glEnd();
	glDisable(GL_TEXTURE_2D);
*/
}


void Display_Scene(){
	
	SDL_GL_SwapBuffers();
// 	glFlush();
}


void GLDrawLineRGBA( int x1, int y1, int x2, int y2, int fill, int alpha, int h) { 
	GLDrawLineRGBA_labelled(x1,y1,x2,y2,fill,alpha,h,"");
}

void GLDrawLineRGBA_labelled( int x1, int y1, int x2, int y2, int fill, int alpha, int linewidth,char * label) { 

//  	glEnable( GL_POLYGON_SMOOTH );

	float dx=x2-x1;
	float dy=y2-y1;

	float cx=(x2+x1)/2;
	float cy=(y2+y1)/2;


	int w=round(sqrt(pow((dx),2)+pow((dy),2)));

	float angle=atan (dy/dx) * 180 / PI;

// 	h=8;

	glPushMatrix();
	glTranslatef(cx,cy,0);
 	glColor4f( 0,0,0,1);
 	glRasterPos2f( 1,1 );
//  	glPrint(label);
	glRotatef(angle,0.0,0.0,1.0);

	extern int color[][3];
	float r=(float)(color[fill][0])/65535;
	float g=(float)(color[fill][1])/65535;
	float b=(float)(color[fill][2])/65535;
	glColor4f( r, g, b, 1);

// 	DrawText(label,w,h);

//   	printf("Drawing %s\n",label);
//  	printf ("Seg (%i,%i) -> (%i,%i) len : %i, angle = %.2f drawn at (%.2f,%.2f)\n",x1,y1,x2,y2,w,angle,cx+w/2,cy+h/2);


//   	Load_texture("./gfx/zoomin.png");
//  	glLoadIdentity();
	glBegin( GL_POLYGON );
			glVertex2f( -w/2,-linewidth/2 );
			glVertex2f( -w/2-4,0 );
			glVertex2f( -w/2,+linewidth/2 );
			glVertex2f( +w/2,+linewidth/2 );
			glVertex2f( +w/2+4,0 );
			glVertex2f( +w/2,-linewidth/2 );
			glVertex2f( -w/2,+linewidth/2 );
	glEnd();


	glPopMatrix();

}

/* function to reset our viewport after a window resize */
int resizeWindow( int width, int height )
{
    /* Height / width ration */
    GLfloat ratio;

    /* Protect against a divide by zero */
    if ( height == 0 )
	height = 1;

    ratio = ( GLfloat )width / ( GLfloat )height;

    /* Setup our viewport. */
    glViewport( 0, 0, ( GLint )width, ( GLint )height );

    /* change to the projection matrix and set our viewing volume. */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    /* Set our perspective */
    gluPerspective( 45.0f, ratio, 0.1f, 100.0f );

    /* Make sure we're chaning the model view and not the projection */
    glMatrixMode( GL_MODELVIEW );

    /* Reset The View */
    glLoadIdentity( );

    return( TRUE );
}

void destination_search(struct search_param *search){
	extern struct container *co;
	printf("Searching for town %s\n",input_string);
// 	town_search_by_name(search->map_data, cou->id, search->town, 1, destination_town_add, search);

	if (strlen(input_string) > 1) {
		if (search->town) g_free(search->town);
		search->map_data=co->map_data;
// 		search->town=input_string;
// 		search->town_hash=destinatclion_town_new();
// 		search->district_hash=destination_town_new();
// 		g_hash_table_foreach(search->country_hash, destination_town_search, search);
// 		g_hash_table_foreach(search->town_hash, destination_town_show, search);
	}

}

/* function to handle key press events */
void handleEvents(){
	extern struct container *co;
	unsigned long scale;
	graphics_get_view(co, NULL, NULL, &scale);


	struct coord *pos;
	struct coord *pos2;
	double lat;
	double lng;

	SDL_Event event;
	
	while (SDL_PollEvent(&event) != 0){
		switch (event.type) {
			case SDL_MOUSEBUTTONDOWN:
				printf("MouseClic (%d,%d) \n",event.button.x,event.button.y);
				int i;
				for(i=0;i<buttons;i++){
					if(
						(event.button.x>button_list[i].x) &&
						(event.button.x<button_list[i].x+button_list[i].w) &&
						(event.button.y>button_list[i].y) &&
						(event.button.y<button_list[i].y+button_list[i].h)
					) {
				switch (button_list[i].keybind){
					case SDLK_F1:
						SDL_WM_ToggleFullScreen( affichage );
						break;
					case SDLK_PAGEDOWN:
						scale*=2;
					 	graphics_set_view(co, NULL, NULL, &scale);
						break;
					case SDLK_PAGEUP:
						if(scale>1){
							scale/=2;
							graphics_set_view(co, NULL, NULL, &scale);
						}
						break;
					case SDLK_HOME:
						lat=50.3314;
						lng=2.98361;
						transform_mercator(&lng, &lat, &pos);
						pos2=cursor_pos_get(co->cursor);
						route_set_position(co->route, pos2);
						route_set_destination(co->route, &pos);
						break;
					case SDLK_END:
						lat=50.56655;
						lng=3.03422;
						transform_mercator(&lng, &lat, &pos);
						pos2=cursor_pos_get(co->cursor);
						route_set_position(co->route, pos2);
						route_set_destination(co->route, &pos);
						break;
				}
					}
				}
				break;
			case SDL_KEYDOWN:
				if(inputting){
					switch (event.key.keysym.sym){
						case SDLK_ESCAPE:
						strcpy(input_string,"");
						inputting=0;
						break;
						case SDLK_BACKSPACE:
						input_string[strlen(input_string)-1]=0;
						break;
						case SDLK_SPACE:
						strcat(input_string," ");
						break;
						case SDLK_KP_ENTER:
						inputting=0;
						break;
						default:
						printf("appending %s to %s\n",SDL_GetKeyName(event.key.keysym.sym),input_string);
						strcat(input_string,SDL_GetKeyName(event.key.keysym.sym));
						destination_search(search);
					}
				} else {

					switch (event.key.keysym.sym){
						case SDLK_KP_ENTER:
							inputting=1;
						break;
						case SDLK_ESCAPE:
							Quit(0);
							break;
						case SDLK_F1:
							SDL_WM_ToggleFullScreen( affichage );
							break;
						case SDLK_PAGEDOWN:
							scale*=2;
							graphics_set_view(co, NULL, NULL, &scale);
							break;
						case SDLK_PAGEUP:
							if(scale>1){
								scale/=2;
								graphics_set_view(co, NULL, NULL, &scale);
							}
						break;
							break;
						default:
							printf("Unknown key %s!\n",SDL_GetKeyName(event.key.keysym.sym));
					}
				}
			break;
 		}	
	}
}
