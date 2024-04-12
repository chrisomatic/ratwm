

typedef struct
{
    unsigned int w, h;
    Display *dpy;
    int screen;
    Window root;
    Drawable drawable;
    GC gc;
} Drw;

Drw* draw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
    Drw *drw = calloc(1, sizeof(Drw));

    drw->dpy = dpy;
    drw->screen = screen;
    drw->root = root;
    drw->w = w;
    drw->h = h;
    drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
    drw->gc = XCreateGC(dpy, root, 0, NULL);
    XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

    return drw;
}

void draw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned long color, int filled)
{
    if (!drw)
        return;

    XSetForeground(drw->dpy, drw->gc, color);

    if (filled)
        XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
    else
        XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}
