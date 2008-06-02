#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <glib.h>
#include "item.h"
#include "attr.h"
#include "navit.h"
#include "search.h"
#include "debug.h"
#include "util.h"
#include "resources\resource.h"

static const TCHAR g_szDataClassName[] = TEXT("navit_gui_datawindow_class");


struct datawindow_priv
{
    HWND hwnd;
    HWND hwndLabel;
    HWND hwndEdit;
    HWND hwndList;
    enum attr_type currentSearchState;
    struct search_list *sl;
    struct navit *nav;
};

static void destroy_destination_window(struct datawindow_priv *datawindow)
{
    DestroyWindow(datawindow->hwnd);
    search_list_destroy(datawindow->sl);
    g_free(datawindow);
}

static void notify_apply(struct datawindow_priv *datawindow, int index)
{
    TCHAR txtBuffer[1024];
    struct attr search_attr;
    struct search_list_result *res;

    ListView_GetItemText(datawindow->hwndList, index, 1, txtBuffer, 1024);

    TCHAR_TO_UTF8(txtBuffer, search_string);

    search_attr.type = datawindow->currentSearchState;
    search_attr.u.str = search_string;

    search_list_search(datawindow->sl, &search_attr, 0);
    res=search_list_get_result(datawindow->sl);

    switch (datawindow->currentSearchState)
    {
    case attr_country_name:
    {
        datawindow->currentSearchState = attr_town_name;
        Edit_SetText(datawindow->hwndLabel, TEXT("Postal or City"));
    }
    break;
    case attr_town_name:
    {
        datawindow->currentSearchState = attr_street_name;
        Edit_SetText(datawindow->hwndLabel, TEXT("Street"));
    }
    break;
    case attr_street_name:
    {

        navit_set_destination(datawindow->nav, res->c, "Mein Test");

        destroy_destination_window(datawindow);
    }
    break;
    default:
        break;

    }

    Edit_SetText(datawindow->hwndEdit, TEXT(""));
    SetFocus(datawindow->hwndEdit);
}

static void notify_textchange(struct datawindow_priv *datawindow, TCHAR *newText)
{
    struct attr search_attr;
    struct search_list_result *res;

    (void)ListView_DeleteAllItems( datawindow->hwndList);

    TCHAR_TO_UTF8(newText, search_string);

    search_attr.type = datawindow->currentSearchState;
    search_attr.u.str = search_string;

    if (lstrlen(newText)<1)
        return;

    search_list_search(datawindow->sl, &search_attr, 1);


    TCHAR *tcharBuffer;
    int listIndex = 0;

    LVITEM lvI;
    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    while ((res=search_list_get_result(datawindow->sl)) && listIndex < 50)
    {

        switch (search_attr.type)
        {
        case attr_country_name:
            tcharBuffer = newSysString(res->country->name);
            break;
        case attr_town_name:
            tcharBuffer = newSysString(res->town->name);
            break;
        case attr_street_name:
            if (res->street->name)
            {
                tcharBuffer = newSysString(res->street->name);
            }
            else
            {
                continue;
            }
            break;
        default:
            dbg(0, "Unhandled search type");
        }

//	    listIndex = SendMessage( datawindow->hwndList, LB_ADDSTRING, 0, tcharBuffer);
        lvI.iItem = listIndex;
        lvI.iImage = listIndex;
        lvI.iSubItem = 0;
        lvI.lParam = (LPARAM) res->country->iso2;
        UTF8_TO_TCHAR(res->country->iso2, converted_iso2);
        lvI.pszText = converted_iso2;//LPSTR_TEXTCALLBACK; // sends an LVN_GETDISP message.
        (void)ListView_InsertItem(datawindow->hwndList, &lvI);
        ListView_SetItemText(datawindow->hwndList, listIndex, 1, tcharBuffer);
//	    if ( listIndex != LB_ERR )
//	    {
//	        SendMessage( datawindow->hwndList, LB_SETITEMDATA, listIndex, search_attr.type);
//	    }
        g_free(tcharBuffer);
        dbg(0,"%s\n", res->country->name);
        listIndex++;
    }
}

static BOOL init_lv_columns(HWND hWndListView)
{
    TCHAR szText[][8] = {TEXT("Iso"),TEXT("Country")};     // temporary buffer
    LVCOLUMN lvc;
    int iCol;

    // Initialize the LVCOLUMN structure.
    // The mask specifies that the format, width, text, and subitem members
    // of the structure are valid.
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    // Add the columns
    for (iCol = 0; iCol < 2; iCol++)
    {
        lvc.iSubItem = iCol;
        lvc.pszText = szText[iCol];
        lvc.cx = 100;     // width of column in pixels

        if ( iCol < 2 )
            lvc.fmt = LVCFMT_LEFT;  // left-aligned column
        else
            lvc.fmt = LVCFMT_RIGHT; // right-aligned column

        if (ListView_InsertColumn(hWndListView, iCol, &lvc) == -1)
            return FALSE;
    }
    return TRUE;
}


static LRESULT CALLBACK window_proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    dbg(0 , "Message: %d\n", Message);

    switch (Message)
    {
    case WM_CREATE:
    {

//            // Add text to the window.
//            SendMessage(datawindow->hwndEdit, WM_SETTEXT, 0, (LPARAM) lpszLatin);
        return 0;
    }
    break;
    case WM_SIZE:
    {
        struct datawindow_priv* datawindow = (struct datawindow_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

        if (datawindow)
        {
            MoveWindow(datawindow->hwndLabel,
                       0, 0,                  // starting x- and y-coordinates
                       LOWORD(lParam),        // width of client area
                       20,        // height of client area
                       TRUE);                 // repaint window
            MoveWindow(datawindow->hwndEdit,
                       0, 20,                  // starting x- and y-coordinates
                       LOWORD(lParam),        // width of client area
                       20,        // height of client area
                       TRUE);                 // repaint window
            MoveWindow(datawindow->hwndList,
                       0, 40,                  // starting x- and y-coordinates
                       LOWORD(lParam),        // width of client area
                       HIWORD(lParam) - 40,        // height of client area
                       TRUE);                 // repaint window
        }
        return 0;
    }
    break;
    case WM_DESTROY:
    {
        struct attr attr;
        struct datawindow_priv* datawindow = (struct datawindow_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

        attr.type=attr_destination;
        attr.u.num = 0; // TODO
        if (!navit_set_attr(datawindow->nav, &attr))
        {
            dbg(0, "Failed to set attr_orientation\n");
        }
        return 0;
    }
    case WM_NOTIFY:
    {
        struct datawindow_priv* datawindow = (struct datawindow_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

        if ( datawindow )
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_DBLCLK:
            {
                LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
                notify_apply(datawindow, lpnmitem->iItem );
            }
            }
        }
    }
    break;
    case WM_COMMAND:
    {
        struct datawindow_priv* datawindow = (struct datawindow_priv*)GetWindowLongPtr( hwnd , DWLP_USER );

        if ( datawindow )
        {
            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
            {
                if ( (HWND)lParam == datawindow->hwndEdit )
                {
                    int lineLength = SendMessage(datawindow->hwndEdit, EM_LINELENGTH, 0, 0) + 1;
                    TCHAR line[lineLength];
                    *((LPWORD)line) = lineLength;
                    SendMessage(datawindow->hwndEdit, EM_GETLINE, 0, (LPARAM)(LPTSTR)line);
                    line[lineLength-1] = 0;
                    notify_textchange(datawindow, line );
                }
                dbg(0 , "%x:%x", lParam, datawindow->hwndEdit);
                dbg(2 , "Lost Focus");
            }
            break;
            }
        }
    }
    break;

    default:
        return DefWindowProc(hwnd, Message, wParam, lParam);
    }

    return 0;
}

BOOL register_destination_window()
{
    WNDCLASS wc;

    wc.style		 = 0;
    wc.lpfnWndProc	 = window_proc;
    wc.cbClsExtra	 = 0;
    wc.cbWndExtra	 = 32;
    wc.hInstance	 = NULL;
    wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szDataClassName;
    wc.hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NAVIT));

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    return TRUE;
}

HANDLE create_destination_window( struct navit *nav )
{


    struct datawindow_priv *this_;

    this_=g_new0(struct datawindow_priv, 1);
    this_->nav = nav;
    this_->currentSearchState = attr_country_name;
    this_->sl=search_list_new(navit_get_mapset(this_->nav));

    this_->hwnd = CreateWindowEx(
                      WS_EX_CLIENTEDGE,
                      g_szDataClassName,
                      TEXT("Destination Input"),
#if defined(__CEGCC__)
                      WS_SYSMENU | WS_CLIPCHILDREN,
                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
#else
                      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                      CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
#endif
                      NULL, NULL, NULL, NULL);

    if (this_->hwnd == NULL)
    {
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"), MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    SetWindowLongPtr( this_->hwnd , DWLP_USER, (LONG_PTR) this_ );

    this_->hwndLabel = CreateWindow(WC_STATIC,      // predefined class
                                    TEXT("Country"),        // no window title
                                    WS_CHILD | WS_VISIBLE | ES_LEFT , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                    0, 0, 0, 0,  // set size in WM_SIZE message
                                    this_->hwnd,        // parent window
                                    NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                    (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                    NULL);       // pointer not needed

    this_->hwndEdit = CreateWindow(WC_EDIT,      // predefined class
                                   NULL,        // no window title
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                   0, 0, 0, 0,  // set size in WM_SIZE message
                                   this_->hwnd,        // parent window
                                   NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                   (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                   NULL);       // pointer not needed

    this_->hwndList = CreateWindow(WC_LISTVIEW,      // predefined class
                                   NULL,        // no window title
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | LVS_REPORT  , //| WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL
                                   0, 0, 0, 0,  // set size in WM_SIZE message
                                   this_->hwnd,        // parent window
                                   NULL,//(HMENU) ID_EDITCHILD,   // edit control ID
                                   (HINSTANCE) GetWindowLong(this_->hwnd, GWL_HINSTANCE),
                                   NULL);       // pointer not needed

    (void)ListView_SetExtendedListViewStyle(this_->hwndList,LVS_EX_FULLROWSELECT);
    init_lv_columns(this_->hwndList);
    SetFocus(this_->hwndEdit);
    ShowWindow(this_->hwnd, TRUE);
    UpdateWindow(this_->hwnd);




//	g_idle_add (message_pump, NULL);

    return this_->hwnd;
}

