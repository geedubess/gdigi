#ifndef __GDIGIGTK_H
#define __GDIGIGTK_H

#include <gtk/gtk.h>

#define GDIGI_APP_TYPE (gdigi_app_get_type ())
G_DECLARE_FINAL_TYPE (GdigiApp, gdigi_app, GDIGI, APP, GtkApplication)

GdigiApp     *gdigi_app_new         (void);

#define GDIGI_APP_WINDOW_TYPE (gdigi_app_window_get_type ())
G_DECLARE_FINAL_TYPE (GdigiAppWindow, gdigi_app_window, GDIGI, APP_WINDOW, GtkApplicationWindow)

GdigiAppWindow *gdigi_app_window_new  (GdigiApp *app);
void            gdigi_app_window_open (GdigiAppWindow *win, GFile *file);

#endif /* __GDIGIGTK_H */
