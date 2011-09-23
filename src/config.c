/*
 *  wmfs2 by Martin Duquesnoy <xorg62@gmail.com> { for(i = 2011; i < 2111; ++i) ©(i); }
 *  For license, see COPYING.
 */

#include "config.h"
#include "wmfs.h"
#include "parse.h"
#include "tag.h"
#include "screen.h"
#include "infobar.h"
#include "util.h"

#define CONFIG_DEFAULT_PATH ".config/wmfs/wmfsrc2" /* tmp */

static void
config_theme(void)
{
     struct theme *t, *p = NULL;
     size_t i, n;
     struct conf_sec *sec, **ks;

     /* [themes] */
     sec = fetch_section_first(NULL, "themes");
     ks = fetch_section(sec, "theme");

     /* No theme section? Make one with default value anyway. */
     if(!(n = fetch_section_count(ks)))
          ++n;

     SLIST_INIT(&W->h.theme);

     /* [theme]*/
     for(i = 0; i < n; ++i)
     {
          t = (struct theme*)xcalloc(1, sizeof(struct theme));

          t->name = fetch_opt_first(ks[i], "default", "name").str;

          wmfs_init_font(fetch_opt_first(ks[i], "fixed", "font").str, t);

          /* bars */
          t->bars.fg    = color_atoh(fetch_opt_first(ks[i], "#CCCCCC", "bars_fg").str);
          t->bars.bg    = color_atoh(fetch_opt_first(ks[i], "#222222", "bars_bg").str);
          t->bars_width = fetch_opt_first(ks[i], "12", "bars_width").num;

          /*
           * Elements
           */
          t->tags_n.fg         = color_atoh(fetch_opt_first(ks[i], "#CCCCCC", "tags_normal_fg").str);
          t->tags_n.bg         = color_atoh(fetch_opt_first(ks[i], "#222222", "tags_normal_bg").str);
          t->tags_s.fg         = color_atoh(fetch_opt_first(ks[i], "#222222", "tags_sel_fg").str);
          t->tags_s.bg         = color_atoh(fetch_opt_first(ks[i], "#CCCCCC", "tags_sel_bg").str);
          t->tags_border_col   = color_atoh(fetch_opt_first(ks[i], "#888888", "tags_border_color").str);
          t->tags_border_width = fetch_opt_first(ks[i], "0", "tags_border_width").num;

          /* Client / frame */
          t->client_n.fg = color_atoh(fetch_opt_first(ks[i], "#CCCCCC", "client_normal_fg").str);
          t->client_n.bg = color_atoh(fetch_opt_first(ks[i], "#222222", "client_normal_bg").str);
          t->client_s.fg = color_atoh(fetch_opt_first(ks[i], "#222222", "client_sel_fg").str);
          t->client_s.bg = color_atoh(fetch_opt_first(ks[i], "#CCCCCC", "client_sel_bg").str);
          t->frame_bg    = color_atoh(fetch_opt_first(ks[i], "#555555", "frame_bg").str);
          t->client_titlebar_width = fetch_opt_first(ks[i], "12", "client_titlebar_width").num;
          t->client_border_width   = fetch_opt_first(ks[i], "1", "client_border_width").num;

          /* insert_tail with SLIST */
          if(SLIST_EMPTY(&W->h.theme))
               SLIST_INSERT_HEAD(&W->h.theme, t, next);
          else
               SLIST_INSERT_AFTER(p, t, next);

          p = t;
     }

     free(ks);
}

static void
config_bars(void)
{
     struct screen *s;
     struct theme *t;
     size_t i, n;
     struct conf_sec *sec, **ks;
     int screenid;
     char *elem;
     Barpos pos = BarTop;

     /* [bars] */
     sec = fetch_section_first(NULL, "bars");
     ks = fetch_section(sec, "bar");
     n = fetch_section_count(ks);

     /* [bar] */
     for(i = 0; i < n; ++i)
     {
          elem = fetch_opt_first(ks[i], "", "elements").str;
          screenid = fetch_opt_first(ks[i], "-1", "screen").num;
          t = name_to_theme(fetch_opt_first(ks[i], "default", "theme").str);
          pos = fetch_opt_first(ks[i], "0", "position").num;

          SLIST_FOREACH(s, &W->h.screen, next)
               if(screenid == s->id || screenid == -1)
                    infobar_new(s, t, pos, elem);
     }

     free(ks);
}


static void
config_tag(void)
{
     struct screen *s;
     struct tag *t;
     size_t i, n;
     struct conf_sec *sec, **ks;
     char *name;
     int screenid;

     /* [tags] */
     sec = fetch_section_first(NULL, "tags");
     ks = fetch_section(sec, "tag");
     n = fetch_section_count(ks);

     /* [tag] */
     for(i = 0; i < n; ++i)
     {
          name = fetch_opt_first(ks[i], "tag", "name").str;
          screenid = fetch_opt_first(ks[i], "-1", "screen").num;

          SLIST_FOREACH(s, &W->h.screen, next)
               if(screenid == s->id || screenid == -1)
               {
                    t = tag_new(s, name);

                    /* Set first tag as seltag */
                    if(t == TAILQ_FIRST(&s->tags))
                         s->seltag = t;
               }
     }

     free(ks);
}

static void
config_keybind(void)
{
     int i, n;
     size_t j;
     struct conf_sec *sec, **ks;
     struct opt_type *opt;
     char *cmd;
     struct keybind *k;

     /* [keys] */
     sec = fetch_section_first(NULL, "keys");
     ks = fetch_section(sec, "key");
     n = fetch_section_count(ks);

     SLIST_INIT(&W->h.keybind);

     /* [key] */
     for(i = 0; i < n; ++i)
     {
          k = (struct keybind*)xcalloc(1, sizeof(struct keybind));

          /* mod = {} */
          opt = fetch_opt(ks[i], "", "mod");

          for(j = k->mod = 0; j < fetch_opt_count(opt); ++j)
               k->mod |= modkey_keysym(opt[j].str);

          free(opt);

          /* key = */
          k->keysym = XStringToKeysym(fetch_opt_first(ks[i], "None", "key").str);

          /* func = */
          if(!(k->func = uicb_name_func(fetch_opt_first(ks[i], "", "func").str)))
          {
               warnx("configuration: Unknown Function \"%s\".",
                         fetch_opt_first(ks[i], "", "func").str);
               k->func = uicb_spawn;
          }

          /* cmd = */
          if((cmd = fetch_opt_first(ks[i], "", "cmd").str))
               k->cmd = xstrdup(cmd);

          SLIST_INSERT_HEAD(&W->h.keybind, k, next);
     }

     wmfs_grab_keys();

     free(ks);
}

void
config_init(void)
{
     char *path;

     xasprintf(&path, "%s/"CONFIG_DEFAULT_PATH, getenv("HOME"));

     if(get_conf(path) == -1)
          errx(1, "parsing configuration file (%s) failed.", path);

     config_theme();
     config_keybind();
     config_tag();
     config_bars();

     free(path);
     free_conf();
}