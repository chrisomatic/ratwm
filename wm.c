
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define MAX_CLIENTS 32

// Note: Window is an unsigned long in the Xlib
typedef struct
{
    Window w;
    Window frame;
} FrameClient;

FrameClient clients[MAX_CLIENTS] = {0};
int client_count = 0;

FrameClient* _frame_client_get(Window w)
{
    for(int i = 0; i < client_count; ++i)
        if(w == clients[i].w)
            return &clients[i];

    return NULL;
}

int _frame_client_get_index(Window w)
{
    for(int i = 0; i < client_count; ++i)
        if(w == clients[i].w)
            return i;

    return -1;
}

bool _frame_client_save(Window w, Window frame)
{
    int _index = client_count;
    bool new_client = true;

    for(int i = 0; i < client_count; ++i)
    {
        if(w == clients[i].w)
        {
            // found existing frame client
            _index = i;
            new_client = false;
            break;
        }
    }

    if(new_client)
    {
        if(client_count >= MAX_CLIENTS)
            return false;
       
        client_count++;
    }

    FrameClient* client = &clients[_index];
    client->w = w;
    client->frame = frame;

    return true;
}

void _frame_client_remove(Window w)
{
    int index = _frame_client_get_index(w);
    if(index < 0)
        return;

    client_count--;
    memcpy(&clients[index], &clients[client_count], sizeof(FrameClient));
}

typedef struct
{
    int x,y;
} Vec2i;

const unsigned int  BORDER_WIDTH = 3;
const unsigned long BORDER_COLOR = 0xf08000;
const unsigned long BG_COLOR     = 0x323232;
const unsigned int  BAR_HEIGHT   = 25;

const char* event_type_str_lookup[37] = {"","","KeyPress","KeyRelease","ButtonPress","ButtonRelease","MotionNotify","EnterNotify","LeaveNotify","FocusIn","FocusOut","KeymapNotify","Expose","GraphicsExpose","NoExpose","VisibilityNotify","CreateNotify","DestroyNotify","UnmapNotify","MapNotify","MapRequest","ReparentNotify","ConfigureNotify","ConfigureRequest","GravityNotify","ResizeRequest","CirculateNotify","CirculateRequest","PropertyNotify","SelectionClear","SelectionRequest","SelectionNotify","ColormapNotify","ClientMessage","MappingNotify","GenericEvent","LASTEvent"};

Display* display;
Window root;
Drw* drw;

int screen;
int screen_width;
int screen_height;

Vec2i drag_start_pos;
Vec2i drag_start_frame_pos;
Vec2i drag_start_frame_size;

void wm_frame(Window w, bool existed_before_wm)
{
    XWindowAttributes x_window_attrs;
    int status = XGetWindowAttributes(display, w, &x_window_attrs);
    if(status == 0)
    {
        loge("Failed to get Window Attributes.");
        return;
    }

    if(existed_before_wm)
    {
        if(x_window_attrs.override_redirect || x_window_attrs.map_state != IsViewable)
        {
            return;
        }
    }

    const Window frame = XCreateSimpleWindow(
          display,
          root,
          x_window_attrs.x,
          x_window_attrs.y,
          x_window_attrs.width,
          x_window_attrs.height,
          BORDER_WIDTH,
          BORDER_COLOR,
          BG_COLOR
    );

    XSelectInput(display, frame, SubstructureRedirectMask | SubstructureNotifyMask);
    XAddToSaveSet(display, w);

    XReparentWindow(display, w, frame, 0, 0);
    XMapWindow(display, frame);

    _frame_client_save(w, frame);

    // Grab universal window management actions on client window.
    XGrabButton(display, Button1, Mod1Mask, w, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, Button3, Mod1Mask, w, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabKey(display, XKeysymToKeycode(display, XK_F4), Mod1Mask, w, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask, w, false, GrabModeAsync, GrabModeAsync);

    logi("Framed window %lu in frame %lu", w, frame);
}

void wm_unframe(Window w)
{
    FrameClient* client = _frame_client_get(w);

    XUnmapWindow(display, client->frame);
    XReparentWindow(display, w, root, 0, 0);
    XRemoveFromSaveSet(display, w);
    XDestroyWindow(display, client->frame);

    _frame_client_remove(w);

    logi("Unframed window %lu [frame: %lu]", w, client->frame);
}


bool wm_detected;
int on_wm_detected(Display* display, XErrorEvent* e)
{
    if((int)e->error_code == BadAccess)
    {
        wm_detected = true; 
    }

    return 0;
}

int on_x_error(Display* display, XErrorEvent* e)
{
    return 0;
}

void on_create_notify(const XCreateWindowEvent e)
{
    return;
}

void on_reparent_notify(const XReparentEvent e)
{
    return;
}

void on_configure_request(const XConfigureRequestEvent e)
{
    XWindowChanges changes;

    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    if(client_count > 0)
    {
        FrameClient* cli = _frame_client_get(e.window);
        if(cli)
        {
            XConfigureWindow(display, cli->frame, e.value_mask, &changes);
            logi("Resize frame %lu to size %d %d", cli->frame, e.width, e.height);
        }
    }


    XConfigureWindow(display, e.window, e.value_mask, &changes);
    logi("Resize window %lu to size %d %d", e.window, e.width, e.height);
}

void on_map_request(const XMapRequestEvent e)
{
    wm_frame(e.window, false);
    XMapWindow(display, e.window);
}

void on_map_notify(const XMapEvent e)
{
    return;
}

void on_unmap_notify(const XUnmapEvent e)
{
    if (_frame_client_get(e.window) == NULL)
    {
        logi("Ignore UnmapNotify for non-client window %lu", e.window);
        return;
    }

    wm_unframe(e.window);
}


void on_configure_notify(const XConfigureEvent e)
{
    return;
}

void on_button_press(const XButtonEvent e)
{
    FrameClient* cli = _frame_client_get(e.window);
    if(!cli) return;

    drag_start_pos.x = e.x_root;
    drag_start_pos.y = e.y_root;

    Window returned_root;
    int x, y;
    unsigned width, height, border_width, depth;

    if(XGetGeometry(display,cli->frame, &returned_root, &x, &y, &width, &height, &border_width, &depth) == 0) 
    {
      loge("Failed to Get Geometry for frame %lu", cli->frame);
      return;
    }

    drag_start_frame_pos.x = x;
    drag_start_frame_pos.y = y;

    drag_start_frame_size.x = width;
    drag_start_frame_size.y = height;

    XRaiseWindow(display, cli->frame);
}

void on_button_release(const XButtonEvent e)
{
    return;
}

void on_motion_notify(const XMotionEvent e)
{
    FrameClient* cli = _frame_client_get(e.window);
    if(!cli) return;

    const Vec2i drag_pos = {e.x_root, e.y_root};
    const Vec2i delta    = {drag_pos.x - drag_start_pos.x, drag_pos.y - drag_start_pos.y};

    if (e.state & Button1Mask)
    {
        const Vec2i dest_frame_pos = {drag_start_frame_pos.x + delta.x, drag_start_frame_pos.y + delta.y};
        XMoveWindow(display, cli->frame, dest_frame_pos.x, dest_frame_pos.y);
    }
    else if (e.state & Button3Mask)
    {
        const Vec2i size_delta      = {MAX(delta.x, -drag_start_frame_size.x), MAX(delta.y, -drag_start_frame_size.y)};
        const Vec2i dest_frame_size = {drag_start_frame_size.x + size_delta.x, drag_start_frame_size.y + size_delta.y};

        XResizeWindow(display, cli->frame, dest_frame_size.x, dest_frame_size.y);
        XResizeWindow(display, e.window,   dest_frame_size.x, dest_frame_size.y);
    }
}

void on_key_press(const XKeyEvent e)
{
#if 0
    
    if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display, XK_F4)))
    {
        // alt + f4: Close window.

        Atom* supported_protocols;
        int num_supported_protocols;

        int got_protocols = XGetWMProtocols(display, e.window, &supported_protocols, &num_supported_protocols);
        bool has_delete_protocol = false;

        if(got_protocols)
        {
            for(int i = 0; i < num_supported_protocols; ++i)
            {
                if(supported_protocols + i == WM_DELETE_WINDOW)
                {
                    has_delete_protocol = true;
                    break;
                }
            }
        }

        if (has_delete_protocol)
        {
            logi("Gracefully deleting window");

            XEvent msg;
            memset(&msg, 0, sizeof(msg));
            msg.xclient.type = ClientMessage;
            msg.xclient.message_type = WM_PROTOCOLS;
            msg.xclient.window = e.window;
            msg.xclient.format = 32;
            msg.xclient.data.l[0] = WM_DELETE_WINDOW;

            XSendEvent(display, e.window, false, 0, &msg);
        }
        else
        {
            // force kill
            logi("Killing window %lu", e.window);
            XKillClient(display, e.window);
        }
    }
    else if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display, XK_Tab)))
    {
#if 0
        // alt + tab: Switch window.
        // 1. Find next window.



        auto i = clients_.find(e.window);
        CHECK(i != clients_.end());
        ++i;
        if (i == clients_.end()) {
          i = clients_.begin();
        }

        // 2. Raise and set focus.
        XRaiseWindow(display_, i->second);
        XSetInputFocus(display_, i->first, RevertToPointerRoot, CurrentTime);
#endif
    }
#endif
}

void on_key_release(const XKeyEvent e)
{
    return;
}

void on_destroy_notify(const XDestroyWindowEvent e)
{
    return;
}

void draw_bar()
{
    draw_rect(drw, 0, 0, screen_width, BAR_HEIGHT, 0x323232, true);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    size_t ret = strftime(s, sizeof(s), "%c", tm);
    
    XSetForeground(drw->dpy, drw->gc, 0xffffff);
    XDrawString(drw->dpy, drw->drawable, drw->gc, 2,12, s, strlen(s));

    XCopyArea(display, drw->drawable, root, drw->gc, 0, 0, screen_width, BAR_HEIGHT, 0, 0);
    XSync(display,false);
}

void on_expose(const XExposeEvent e)
{
    if(e.count > 0)
        return; // don't waste time updating if there are too many queued

    if(e.window != root)
        return;

    // draw bar
    draw_bar();
}

bool wm_create()
{
    Display* _display = XOpenDisplay(NULL);
    if(!_display)
    {
        loge("Failed to open XDisplay.");
        return false;
    }

    logi("Opened XDisplay.");

    display = _display;
    root    = DefaultRootWindow(_display);

    screen = DefaultScreen(display);
    screen_width = DisplayWidth(display, screen);
    screen_height = DisplayHeight(display, screen);

    drw = draw_create(display, 0, root, screen_width, BAR_HEIGHT);

    draw_bar();

    return true;
}

void wm_run()
{
    wm_detected = false;
    XSetErrorHandler(on_wm_detected);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | ExposureMask);
    XSync(display,false);
    if(wm_detected)
    {
        loge("Detected another window manager on display %s", XDisplayString(display));
        return;
    }

    XSetErrorHandler(on_x_error);

    XGrabServer(display);

    // handle any existing top level windows, by framing them
    Window returned_root;
    Window returned_parent;

    Window* top_level_windows;
    unsigned int top_level_windows_count;

    if(XQueryTree(display,root, &returned_root, &returned_parent, &top_level_windows, &top_level_windows_count))
    {
        if(returned_root == root)
        {
            for (unsigned int i = 0; i < top_level_windows_count; ++i)
            {
                wm_frame(top_level_windows[i], true);
            }
        }

        XFree(top_level_windows);
    }

    XUngrabServer(display);
    
    // Main event loop
    for(;;)
    {
        XEvent e;
        XNextEvent(display, &e);

        logi("Received event: %s", event_type_str_lookup[e.type]);

        switch(e.type)
        {
            case CreateNotify:
                on_create_notify(e.xcreatewindow);
                break;
            case DestroyNotify:
                on_destroy_notify(e.xdestroywindow);
                break;
            case ReparentNotify:
                on_reparent_notify(e.xreparent);
                break;
            case ConfigureRequest:
                on_configure_request(e.xconfigurerequest);
                break;
            case MapRequest:
                on_map_request(e.xmaprequest);
                break;
            case MapNotify:
                on_map_notify(e.xmap);
                break;
            case UnmapNotify:
                on_unmap_notify(e.xunmap);
                break;
            case ConfigureNotify:
                on_configure_notify(e.xconfigure);
                break;
            case ButtonPress:
                on_button_press(e.xbutton);
                break;
            case ButtonRelease:
                on_button_release(e.xbutton);
                break;
            case MotionNotify:
                // Skip any already pending motion events.
                while (XCheckTypedWindowEvent(display, e.xmotion.window, MotionNotify, &e)) {}
                on_motion_notify(e.xmotion);
                break;
            case KeyPress:
                on_key_press(e.xkey);
                break;
            case KeyRelease:
                on_key_release(e.xkey);
                break;
            case Expose:
                on_expose(e.xexpose);
                break;
            default:
                logw("Ignored event, type: %s", event_type_str_lookup[e.type]);
        }
    }

}

void wm_destroy()
{
    XCloseDisplay(display);
}

