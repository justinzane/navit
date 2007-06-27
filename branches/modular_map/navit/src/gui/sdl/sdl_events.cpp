
bool handleItemSelect(int r)
{
	using namespace CEGUI;

	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
	
	ListboxItem * item = mcl->getItemAtGridReference(MCLGridRef(r,0));
	ListboxItem * itemid = mcl->getItemAtGridReference(MCLGridRef(r,1));
	ListboxItem * item_assoc = mcl->getItemAtGridReference(MCLGridRef(r,2));


	Window* country_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox"));
	Window* twn_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
	Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));

	if(SDL_dest.current_search==SRCH_COUNTRY){
		extern Window* myRoot;
		country_edit->setText(item->getText());
		// Need to record the country here 
		twn_edit->activate();
		SDL_dest.current_search=SRCH_TOWN;
		myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");

	} else 	if(SDL_dest.current_search==SRCH_TOWN){
		extern Window* myRoot;
		twn_edit->setText(item->getText());
		SDL_dest.town_street_assoc=atoi(item_assoc->getText().c_str());
		SDL_dest.town=atoi(itemid->getText().c_str());
		printf(" town %s , id=%lx, assoc=%li\n",item->getText().c_str(),SDL_dest.town_street_assoc,SDL_dest.town_street_assoc);


		ListboxItem * itemx = mcl->getItemAtGridReference(MCLGridRef(r,3));
		ListboxItem * itemy = mcl->getItemAtGridReference(MCLGridRef(r,4));
	
		Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
		Dest_x->setText(itemx->getText().c_str());

		Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
		Dest_y->setText(itemy->getText().c_str());

		mcl->resetList();

		SDL_dest.current_search=SRCH_STREET;
		street_edit->activate();
		myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");

	} else if(SDL_dest.current_search==SRCH_STREET){
		extern Window* myRoot;
		street_edit->setText(item->getText());

		myRoot->getChild("Navit/Keyboard")->hide();

		ListboxItem * itemid = mcl->getItemAtGridReference(MCLGridRef(r,1));
		int segment_id=atoi(itemid->getText().c_str());
		printf("street seg id : %li\n",segment_id);
		
		extern struct container *co;
		struct block_info res_blk_inf;
		struct street_str *res_str;
		street_get_by_id(co->map_data, 33, segment_id,&res_blk_inf,&res_str );

		struct street_coord * streetcoord;
		streetcoord=street_coord_get(&res_blk_inf,res_str);

		printf("Street coordinates : %i,%i\n",streetcoord->c->x,streetcoord->c->y);

	 	char xbuff [256];
		sprintf(xbuff,"%li",streetcoord->c->x);
	 	char ybuff [256];
		sprintf(ybuff,"%li",streetcoord->c->y);

		Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
		Dest_x->setText(xbuff);

		Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
		Dest_y->setText(ybuff);

		struct street_name name;
// 		printf("street_name_get_by_id returns : %i\n",street_name_get_by_id(&name, res_blk_inf.mdata, res_str->nameid));
		street_name_get_by_id(&name, res_blk_inf.mdata, res_str->nameid);
// 		printf("name1:%s / name2%s\n",name.name1,name.name2);

		struct street_name_number_info num;
		struct street_name_info inf;

		SDL_dest.current_search=SRCH_NUMBER;
		mcl->resetList();

		while (street_name_get_info(&inf, &name)) {
			while(street_name_get_number_info(&num,&inf)){
// 				printf(" House Number : %i -> %i\n",num.first,num.last);
				for(int i=num.first;i<=num.last;i+=2){
					add_number_to_list(i,num.c->x,num.c->y);
				}
			}
		}

// 		route_to(streetcoord->c->x,streetcoord->c->y);
	} else if (SDL_dest.current_search==SRCH_NUMBER){

		struct coord pos;
		ListboxItem * itemx = mcl->getItemAtGridReference(MCLGridRef(r,3));
		ListboxItem * itemy = mcl->getItemAtGridReference(MCLGridRef(r,4));
	
		pos.x=atoi(itemx->getText().c_str());
		pos.y=atoi(itemy->getText().c_str());

		route_to(pos.x,pos.y);
	}

	return true;
}

bool ItemSelect(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;

	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
	ListboxItem * item = mcl->getFirstSelectedItem();
	handleItemSelect(mcl->getItemRowIndex(item));
}

bool handleMouseEnters(const CEGUI::EventArgs& event)
{

	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);

	using namespace CEGUI;
	myRoot->getChild("Navit/Keyboard")->getChild("Navit/Keyboard/Input")->setText("");
	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));

	String senderID = we.window->getName();

	if (senderID == "DestinationWindow/CountryEditbox"){
		// First, clean off the Street and Town Editbox
		extern Window* myRoot;
		Window* town_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
		town_edit->setText("");
		Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		street_edit->setText("");
		SDL_dest.current_search=SRCH_COUNTRY;

	} else if (senderID == "DestinationWindow/TownEditbox"){
		// First, clean off the Street Editbox
		extern Window* myRoot;
		Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		street_edit->setText("");
		SDL_dest.current_search=SRCH_TOWN;

	} else if (senderID == "DestinationWindow/StreetEditbox"){
		// First, make sure the user selected an entry in the town choice. If he hadn't, select the first for him.
  		if(SDL_dest.current_search==SRCH_TOWN){
			if (mcl->getRowCount()>0)
			{
				handleItemSelect(0);
			}			
		}
		SDL_dest.current_search=SRCH_STREET;

	}
}

void handle_destination_change(){
	using namespace CEGUI;

	struct search_param *search=&search_param2;
	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));


	if (SDL_dest.current_search==SRCH_COUNTRY)
	{	
		Editbox* country_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/CountryEditbox"));
		String content=country_edit->getText();

		mcl->resetList();

		country_search_by_name(content.c_str(), 1, destination_country_add, search);

	} else if (SDL_dest.current_search==SRCH_TOWN)
	{	
		Editbox* town_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
		String content=town_edit->getText();

		if(strlen(content.c_str())<4){

		}  else {
			extern struct container *co;
			search->map_data=co->map_data;

			mcl->resetList();
			mcl->setSortColumnByID(0);

			town_search_by_name(search->map_data, 33, content.c_str(), 1, destination_town_add, search);
		}
	} else if (SDL_dest.current_search==SRCH_STREET)
	{	
		Editbox* street_edit = static_cast<Editbox*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		
		String content=street_edit->getText();
		if(strlen(content.c_str())<1){

		}  else {
			extern struct container *co;
			search->map_data=co->map_data;

			mcl->resetList();
	
			street_name_search(search->map_data, 33, SDL_dest.town_street_assoc, content.c_str(), 1, destination_street_add, search);
		}
	}
}

bool DestinationEntryChange(const CEGUI::EventArgs& event)
{
	handleMouseEnters(event);
	handle_destination_change();

	return true;
}

bool DialogWindowSwitch(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;
	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);
	if(we.window->getParent()->getChild("DestinationWindow")->isVisible()){
		we.window->getParent()->getChild("DestinationWindow")->hide();
	} else {
		Window* town_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/TownEditbox"));
		town_edit->setText("");
		Window* street_edit = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/StreetEditbox"));
		street_edit->setText("");
		town_edit->activate();
		SDL_dest.current_search=SRCH_TOWN;
		MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
		mcl->resetList();
		we.window->getParent()->getChild("DestinationWindow")->show();
	}
	return true;
}

bool RoadBookSwitch(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;

// 	const CEGUI::WindowEventArgs& we =  static_cast<const CEGUI::WindowEventArgs&>(event);
	if(myRoot->getChild("Navit/RoadBook")->isVisible()){
		myRoot->getChild("Navit/RoadBook")->hide();
		WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->show();
	} else {
		myRoot->getChild("Navit/RoadBook")->show();
		WindowManager::getSingleton().getWindow("OSD/RoadbookButton")->hide();
	}
	return true;
}

bool ButtonGo(const CEGUI::EventArgs& event)
{
	using namespace CEGUI;
	MultiColumnList* mcl = static_cast<MultiColumnList*>(WindowManager::getSingleton().getWindow("DestinationWindow/Listbox"));
	// First, make sure the user selected an entry in the town choice. If he hadn't, select the first for him.
	if(SDL_dest.current_search==SRCH_TOWN){
		if (mcl->getRowCount()>0)
		{
			handleItemSelect(0);
		}
	}


	Window* Dest_x = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_x"));
	Window* Dest_y = static_cast<Window*>(myRoot->getChild("DestinationWindow")->getChild("DestinationWindow/Dest_y"));
	
	route_to(atoi(Dest_x->getText().c_str()),atoi(Dest_y->getText().c_str()));

	return true;
}


bool ZoomIn(const CEGUI::EventArgs& event)
{
	extern struct container *co;
	struct transformation *t=co->trans;
	if(t->scale>1){
		t->scale/=2;
	}

}

bool ZoomOut(const CEGUI::EventArgs& event)
{
	extern struct container *co;
	struct transformation *t=co->trans;
	t->scale*=2;
}

bool ButtonQuit(const CEGUI::EventArgs& event)
{
	exit(0);
}