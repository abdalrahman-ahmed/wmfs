/*
 *  wmfs2 by Martin Duquesnoy <xorg62@gmail.com> { for(i = 2011; i < 2111; ++i) ©(i); }
 *  For license, see COPYING.
 */

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "wmfs.h"
#include "event.h"
#include "util.h"

int
wmfs_error_handler(Display *d, XErrorEvent *event)
{
      char mess[256];

      /* Check if there is another WM running */
      if(event->error_code == BadAccess
                && W->root == event->resourceid)
           errx(EXIT_FAILURE, "Another Window Manager is already running.");

      /* Ignore focus change error for unmapped client
       * 42 = X_SetInputFocus
       * 28 = X_GrabButton
       */
     if(client_gb_win(event->resourceid))
          if(event->error_code == BadWindow
                    || event->request_code == 42
                    || event->request_code == 28)
               return 0;


     XGetErrorText(d, event->error_code, mess, 128);
     warnx("%s(%d) opcodes %d/%d\n  resource #%lx\n",
               mess,
               event->error_code,
               event->request_code,
               event->minor_code,
               event->resourceid);

     return 1;
}

int
wmfs_error_handler_dummy(Display *d, XErrorEvent *event)
{
     (void)d;
     (void)event;

     return 0;
}

static void
wmfs_xinit(void)
{
     char **misschar, **names, *defstring;
     int d, i = 0, j = 0;
     XModifierKeymap *mm;
     XFontStruct **xfs = NULL;
     XSetWindowAttributes at;

     /*
      * X Error handler
      */
     XSetErrorHandler(wmfs_error_handler);

     /*
      * X var
      */
     W->xscreen = DefaultScreen(W->dpy);
     W->xdepth = DefaultDepth(W->dpy, W->xscreen);
     W->gc = DefaultGC(W->dpy, W->xscreen);

     /*
      * Root window/cursor
      */
     W->root = RootWindow(W->dpy, W->xscreen);

     at.event_mask = KeyMask | ButtonMask | MouseMask | PropertyChangeMask
          | SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask;
     at.cursor = XCreateFontCursor(W->dpy, XC_left_ptr);

     XChangeWindowAttributes(W->dpy, W->root, CWEventMask | CWCursor, &at);

     /*
      * Font
      */
     setlocale(LC_CTYPE, "");

     W->font.fontset = XCreateFontSet(W->dpy, "fixed", &misschar, &d, &defstring);

     XExtentsOfFontSet(W->font.fontset);
     XFontsOfFontSet(W->font.fontset, &xfs, &names);

     W->font.as    = xfs[0]->max_bounds.ascent;
     W->font.de    = xfs[0]->max_bounds.descent;
     W->font.width = xfs[0]->max_bounds.width;

     W->font.height = W->font.as + W->font.de;

     if(misschar)
          XFreeStringList(misschar);

     /*
      * Keys
      */
     mm = XGetModifierMapping(W->dpy);

     for(; i < 8; i++)
          for(; j < mm->max_keypermod; ++j)
               if(mm->modifiermap[i * mm->max_keypermod + j]
                         == XKeysymToKeycode(W->dpy, XK_Num_Lock))
                    W->numlockmask = (1 << i);
     XFreeModifiermap(mm);

}

void
wmfs_grab_keys(void)
{
     KeyCode c;
     Keybind *k;

     XUngrabKey(W->dpy, AnyKey, AnyModifier, W->root);

     SLIST_FOREACH(k, &W->h.keybind, next)
          if((c = XKeysymToKeycode(W->dpy, k->keysym)))
          {
               XGrabKey(W->dpy, c, k->mod, W->root, True, GrabModeAsync, GrabModeAsync);
               XGrabKey(W->dpy, c, k->mod | LockMask, W->root, True, GrabModeAsync, GrabModeAsync);
               XGrabKey(W->dpy, c, k->mod | W->numlockmask, W->root, True, GrabModeAsync, GrabModeAsync);
               XGrabKey(W->dpy, c, k->mod | LockMask | W->numlockmask, W->root, True, GrabModeAsync, GrabModeAsync);
          }
}

/** Scan if there are windows on X
 *  for manage it
*/
static void
wmfs_scan(void)
{
     int i, n;
     XWindowAttributes wa;
     Window usl, usl2, *w = NULL;
     Client *c;

     /*
        Atom rt;
        int s, rf, tag = -1, screen = -1, flags = -1, i;
        ulong ir, il;
        uchar *ret;
      */

     if(XQueryTree(W->dpy, W->root, &usl, &usl2, &w, &n))
          for(i = n - 1; i != -1; --i)
          {
               XGetWindowAttributes(W->dpy, w[i], &wa);

               if(!wa.override_redirect && wa.map_state == IsViewable)
               {/*
                    if(XGetWindowProperty(dpy, w[i], ATOM("_WMFS_TAG"), 0, 32,
                                   False, XA_CARDINAL, &rt, &rf, &ir, &il, &ret) == Success && ret)
                    {
                         tag = *ret;
                         XFree(ret);
                    }

                    if(XGetWindowProperty(dpy, w[i], ATOM("_WMFS_SCREEN"), 0, 32,
                                   False, XA_CARDINAL, &rt, &rf, &ir, &il, &ret) == Success && ret)
                    {
                         screen = *ret;
                         XFree(ret);
                    }

                    if(XGetWindowProperty(dpy, w[i], ATOM("_WMFS_FLAGS"), 0, 32,
                                   False, XA_CARDINAL, &rt, &rf, &ir, &il, &ret) == Success && ret)
                    {
                         flags = *ret;
                         XFree(ret);
                     }
                 */
                    /*c = */ client_new(w[i], &wa);

                    /*
                    if(tag != -1)
                         c->tag = tag;
                    if(screen != -1)
                         c->screen = screen;
                    if(flags != -1)
                         c->flags = flags;
                    */
               }
          }

     XFree(w);

     return;
}

static void
wmfs_loop(void)
{
     XEvent ev;

     W->running = true;

     while(W->running || XPending(W->dpy))
     {
          XNextEvent(W->dpy, &ev);
          HANDLE_EVENT(&ev);
     }
}

static void
wmfs_init(void)
{
     /* X init */
     wmfs_xinit();

     /* EWMH init */
     ewmh_init();

     /* Event init */
     event_init();

     /* Screen init */
     screen_init();

}

void
wmfs_quit(void)
{
     W->running = false;

     /* X stuffs */
     XFreeGC(W->dpy, W->gc);
     XFreeFontSet(W->dpy, W->font.fontset);

     screen_free();

     XCloseDisplay(W->dpy);

     free(W->net_atom);
     free(W);
}


int
main(int argc, char **argv)
{
     W = (struct Wmfs*)xcalloc(1, sizeof(struct Wmfs));

     /* Get X display */
     if(!(W->dpy = XOpenDisplay(NULL)))
     {
          fprintf(stderr, "%s: Can't open X server\n", argv[0]);
          exit(EXIT_FAILURE);
     }

     /* Opt */

     /* Core */
     wmfs_init();
     wmfs_scan();

     wmfs_loop();

     wmfs_quit();

     return 1;
}