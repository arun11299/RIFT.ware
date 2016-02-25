
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/*
 * @file rwnetconf_client-c.in.h
 * @author Tom Seidenberg
 * @date 2014/05/21
 * @brief RW.Netconf client library C plugin.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libpeas/peas.h>

// ATTN: Not yet
#if 0

#include "rwnetconf.h"
#include "rwnetconf_client-c.h"
#include "yangmodel.h"

#undef ABCD
#define ABCD \
  YMPA_1_0(yang_model_create_libncx,            guint64) \
/*YMPA_0_2(yang_model_filename_to_module,                 gchar*, gchar**,               const char*, char**) */\
  YMPA_1_2(yang_model_load_module,              guint64,  guint64, gchar*,               rw_yang_model_t*, const char*) \
  YMPA_1_1(yang_model_get_root_node,            guint64,  guint64,                       rw_yang_model_t*) \
  YMPA_1_2(yang_model_search_module_ns,         guint64,  guint64, gchar*,               rw_yang_model_t*, const char*) \
  YMPA_1_3(yang_model_search_module_name_rev,   guint64,  guint64, gchar*, gchar*,       rw_yang_model_t*, const char*, const char*) \
  YMPA_1_2(yang_model_search_module_ypbc,       guint64,  guint64, guint64,              rw_yang_model_t*, const rw_yang_pb_module_t*) \
  YMPA_1_1(yang_model_get_first_module,         guint64,  guint64,                       rw_yang_model_t*) \
  \
  YMPA_0_2(yang_module_get_location,                      guint64, gchar**,              rw_yang_module_t*, char**) \
  YMPA_1_1(yang_module_get_description,         gchar*,   guint64,                       rw_yang_module_t*) \
  YMPA_1_1(yang_module_get_name,                gchar*,   guint64,                       rw_yang_module_t*) \
  YMPA_1_1(yang_module_get_prefix,              gchar*,   guint64,                       rw_yang_module_t*) \
  YMPA_1_1(yang_module_get_ns,                  gchar*,   guint64,                       rw_yang_module_t*) \
  YMPA_1_1(yang_module_get_next_module,         guint64,  guint64,                       rw_yang_module_t*) \
  YMPA_1_1(yang_module_was_loaded_explicitly,   gboolean, guint64,                       rw_yang_module_t*) \
  YMPA_1_1(yang_module_get_first_node,          guint64,  guint64,                       rw_yang_module_t*) \
/*YMPA_1_1(yang_module_node_begin,              guint64,  guint64,                       rw_yang_module_t*) */\
/*YMPA_1_1(yang_module_node_end,                guint64,  guint64,                       rw_yang_module_t*) */\
  YMPA_1_1(yang_module_get_first_extension,     guint64,  guint64,                       rw_yang_module_t*) \
/*YMPA_1_1(yang_module_extension_begin,         guint64,  guint64,                       rw_yang_module_t*) */\
/*YMPA_1_1(yang_module_extension_end,           guint64,  guint64,                       rw_yang_module_t*) */\
  YMPA_1_1(yang_module_get_first_augment,       guint64,  guint64,                       rw_yang_module_t*) \
/*YMPA_1_1(yang_module_augment_begin,           guint64,  guint64,                       rw_yang_module_t*) */\
/*YMPA_1_1(yang_module_augment_end,             guint64,  guint64,                       rw_yang_module_t*) */\
  \
  YMPA_0_2(yang_node_get_location,                        guint64, gchar**,              rw_yang_node_t*, char**) \
  YMPA_1_1(yang_node_get_description,           gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_help_short,            gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_help_full,             gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_name,                  gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_prefix,                gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_ns,                    gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_mode_string,           gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_0_2(yang_node_set_mode,                            guint64, gchar*,               rw_yang_node_t*, const char *) \
  YMPA_1_1(yang_node_is_mode,                   gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_cli_print_hook_string, gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_0_2(yang_node_set_cli_print_hook,                  guint64, gchar*,               rw_yang_node_t*, const char *) \
  YMPA_1_1(yang_node_is_cli_print_hook,         gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_default_value,         gchar*,   guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_max_elements,          guint32,  guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_stmt_type,             guint32,  guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_config,                 gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_leafy,                  gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_listy,                  gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_key,                    gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_has_keys,                  gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_presence,               gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_mandatory,              gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_parent,                guint64,  guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_first_child,           guint64,  guint64,                       rw_yang_node_t*) \
/*YMPA_1_1(yang_node_child_begin,               guint64,  guint64,                       rw_yang_node_t*) */\
/*YMPA_1_1(yang_node_child_end,                 guint64,  guint64,                       rw_yang_node_t*) */\
  YMPA_1_1(yang_node_get_next_sibling,          guint64,  guint64,                       rw_yang_node_t*) \
  YMPA_1_3(yang_node_search_child,              guint64,  guint64, gchar*, gchar*,       rw_yang_node_t*, const char*, const char*) \
/*YMPA_1_2(yang_node_search_descendant_path,    guint64,  guint64, guint64,              rw_yang_node_t*, const rw_yang_path_element_t* const*) */\
  YMPA_1_1(yang_node_get_type,                  guint64,  guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_first_value,           guint64,  guint64,                       rw_yang_node_t*) \
/*YMPA_1_1(yang_node_value_begin,               guint64,  guint64,                       rw_yang_node_t*) */\
/*YMPA_1_1(yang_node_value_end,                 guint64,  guint64,                       rw_yang_node_t*) */\
  YMPA_1_1(yang_node_get_first_key,             guint64,  guint64,                       rw_yang_node_t*) \
/*YMPA_1_1(yang_node_key_begin,                 guint64,  guint64,                       rw_yang_node_t*) */\
/*YMPA_1_1(yang_node_key_end,                   guint64,  guint64,                       rw_yang_node_t*) */\
  YMPA_1_1(yang_node_get_first_extension,       guint64,  guint64,                       rw_yang_node_t*) \
/*YMPA_1_1(yang_node_extension_begin,           guint64,  guint64,                       rw_yang_node_t*) */\
/*YMPA_1_1(yang_node_extension_end,             guint64,  guint64,                       rw_yang_node_t*) */\
  YMPA_1_3(yang_node_search_extension,          guint64,  guint64, gchar*, gchar*,       rw_yang_node_t*, const char*, const char*) \
  YMPA_1_1(yang_node_get_module,                guint64,  guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_get_module_orig,           guint64,  guint64,                       rw_yang_node_t*) \
  YMPA_1_2(yang_node_matches_prefix,            gboolean, guint64, gchar*,               rw_yang_node_t*, const char*) \
/*YMPA_1_3(yang_node_parse_value,               guint32,  guint64, gchar*, guint64**,    rw_yang_node_t*, const char*, rw_yang_value_t**) */\
/*YMPA_0_2(yang_node_set_mode_path,                       guint64, gchar*,               rw_yang_node_t*, const char*) */\
  YMPA_1_1(yang_node_is_mode_path,              gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_rpc,                    gboolean, guint64,                       rw_yang_node_t*) \
  YMPA_1_1(yang_node_is_rpc_input,              gboolean, guint64,                       rw_yang_node_t*) \
  \
  YMPA_1_1(yang_key_get_list_node,              guint64,  guint64,                       rw_yang_key_t*) \
  YMPA_1_1(yang_key_get_key_node,               guint64,  guint64,                       rw_yang_key_t*) \
  YMPA_1_1(yang_key_get_next_key,               guint64,  guint64,                       rw_yang_key_t*) \
  \
  /* YangAugment C APIs - TBD */ \
  \
  YMPA_0_2(yang_type_get_location,                        guint64, gchar**,              rw_yang_type_t*, char **) \
  YMPA_1_1(yang_type_get_description,           gchar*,   guint64,                       rw_yang_type_t*) \
  YMPA_1_1(yang_type_get_help_short,            gchar*,   guint64,                       rw_yang_type_t*) \
  YMPA_1_1(yang_type_get_help_full,             gchar*,   guint64,                       rw_yang_type_t*) \
  YMPA_1_1(yang_type_get_prefix,                gchar*,   guint64,                       rw_yang_type_t*) \
  YMPA_1_1(yang_type_get_ns,                    gchar*,   guint64,                       rw_yang_type_t*) \
  YMPA_1_1(yang_type_get_leaf_type,             guint64,  guint64,                       rw_yang_type_t*) \
  YMPA_1_1(yang_type_get_first_value,           guint64,  guint64,                       rw_yang_type_t*) \
/*YMPA_1_1(yang_type_value_begin,               guint64,  guint64,                       rw_yang_type_t*) */\
/*YMPA_1_1(yang_type_value_end,                 guint64,  guint64,                       rw_yang_type_t*) */\
/*YMPA_1_3(yang_type_parse_value,               guint32,  guint64, gchar*, guint64**,    rw_yang_type_t*, const char*, rw_yang_value_t**) */\
  YMPA_1_2(yang_type_count_matches,             guint32,  guint64, gchar*,               rw_yang_type_t*, const char*) \
  YMPA_1_2(yang_type_count_partials,            guint32,  guint64, gchar*,               rw_yang_type_t*, const char*) \
  YMPA_1_1(yang_type_get_first_extension,       guint64,  guint64,                       rw_yang_type_t*) \
/*YMPA_1_1(yang_type_extension_begin,           guint64,  guint64,                       rw_yang_type_t*) */\
/*YMPA_1_1(yang_type_extension_end,             guint64,  guint64,                       rw_yang_type_t*) */\
  \
  YMPA_1_1(yang_value_get_name,                 gchar*,   guint64,                       rw_yang_value_t*) \
  YMPA_0_2(yang_value_get_location,                       guint64, gchar**,              rw_yang_value_t*, char **) \
  YMPA_1_1(yang_value_get_description,          gchar*,   guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_help_short,           gchar*,   guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_help_full,            gchar*,   guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_prefix,               gchar*,   guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_ns,                   gchar*,   guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_leaf_type,            guint64,  guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_max_length,           guint64,  guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_next_value,           guint64,  guint64,                       rw_yang_value_t*) \
  YMPA_1_2(yang_value_parse_value,              guint32,  guint64, gchar*,               rw_yang_value_t*, const char*) \
  YMPA_1_2(yang_value_parse_partial,            guint32,  guint64, gchar*,               rw_yang_value_t*, const char*) \
  YMPA_1_1(yang_value_is_keyword,               gboolean, guint64,                       rw_yang_value_t*) \
  YMPA_1_1(yang_value_get_first_extension,      guint64,  guint64,                       rw_yang_value_t*) \
/*YMPA_1_1(yang_value_extension_begin,          guint64,  guint64,                       rw_yang_value_t*) */\
/*YMPA_1_1(yang_value_extension_end,            guint64,  guint64,                       rw_yang_value_t*) */\
  \
  YMPA_0_2(yang_extension_get_location,                   guint64, gchar**,              rw_yang_extension_t*, char **) \
  YMPA_0_2(yang_extension_get_location_ext,               guint64, gchar**,              rw_yang_extension_t*, char **) \
  YMPA_1_1(yang_extension_get_value,            gchar*,   guint64,                       rw_yang_extension_t*) \
  YMPA_1_1(yang_extension_get_description_ext,  gchar*,   guint64,                       rw_yang_extension_t*) \
  YMPA_1_1(yang_extension_get_name,             gchar*,   guint64,                       rw_yang_extension_t*) \
  YMPA_1_1(yang_extension_get_module_ext,       gchar*,   guint64,                       rw_yang_extension_t*) \
  YMPA_1_1(yang_extension_get_prefix,           gchar*,   guint64,                       rw_yang_extension_t*) \
  YMPA_1_1(yang_extension_get_ns,               gchar*,   guint64,                       rw_yang_extension_t*) \
  YMPA_1_1(yang_extension_get_next_extension,   guint64,  guint64,                       rw_yang_extension_t*) \
  YMPA_1_3(yang_extension_search,               guint64,  guint64, gchar*, gchar*,       rw_yang_extension_t*, const char*, const char*) \
  YMPA_1_3(yang_extension_is_match,             gboolean, guint64, gchar*, gchar*,       rw_yang_extension_t*, const char*, const char*) \
  
#undef FPRINTF
#define FPRINTF(file, ...) fprintf(file, ##__VA_ARGS__)
//#define FPRINTF(file, ...)


#undef YMPA_0_2
#define YMPA_0_2(verb, a1Type, a2Type, i1Type, i2Type) \
    static void \
    rwnetconf_client__api__##verb(YangmodelPluginApi *api, a1Type a1, a2Type a2) \
    { \
      FPRINTF(stderr, "%s() called arg1=%p\n", #verb, (i1Type)a1); \
      rw_##verb((i1Type)a1, (i2Type)a2); \
      return; \
    } 

#undef YMPA_1_0
#define YMPA_1_0(verb, rType) \
    static rType \
    rwnetconf_client__api__##verb(YangmodelPluginApi *api) \
    { \
      FPRINTF(stderr, "%s() called\n", #verb ); \
      return (rType)rw_##verb(); \
    } 

#undef YMPA_1_1
#define YMPA_1_1(verb, rType, a1Type, i1Type) \
    static rType \
    rwnetconf_client__api__##verb(YangmodelPluginApi *api, a1Type a1) \
    { \
      FPRINTF(stderr, "%s() called arg1=%p\n", #verb, (i1Type)a1); \
      return (rType)rw_##verb((i1Type)a1); \
    } 

#undef YMPA_1_2
#define YMPA_1_2(verb, rType, a1Type, a2Type, i1Type, i2Type) \
    static rType \
    rwnetconf_client__api__##verb(YangmodelPluginApi *api, a1Type a1, a2Type a2) \
    { \
      FPRINTF(stderr, "%s() called arg1=%p\n", #verb, (i1Type)a1); \
      return (rType)rw_##verb((i1Type)a1, (i2Type)a2); \
    } 

#undef YMPA_1_3
#define YMPA_1_3(verb, rType, a1Type, a2Type, a3Type, i1Type, i2Type, i3Type) \
    static rType \
    rwnetconf_client__api__##verb(YangmodelPluginApi *api, a1Type a1, a2Type a2, a3Type a3) \
    { \
      FPRINTF(stderr, "%s() called arg1=%p\n", #verb, (i1Type)a1); \
      return (rType)rw_##verb((i1Type)a1, (i2Type)a2, (i3Type)a3); \
    } 

ABCD

#undef YMPA_0_2
#define YMPA_0_2(verb, a1Type, a2Type, i1Type, i2Type) \
    iface->verb = rwnetconf_client__api__##verb;

#undef YMPA_1_0
#define YMPA_1_0(verb, rType) \
    iface->verb = rwnetconf_client__api__##verb;

#undef YMPA_1_1
#define YMPA_1_1(verb, rType, a1Type, i1Type) \
    iface->verb = rwnetconf_client__api__##verb;

#undef YMPA_1_2
#define YMPA_1_2(verb, rType, a1Type, a2Type, i1Type, i2Type) \
    iface->verb = rwnetconf_client__api__##verb;

#undef YMPA_1_3
#define YMPA_1_3(verb, rType, a1Type, a2Type, i1Type, i2Type) \
    iface->verb = rwnetconf_client__api__##verb;

/*****************************************/
static void
rwnetconf_client_c_extension_init(YangmodelPluginCExtension *plugin)                                                                                                                  
{ 
  // This is called once for each extension created
  FPRINTF(stderr, "rwnetconf_client_c_extension_init() %p called\n",plugin);
}

static void
rwnetconf_client_c_extension_class_init(YangmodelPluginCExtensionClass *klass)
{
  FPRINTF(stderr, "rwnetconf_client_c_extension_class_init() %p called\n",klass);
}

static void
rwnetconf_client_c_extension_class_finalize(YangmodelPluginCExtensionClass *klass)
{ 
  FPRINTF(stderr, "rwnetconf_client_c_extension_class_finalize() %p called\n", klass);
}

#define VAPI_TO_C_AUTOGEN
#ifdef VAPI_TO_C_AUTOGEN

/* Don't modify the code below, it is autogenerated */

static void
rwnetconf_client__api__iface_init(YangmodelPluginApiIface *iface)
{
  ABCD
}

G_DEFINE_DYNAMIC_TYPE_EXTENDED(YangmodelPluginCExtension,
        rwnetconf_client_c_extension,
        PEAS_TYPE_EXTENSION_BASE,
        0,
        G_IMPLEMENT_INTERFACE_DYNAMIC(YANGMODEL_PLUGIN_TYPE_API,
                rwnetconf_client__api__iface_init)
        )

G_MODULE_EXPORT void
peas_register_types(PeasObjectModule *module)
{
  rwnetconf_client_c_extension_register_type(G_TYPE_MODULE(module));
  peas_object_module_register_extension_type(module,
        YANGMODEL_PLUGIN_TYPE_API,
        YANGMODEL_PLUGIN_C_EXTENSION_TYPE);
}

#endif //VAPI_TO_C_AUTOGEN

#endif
