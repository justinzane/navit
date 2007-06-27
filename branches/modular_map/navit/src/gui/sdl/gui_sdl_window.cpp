#include "glib.h"
#include <stdio.h>
// #include <gtk/gtk.h>

//  FIXME temporary fix for enum
#include "projection.h"


#include "navit.h"
#include "gui.h"
#include "coord.h"
#include "plugin.h"
#include "graphics.h"
#include "gui_sdl.h"


#include "CEGUI.h"

#include <CEGUI/RendererModules/OpenGLGUIRenderer/openglrenderer.h>
#include "CEGUIDefaultResourceProvider.h"

// This is for 3d fonts
#include "GL/glc.h"


#define VM_2D 0
#define VM_3D 1

bool VIEW_MODE=VM_3D;

CEGUI::OpenGLRenderer* renderer;

CEGUI::Window* myRoot;


static int
gui_sdl_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	/*
	GtkWidget *graphics;

	graphics=graphics_get_data(gra, "gtk_widget");
	if (! graphics)
		return 1;
	gtk_box_pack_end(GTK_BOX(this_->vbox), graphics, TRUE, TRUE, 0);
	gtk_widget_show_all(graphics);
*/
	return 0;
}

static int gui_run_main_loop(struct gui_priv *this_)
{
	printf("Entering main loop\n");

	bool must_quit = false;
	
	// get "run-time" in seconds
	double last_time_pulse = static_cast<double>(SDL_GetTicks());

	int frames=0;
	char fps [12];
	

	while (!must_quit)
	{
		printf("loop");

// 		profile_timer(NULL);
		glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
/*


 		glMatrixMode(GL_MODELVIEW);
 		glLoadIdentity();

		if(VIEW_MODE==VM_3D){
 			gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
		}
  		
// 		printf("eyeX=%f, eyeY=%f, eyeZ=%f, centerX=%f, centerY=%f, centerZ=%f, upX=%f, upY=%f, upZ=%f\n",eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

		glColor4f(0.0f,0.7f,0.35f,1.0f);
		glBegin(GL_POLYGON);
			glVertex3f( -800,-600*3, 0.0f);
			glVertex3f( -800,600*2, 0.0f);
			glVertex3f( 1600,600*2, 0.0f);	
			glVertex3f( 1600,-600*3, 0.0f);	
		glEnd();

		extern struct container *co;
		profile_timer("3d view");

		graphics_redraw(co);
		profile_timer("graphics_redraw");

 		g_main_context_iteration (NULL, TRUE);
		profile_timer("main context");

		inject_input(must_quit);
		profile_timer("inputs");

		// Render the cursor.
		int x=400;
		int y=480;
		float cursor_size=15.0f;
		glColor4f(0.0f,1.0f,0.0f,0.75f);
		glEnable(GL_BLEND);
		glBegin(GL_TRIANGLES);
			glVertex3f( x, y-cursor_size, 0.0f);
			glVertex3f(x-cursor_size,y+cursor_size, 0.0f);
			glVertex3f( x+cursor_size,y+cursor_size, 0.0f);	
		glEnd();
		glDisable(GL_BLEND);
		profile_timer("cursor");

		frames++;
		if(SDL_GetTicks()-last_time_pulse>1000){
			sprintf(fps,"%i",frames); // /(SDL_GetTicks()/1000));
			frames=0;
			last_time_pulse = SDL_GetTicks();
		}


		glcRenderStyle(GLC_TEXTURE);
		glColor3f(1, 0, 0);
		glRotatef(180,1,0,0);
		glScalef(64, 64, 0);
		glcRenderString(fps);
		profile_timer("fps");
*/
 		CEGUI::System::getSingleton().renderGUI();
// 		profile_timer("GUI");

		SDL_GL_SwapBuffers();
	}


}


static struct menu_priv *
gui_sdl_toolbar_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/ToolBar", nav, 0);
}

static struct statusbar_priv *
gui_sdl_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/ToolBar", nav, 0);
}

static struct menu_priv *
gui_sdl_menubar_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/MenuBar", nav, 0);
}

static struct menu_priv *
gui_sdl_popup_new(struct gui_priv *this_, struct menu_methods *meth)
{
	return NULL; //gui_gtk_ui_new(this_, meth, "/ui/PopUp", nav, 1);
}

struct gui_methods gui_sdl_methods = {
	gui_sdl_menubar_new,
	gui_sdl_toolbar_new,
	gui_sdl_statusbar_new,
	gui_sdl_popup_new,
	gui_sdl_set_graphics,
	gui_run_main_loop,
};


static void init_sdlgui(void)
{
	SDL_Surface * screen;
// 	atexit (SDL_Quit);
	SDL_Init (SDL_INIT_VIDEO);
	int videoFlags;
	const SDL_VideoInfo *videoInfo;
	videoInfo = SDL_GetVideoInfo( );

	if ( !videoInfo )
	{
	    fprintf( stderr, "Video query failed: %s\n",
		     SDL_GetError( ) );
	}

	/* the flags to pass to SDL_SetVideoMode */
	videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
	videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
	videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
	videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */
	
	/* This checks to see if surfaces can be stored in memory */
	if ( videoInfo->hw_available )
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;
	
	/* This checks if hardware blits can be done */
	if ( videoInfo->blit_hw )
		videoFlags |= SDL_HWACCEL;
	
	/* Sets up OpenGL double buffering */
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_WM_SetCaption("NavIt - The OpenSource vector based navigation engine", NULL);

	/* get a SDL surface */
	screen = SDL_SetVideoMode( XRES, YRES, 32,
					videoFlags );

	if (screen == NULL) {
		fprintf (stderr, "Can't set SDL: %s\n", SDL_GetError ());
		exit (1);
	}

	SDL_ShowCursor (SDL_ENABLE);
	SDL_EnableUNICODE (1);
	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	// GL Setup
//  	glClearColor(1.0,0.9,0.7,0);

	// Blue sky
 	glClearColor(0.3,0.7,1.0,0);

	if(VIEW_MODE==VM_2D){
		printf("Switching to 2D view\n");
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
	
		glOrtho( 0, XRES, YRES, 0, -1, 1 );
	
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	} else {
		printf("Switching to 3D view\n");
		glViewport(0, 0, XRES, YRES);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, 1.0, 0.1, 2800.0);
		// Rendu avec lissage de Gouraud 
// 		glShadeModel(GL_SMOOTH);
	// 	glEnable(GL_DEPTH_TEST);


 		glMatrixMode(GL_MODELVIEW);
   		glLoadIdentity();
	}

// 	if( glGetError() != GL_NO_ERROR ) {
// 		return 0;
// 	}

	
// 	sdl_audio_init();

	try
	{
		renderer = new CEGUI::OpenGLRenderer(0,XRES,YRES);
		new CEGUI::System(renderer);

		using namespace CEGUI;

		SDL_ShowCursor(SDL_ENABLE);
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		
		CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>
		(CEGUI::System::getSingleton().getResourceProvider());
		
		rp->setResourceGroupDirectory("schemes", "../datafiles/schemes/");
		rp->setResourceGroupDirectory("imagesets", "../datafiles/imagesets/");
		rp->setResourceGroupDirectory("fonts", "../datafiles/fonts/");
		rp->setResourceGroupDirectory("layouts", "../datafiles/layouts/");
		rp->setResourceGroupDirectory("looknfeels", "../datafiles/looknfeel/");
		rp->setResourceGroupDirectory("lua_scripts", "../datafiles/lua_scripts/");


		CEGUI::Imageset::setDefaultResourceGroup("imagesets");
		CEGUI::Font::setDefaultResourceGroup("fonts");
		CEGUI::Scheme::setDefaultResourceGroup("schemes");
		CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
		CEGUI::WindowManager::setDefaultResourceGroup("layouts");
		CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");

		CEGUI::SchemeManager::getSingleton().loadScheme("TaharezLook.scheme");

		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-10.font");
		CEGUI::FontManager::getSingleton().createFont("DejaVuSans-14.font");

		CEGUI::System::getSingleton().setDefaultFont("DejaVuSans-10");

		CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

 		myRoot = CEGUI::WindowManager::getSingleton().loadWindowLayout("navit.layout");

 		CEGUI::System::getSingleton().setGUISheet(myRoot);

	/*
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox")->subscribeEvent(Window::EventKeyUp, Event::Subscriber(DestinationEntryChange));
 		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox")->subscribeEvent(Window::EventMouseButtonDown, Event::Subscriber(handleMouseEnters));

		myRoot->getChild("OSD/DestinationButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(DialogWindowSwitch));

		myRoot->getChild("OSD/RoadbookButton")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(RoadBookSwitch));
		myRoot->getChild("Navit/RoadBook")->getChild("OSD/RoadbookButton2")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(RoadBookSwitch));
		myRoot->getChild("OSD/ZoomIn")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ZoomIn));
		myRoot->getChild("OSD/ZoomOut")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ZoomOut));
		myRoot->getChild("OSD/Quit")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonQuit));
		myRoot->getChild("OSD/ViewMode")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ToggleView));

		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/GO")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ButtonGo));
		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/KB")->subscribeEvent(PushButton::EventClicked, Event::Subscriber(ShowKeyboard));

		myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Listbox")->subscribeEvent(MultiColumnList::EventSelectionChanged, Event::Subscriber(ItemSelect));

		myRoot->getChild("OSD/Scrollbar1")->subscribeEvent(Scrollbar::EventScrollPositionChanged, Event::Subscriber(MoveCamera));
*/
		
 		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));

		mcl->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl->addColumn("Value", 0, cegui_absdim(200.0));
		mcl->addColumn("ID", 1, cegui_absdim(70.0));
		mcl->addColumn("Assoc", 2, cegui_absdim(70.0));
		mcl->addColumn("x", 3, cegui_absdim(70.0));
		mcl->addColumn("y", 4, cegui_absdim(70.0));

 		MultiColumnList* mcl2 = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("Roadbook"));

		mcl2->setSelectionMode(MultiColumnList::RowSingle) ;
		mcl2->addColumn("From", 0, cegui_absdim(200.0));
		mcl2->addColumn("To", 1, cegui_absdim(200.0));
		mcl2->addColumn("Dist", 2, cegui_absdim(80.0));
		mcl2->addColumn("ETA", 3, cegui_absdim(80.0));
		mcl2->addColumn("Instruction",4, cegui_absdim(300.0));

// 		BuildKeyboard();
		
	}
	catch (CEGUI::Exception& e)
	{
		fprintf(stderr,"CEGUI Exception occured: \n%s\n", e.getMessage().c_str());
		printf("quiting...\n");
		exit(1);
	}

	char * fontname="/usr/share/fonts/corefonts/verdana.ttf";

	printf("Gui initialized\n");


	int ctx = 0;
	int font = 0;
	ctx = glcGenContext();
	glcContext(ctx);
	font = glcNewFontFromFamily(glcGenFontID(), "Arial");
	glcFont(font);
// 	glcFontFace(font, "Italic");


	
}

static struct gui_priv *
gui_sdl_new(struct navit *nav, struct gui_methods *meth, int w, int h) 
{
	printf("Begin SDL init\n");
	struct gui_priv *this_;

	*meth=gui_sdl_methods;

	this_=g_new0(struct gui_priv, 1);
	init_sdlgui();
	printf("End SDL init\n");

	/*
 	this_->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
 	this_->vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_default_size(GTK_WINDOW(this_->win), w, h);
	gtk_window_set_title(GTK_WINDOW(this_->win), "Navit");
	gtk_widget_realize(this_->win);
	gtk_container_add(GTK_CONTAINER(this_->win), this_->vbox);
	gtk_widget_show_all(this_->win);
	*/

	return this_;
}

void
plugin_init(void)
{
	printf("registering sdl plugin\n");
	plugin_register_gui_type("sdl", gui_sdl_new);
}
