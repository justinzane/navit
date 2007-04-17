#include <SDL/SDL_ttf.h>"
TTF_Font* font;


// 	TTF_Init();

// 	LoadFont("/usr/share/fonts/corefonts/arial.ttf", 10, 255,255,255);


int LoadFont(char * police, int size, int R, int G, int B)
{
	Tcolor.r=R;
	Tcolor.g=G;
	Tcolor.b=B;
 
	font = TTF_OpenFont(police, size);
    if(font==NULL) {
	printf("TTF_OpenFont: %s\n", TTF_GetError());
        return 0;
    }
	rectsrc.x = 0;
	rectsrc.y = 0;
	rectsrc.w = 100;
	rectsrc.h = 70;
 
    return 1;
}

int DrawText(char * message,float w,float h)
{
	if(message==NULL){
		return 0;
	}
    if(message != old_message) {
        if(old_message != "") {
            glDeleteTextures(1,&surface);
        }
 
        old_message = message;
 
        if(message != "") {
            //Création de la texture du sprite
            SDL_Surface *temp;

            glGenTextures (1, & surface);
            glBindTexture (GL_TEXTURE_2D, surface);
 
            // Paramétrage de la texture.
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 
            temp = TTF_RenderText_Blended(font, message, Tcolor);
 
            if(temp != NULL) {
                //Calcul de taille
                int w = puissance2sup(temp->w);
                int h = puissance2sup(temp->h);
 
                SDL_Surface *temp2=SDL_CreateRGBSurface(
                    SDL_HWSURFACE,
                    w,
                    h,
                    32,
                    temp->format->Rmask,
                    temp->format->Gmask,
                    temp->format->Bmask,
                    temp->format->Amask);
 
                if(temp2 == NULL) {
                    SDL_FreeSurface(temp);
                    return false;
                }
                maxx = temp->w;
                maxx /= w;
                maxy = temp->h;
                maxy /= h;
 
		rectsrc.x = 0;
		rectsrc.y = 0;
		rectsrc.w = temp2->w;
		rectsrc.h = temp2->h;
		
		//Fill the new image with your colorkey color
 		SDL_FillRect(temp2, &rectsrc, SDL_MapRGB(temp2->format, 255, 255, 255));

                SDL_BlitSurface(temp, NULL, temp2, &rectsrc);
 
                // Jonction entre OpenGL et SDL.
                glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, temp2->w, temp2->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp2->pixels);
 
                SDL_FreeSurface(temp);
                SDL_FreeSurface(temp2);
            }
            else {
			printf("woops for %s\n",message);

                return false;
            }
        }
    }
 
//   	glEnable(GL_ALPHA_TEST);
// 
//   	glBlendFunc(GL_ONE,GL_ONE);
//   	glEnable(GL_BLEND);
//  	glAlphaFunc(GL_LESS,0.9);

     glEnable(GL_TEXTURE_2D);

//     glBegin(GL_QUADS);
//         glTexCoord2f (0, 0);
// 		glVertex2f(rectdest.x, rectdest.y);
//  
//         glTexCoord2f (0, maxy);
// 		glVertex2f(rectdest.x, rectdest.y + rectsrc.h);
//  
//         glTexCoord2f (maxx, maxy);
// 		glVertex2f(rectdest.x + rectsrc.w, rectdest.y + rectsrc.h);
//  
//         glTexCoord2f (maxx, 0);
// 		glVertex2f(rectdest.x + rectsrc.w, rectdest.y);
//     glEnd();


}


SDL_Surface *CreateText(char * text, TTF_Font *font, SDL_Color textColor)
{
    //Make text as normal...
	printf("drawing %s\n",text);
    SDL_Surface* TextSurface = TTF_RenderText_Solid(font, text, textColor);

	if(!TextSurface) {
		return TextSurface;
 	}
    //Checks whether TextSurface is a power of two or not
    int WidthRemainder = (TextSurface->w % 2);
    int HeightRemainder = (TextSurface->h % 2);

    //If it is, return the surface
    if(WidthRemainder == 0 && HeightRemainder == 0) return TextSurface;

    int WidthPlus = 0;
    int HeightPlus = 0;

    if(WidthRemainder != 0) WidthPlus = 1;
    if(HeightRemainder != 0) HeightPlus = 1;

    //Else, Create new surface
    SDL_Surface *NewSurface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCCOLORKEY, (TextSurface->w + WidthPlus), (TextSurface->h + HeightPlus), 32, 
                                  0xFF000000, 0x00FF0000, 0x0000FF00, 0);

    //Create a rectangle for coloring the empty image
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = NewSurface->w;
    rect.h = NewSurface->h;

    //Fill the new image with your colorkey color
    SDL_FillRect(NewSurface, &rect, SDL_MapRGB(NewSurface->format, 255, 255, 255));

    //Place your text onto the new surface
    SDL_BlitSurface(TextSurface, NULL, NewSurface, &rect);

    //Free the old surface
    SDL_FreeSurface(TextSurface);

    //Set the colorkey on the empty surface to make the COLORKEY_ color clear
    Uint32 colorkey = SDL_MapRGB(NewSurface->format, 255, 255, 255);
    SDL_SetColorKey(NewSurface, SDL_RLEACCEL|SDL_SRCCOLORKEY, colorkey);

    //Return your new surface, with dimensions of a power of two.
    return NewSurface;
}


