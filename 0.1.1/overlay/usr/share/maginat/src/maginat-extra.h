static gchar *formato(GtkScale *s, gdouble v, gpointer d)
{
    return g_strdup_printf("%d%%", (int)v);
}

static gboolean perso(GtkWidget *w, GdkEvent *e, gpointer d)
{
    gtk_widget_hide(w);
    return FALSE;
}

static gboolean scorri(GtkStatusIcon *s, GdkEventScroll *e, gpointer d)
{
    int m, v = leggi(&m);
    imposta(e->direction == GDK_SCROLL_UP ? v + 5 : v - 5);
    aggiorna();
    return TRUE;
}
static void destro(GtkStatusIcon *s, guint b, guint t, gpointer d)
{
    system("maginat-vol toggle");
    aggiorna();
}

static gboolean timer(gpointer d)
{
    aggiorna();
    return TRUE;
}
