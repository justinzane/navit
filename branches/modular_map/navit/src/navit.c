struct container *
navit_new(char *ui, struct coord_geo *center, double zoom)
{
	struct container *co;
	co=gui_gtk_window(1300000,7000000,8192);

	return co;
}


