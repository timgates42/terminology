#include "private.h"

#include <Elementary.h>
#include <assert.h>
#include "config.h"
#include "termio.h"
#include "options.h"
#include "options_behavior.h"
#include "main.h"

typedef struct _Behavior_Ctx {
     Evas_Object *op_w;
     Evas_Object *op_h;
     Evas_Object *op_wh_current;
     Evas_Object *term;
     Config *config;
} Behavior_Ctx;


#define CB(_cfg_name, _inv)                                     \
static void                                                     \
_cb_op_behavior_##_cfg_name(void *data, Evas_Object *obj,       \
                            void *_event EINA_UNUSED)           \
{                                                               \
   Behavior_Ctx *ctx = data;                                    \
   Config *config = ctx->config;                                \
   if (_inv)                                                    \
     config->_cfg_name = !elm_check_state_get(obj);             \
   else                                                         \
     config->_cfg_name = elm_check_state_get(obj);              \
   termio_config_update(ctx->term);                             \
   windows_update();                                            \
   config_save(config, NULL);                                   \
}

CB(jump_on_change, 0);
CB(jump_on_keypress, 0);
CB(disable_cursor_blink, 1);
CB(disable_visual_bell, 1);
CB(bell_rings, 0);
CB(flicker_on_key, 0);
CB(urg_bell, 0);
CB(active_links, 0);
CB(multi_instance, 0);
CB(xterm_256color, 0);
CB(erase_is_del, 0);
CB(drag_links, 0);
CB(login_shell, 0);
CB(mouse_over_focus, 0);
CB(disable_focus_visuals, 1);
CB(gravatar,  0);
CB(notabs,  1);
CB(mv_always_show, 0);
CB(ty_escapes, 0);
CB(changedir_to_current, 0);

#undef CB

static unsigned int
sback_double_to_expo_int(double d)
{
    if (d < 1.0)
        return 0;
    if (d >= 17.0)
        d = 17.0;
    return 1 << (unsigned char) d;
}

static char *
sback_indicator_units_format(double d)
{
    return (char*)eina_stringshare_printf("%'d", sback_double_to_expo_int(d));
}
static char *
sback_units_format(double d)
{
    return (char*)eina_stringshare_printf(_("%'d lines"), sback_double_to_expo_int(d));
}

static void
_cb_op_behavior_sback_chg(void *data,
                          Evas_Object *obj,
                          void *_event EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;
   Config *config = ctx->config;

   config->scrollback = (double) sback_double_to_expo_int(elm_slider_value_get(obj));
   termio_config_update(ctx->term);
   config_save(config, NULL);
}

static void
_cb_op_behavior_tab_zoom_slider_chg(void *data,
                                    Evas_Object *obj,
                                    void *_event EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;
   Config *config = ctx->config;

   config->tab_zoom = (double)(int)round(elm_slider_value_get(obj) * 10.0) / 10.0;
   termio_config_update(ctx->term);
   config_save(config, NULL);
}

static void
_cb_op_behavior_custom_geometry_current_set(void *data,
                                Evas_Object *obj EINA_UNUSED,
                                void *_event EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;
   Config *config = ctx->config;

   if (config->custom_geometry)
     {
        termio_size_get(ctx->term, &config->cg_width, &config->cg_height);
        elm_spinner_value_set(ctx->op_w, config->cg_width);
        elm_spinner_value_set(ctx->op_h, config->cg_height);
     }
   config_save(config, NULL);
}

static void
_cb_op_behavior_custom_geometry(void *data,
                                Evas_Object *obj,
                                void *_event EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;
   Config *config = ctx->config;

   config->custom_geometry = elm_check_state_get(obj);
   if (config->custom_geometry)
     {
        config->cg_width = (int) elm_spinner_value_get(ctx->op_w);
        config->cg_height = (int) elm_spinner_value_get(ctx->op_h);
     }
   config_save(config, NULL);

   elm_object_disabled_set(ctx->op_w, !config->custom_geometry);
   elm_object_disabled_set(ctx->op_h, !config->custom_geometry);
   elm_object_disabled_set(ctx->op_wh_current, !config->custom_geometry);
}

static void
_cb_op_behavior_cg_width(void *data,
                         Evas_Object *obj,
                         void *_event EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;
   Config *config = ctx->config;

   if (config->custom_geometry)
     {
        config->cg_width = (int) elm_spinner_value_get(obj);
        config_save(config, NULL);
     }
}

static void
_cb_op_behavior_cg_height(void *data,
                          Evas_Object *obj,
                          void *_event EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;
   Config *config = ctx->config;

   if (config->custom_geometry)
     {
        config->cg_height = (int) elm_spinner_value_get(obj);
        config_save(config, NULL);
     }
}

static void
_parent_del_cb(void *data,
               Evas *_e EINA_UNUSED,
               Evas_Object *_obj EINA_UNUSED,
               void *_event_info EINA_UNUSED)
{
   Behavior_Ctx *ctx = data;

   free(ctx);
}

void
options_behavior(Evas_Object *opbox, Evas_Object *term)
{
   Config *config = termio_config_get(term);
   Evas_Object *o, *bx, *sc, *frame;
   int w, h;
   const char *tooltip;
   Behavior_Ctx *ctx;

   termio_size_get(term, &w, &h);

   ctx = calloc(1, sizeof(*ctx));
   assert(ctx);

   ctx->config = config;
   ctx->term = term;

   frame = o = elm_frame_add(opbox);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(o, _("Behavior"));
   elm_box_pack_end(opbox, o);
   evas_object_show(o);

   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                  _parent_del_cb, ctx);

   sc = o = elm_scroller_add(opbox);
   elm_scroller_content_min_limit(sc, EINA_TRUE, EINA_FALSE);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(frame, o);
   evas_object_show(o);

   bx = o = elm_box_add(opbox);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_object_content_set(sc, o);
   evas_object_show(o);

#define CX(_lbl, _cfg_name, _inv)                                         \
   o = elm_check_add(bx);                                                 \
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);            \
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);               \
   elm_object_text_set(o, _lbl);                                          \
   elm_check_state_set(o, _inv ? !config->_cfg_name : config->_cfg_name); \
   elm_box_pack_end(bx, o);                                               \
   evas_object_show(o);                                                   \
   evas_object_smart_callback_add(o, "changed",                           \
                                  _cb_op_behavior_##_cfg_name, ctx)

   CX(_("Scroll to bottom on new content"), jump_on_change, 0);
   CX(_("Scroll to bottom when a key is pressed"), jump_on_keypress, 0);
   CX(_("React to key presses"), flicker_on_key, 0);
   CX(_("Cursor blinking"), disable_cursor_blink, 1);
   CX(_("Visual Bell"), disable_visual_bell, 1);
   CX(_("Bell rings"), bell_rings, 0);
   CX(_("Urgent Bell"), urg_bell, 0);
   CX(_("Active Links"), active_links, 0);
   CX(_("Multiple instances, one process"), multi_instance, 0);
   CX(_("Set TERM to xterm-256color"), xterm_256color, 0);
   CX(_("BackArrow sends Del (instead of BackSpace)"), erase_is_del, 0);
   CX(_("Drag & drop links"), drag_links, 0);
   CX(_("Start as login shell"), login_shell, 0);
   CX(_("Focus split under the Mouse"), mouse_over_focus, 0);
   CX(_("Focus-related visuals"), disable_focus_visuals, 1);
   CX(_("Gravatar integration"), gravatar, 0);
   CX(_("Show tabs"), notabs, 1);
   CX(_("Always show miniview"), mv_always_show, 0);
   CX(_("Enable special Terminology escape codes"), ty_escapes, 0);
   CX(_("Open new terminals in current working directory"), changedir_to_current, 0);

#undef CX


   o = elm_check_add(bx);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(o, _("Always open at size:"));
   elm_check_state_set(o, config->custom_geometry);
   elm_box_pack_end(bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "changed",
                                  _cb_op_behavior_custom_geometry, ctx);

   ctx->op_wh_current = o = elm_button_add(bx);
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, 0.0, 0.5);
   elm_object_text_set(o, _("Set Current:"));
   elm_box_pack_end(bx, o);
   evas_object_show(o);
   elm_object_disabled_set(o, !config->custom_geometry);
   evas_object_smart_callback_add(o, "clicked",
                                  _cb_op_behavior_custom_geometry_current_set,
                                  ctx);

   o = elm_label_add(bx);
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, 0.0, 0.5);
   elm_object_text_set(o, _("Width:"));
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   ctx->op_w = o = elm_spinner_add(bx);
   elm_spinner_editable_set(o, EINA_TRUE);
   elm_spinner_min_max_set(o, 2.0, 350.0);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   if (config->custom_geometry)
     elm_spinner_value_set(o, config->cg_width);
   else
     elm_spinner_value_set(o, w);
   elm_object_disabled_set(o, !config->custom_geometry);
   elm_box_pack_end(bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "changed",
                                  _cb_op_behavior_cg_width, ctx);

   o = elm_label_add(bx);
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, 0.0, 0.5);
   elm_object_text_set(o, _("Height:"));
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   ctx->op_h = o = elm_spinner_add(bx);
   elm_spinner_editable_set(o, EINA_TRUE);
   elm_spinner_min_max_set(o, 1.0, 150.0);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   if (config->custom_geometry)
     elm_spinner_value_set(o, config->cg_height);
   else
     elm_spinner_value_set(o, h);
   elm_object_disabled_set(o, !config->custom_geometry);
   elm_box_pack_end(bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "changed",
                                  _cb_op_behavior_cg_height, ctx);

   o = elm_separator_add(bx);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_separator_horizontal_set(o, EINA_TRUE);
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   o = elm_label_add(bx);
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, 0.0, 0.5);
   elm_object_text_set(o, _("Scrollback:"));
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   o = elm_slider_add(bx);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_slider_span_size_set(o, 40);
   elm_slider_step_set(o, 1);
   elm_slider_units_format_function_set(o,
                                        sback_units_format,
                                        (void(*)(char*))eina_stringshare_del);
   elm_slider_indicator_format_function_set(o,
                                            sback_indicator_units_format,
                                            (void(*)(char*))eina_stringshare_del);
   /* http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogFloat */
   union {
       float v;
       int c;
   } u;
   u.v = config->scrollback;
   u.c = (u.c >> 23) - 127;
   elm_slider_min_max_set(o, 0.0, 17.0);
   elm_slider_value_set(o, u.c);
   elm_box_pack_end(bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "delay,changed",
                                  _cb_op_behavior_sback_chg, ctx);

   o = elm_label_add(bx);
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, 0.0, 0.5);
   elm_object_text_set(o, _("Tab zoom/switch animation time:"));
   tooltip = _("Set the time of the animation that<br>"
       "takes places on tab switches,<br>"
       "be them by key binding, mouse<br>"
       "wheel or tabs panel mouse move");
   elm_object_tooltip_text_set(o, tooltip);
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   o = elm_slider_add(bx);
   elm_object_tooltip_text_set(o, tooltip);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_slider_span_size_set(o, 40);
   elm_slider_unit_format_set(o, _("%1.1f s"));
   elm_slider_indicator_format_set(o, _("%1.1f s"));
   elm_slider_min_max_set(o, 0.0, 1.0);
   elm_slider_value_set(o, config->tab_zoom);
   elm_box_pack_end(bx, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "delay,changed",
                                  _cb_op_behavior_tab_zoom_slider_chg, ctx);

   evas_object_size_hint_weight_set(opbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(opbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(o);
}
