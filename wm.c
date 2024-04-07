
#define MAX_CLIENTS 32

typedef struct
{
    Window w;
    Window frame;
} FrameClient;

FrameClient clients[MAX_CLIENTS] = {0};
int client_count = 0;

const char* event_type_str_lookup[37] = {"","","KeyPress","KeyRelease","ButtonPress","ButtonRelease","MotionNotify","EnterNotify","LeaveNotify","FocusIn","FocusOut","KeymapNotify","Expose","GraphicsExpose","NoExpose","VisibilityNotify","CreateNotify","DestroyNotify","UnmapNotify","MapNotify","MapRequest","ReparentNotify","ConfigureNotify","ConfigureRequest","GravityNotify","ResizeRequest","CirculateNotify","CirculateRequest","PropertyNotify","SelectionClear","SelectionRequest","SelectionNotify","ColormapNotify","ClientMessage","MappingNotify","GenericEvent","LASTEvent"};

Display* display;
Window root;

void wm_frame(Window w)
{
    const unsigned int  BORDER_WIDTH = 3;
    const unsigned long BORDER_COLOR = 0xff0000;
    const unsigned long BG_COLOR = 0x0000ff;

    XWindowAttributes x_window_attrs;
    int status = XGetWindowAttributes(display, w, &x_window_attrs);
    if(status == 0)
    {
        loge("Failed to get Window Attributes.");
        return;
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
          BG_COLOR);

      // 3. Select events on frame.
      XSelectInput(
          display,
          frame,
          SubstructureRedirectMask | SubstructureNotifyMask);
      
      // 4. Add client to save set, so that it will be restored and kept alive if we
      // crash.
      XAddToSaveSet(display, w);

      // 5. Reparent client window.
      XReparentWindow(
          display,
          w,
          frame,
          0, 0);  // Offset of client window within frame.
      // 6. Map frame.
      XMapWindow(display, frame);

      // 7. Save frame handle.
      FrameClient* client = &clients[client_count++];
      client->w = w;
      client->frame = frame;

#if 0
      // 8. Grab events for window management actions on client window.
      //   a. Move windows with alt + left button.
      XGrabButton(...);
      //   b. Resize windows with alt + right button.
      XGrabButton(...);
      //   c. Kill windows with alt + f4.
      XGrabKey(...);
      //   d. Switch windows with alt + tab.
      XGrabKey(...);
#endif

      logi("Framed window %p in frame %p", &w, &frame);
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

void on_destroy_notify(const XDestroyWindowEvent e)
{

}

void on_reparent_notify(const XReparentEvent e)
{

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

    XConfigureWindow(display, e.window, e.value_mask, &changes);
    logi("Resize window %p to size %d %d", &e.window, e.width, e.height);
}

void on_map_request(const XMapRequestEvent e)
{
    wm_frame(e.window);
    XMapWindow(display, e.window);
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

    return true;
}

void wm_run()
{
    wm_detected = false;
    XSetErrorHandler(on_wm_detected);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display,false);
    if(wm_detected)
    {
        loge("Detected another window manager on display %s", XDisplayString(display));
        return;
    }

    XSetErrorHandler(on_x_error);

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
            default:
                logw("Ignored event");
        }
    }

}

void wm_destroy()
{
    XCloseDisplay(display);
}

