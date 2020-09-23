#include <gtk/gtk.h>
#include "gdigi_gtk.h"

extern void device_gui_enable(void);
extern void device_gui_disable(void);
extern void add_menubar(GApplication *app);
extern void gui_create(GApplication *app, void *device);

static void *gdigiDevice;
int gui_init(void *device)
{
    gdigiDevice = device;
    return g_application_run (G_APPLICATION (gdigi_app_new ()), 0, NULL);
}

struct _GdigiApp
{
    GtkApplication parent;
};

G_DEFINE_TYPE(GdigiApp, gdigi_app, GTK_TYPE_APPLICATION);

struct _GdigiAppWindow
{
    GtkApplicationWindow parent;
};

G_DEFINE_TYPE(GdigiAppWindow, gdigi_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void
gdigi_app_init (GdigiApp *app)
{
}

static void
gdigi_app_startup (GApplication *app)
{
    G_APPLICATION_CLASS (gdigi_app_parent_class)->startup (app);

    device_gui_enable();
    add_menubar(app);
}

static void
gdigi_app_activate (GApplication *app)
{
    GError *error = NULL;

    G_APPLICATION_CLASS (gdigi_app_parent_class)
        ->activate(app);

    g_application_register (G_APPLICATION (app), NULL, &error);
    if (error != NULL)
    {
        g_warning ("Unable to register GApplication: %s", error->message);
        g_error_free (error);
        error = NULL;
    }

    if (g_application_get_is_remote (G_APPLICATION (app)))
    {
      g_warning ("already running");
      return;
    }

    gui_create(app, gdigiDevice);
}

static void
gdigi_app_shutdown (GApplication *app)
{
    device_gui_disable();

    G_APPLICATION_CLASS (gdigi_app_parent_class)
        ->shutdown(app);
}

static void
gdigi_app_class_init (GdigiAppClass *class)
{
    G_APPLICATION_CLASS (class)->startup = gdigi_app_startup;
    G_APPLICATION_CLASS (class)->activate = gdigi_app_activate;
    G_APPLICATION_CLASS (class)->shutdown = gdigi_app_shutdown;
}

GdigiApp *
gdigi_app_new (void)
{
    return g_object_new (GDIGI_APP_TYPE,
                       "application-id", "org.gtk.gdigiapp",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       "register-session", TRUE,
                       NULL);
}

static void
gdigi_app_window_init (GdigiAppWindow *win)
{
}

static void
gdigi_app_window_class_init (GdigiAppWindowClass *class)
{
}

GdigiAppWindow *
gdigi_app_window_new (GdigiApp *app)
{
  return g_object_new (GDIGI_APP_WINDOW_TYPE, "application", app, NULL);
}
