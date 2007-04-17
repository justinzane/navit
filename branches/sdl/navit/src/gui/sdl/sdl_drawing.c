#include sdl_drawing.h

void gl_cursor(int x,int y){
	glLoadIdentity();
	glEnable( GL_BLEND );

	float cursor_size=15.0f;
	glColor4f(0.0f,1.0f,0.0f,0.5f);

	glBegin(GL_TRIANGLES);
		glVertex3f( x, y-cursor_size, 0.0f);
		glVertex3f(x-cursor_size,y+cursor_size, 0.0f);
		glVertex3f( x+cursor_size,y+cursor_size, 0.0f);	
	glEnd();
	glDisable(GL_BLEND);
}

void draw_arrow(int angle){

	int x=720;
	int y=150;
	glLineWidth(20);
	glLoadIdentity();
// 	glEnable( GL_BLEND );
	float cursor_size=15.0f;
	glColor4f(0.0f,1.0f,0.0f,1.0f);

	glBegin(GL_LINES);
		glVertex2i( x, y+50 );
		glVertex2i( x, y);
	glEnd();
	glBegin(GL_LINES);
		glVertex2i( x+50*(sin(angle)), y-50*(cos(angle)) );
		glVertex2i( x, y);
	glEnd();
// 	glDisable(GL_BLEND);
}

