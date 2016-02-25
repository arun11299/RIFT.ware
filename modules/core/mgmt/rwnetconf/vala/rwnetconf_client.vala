
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnetconf_client.vala
 * @author Tom Seidenberg
 * @date 2014/05/21
 * @brief Vala interface for RW.Netconf client APIs.
 *
 * @abstract
 * This vala specification defines the interface for RW.Netconf
 * client APIs.
 */

namespace YangmodelPlugin {
  public interface Api: GLib.Object {
/*
    public abstract uint64 yang_model_create_libncx();
 // public abstract void yang_model_filename_to_module(owned string filename, out string str);
    public abstract uint64 yang_model_load_module(uint64 model, owned string module_name);
    public abstract uint64 yang_model_get_root_node(uint64 model);
    public abstract uint64 yang_model_search_module_ns(uint64 model, owned string ns);
    public abstract uint64 yang_model_search_module_name_rev(uint64 model, owned string name, owned string revision);
    public abstract uint64 yang_model_search_module_ypbc(uint64 model, uint64 ypbc_mod);
    public abstract uint64 yang_model_get_first_module(uint64 model);

    public abstract void yang_module_get_location(uint64 ymod, out string str);
    public abstract string? yang_module_get_description(uint64 ymod);
    public abstract string? yang_module_get_name(uint64 ymod);
    public abstract string? yang_module_get_prefix(uint64 ymod);
    public abstract string? yang_module_get_ns(uint64 ymod);
    public abstract uint64 yang_module_get_next_module(uint64 ymod);
    public abstract bool yang_module_was_loaded_explicitly(uint64 ymod);
    public abstract uint64 yang_module_get_first_node(uint64 ymod);
 // public abstract uint64 yang_module_node_begin(uint64 ymod);
 // public abstract uint64 yang_module_node_end(uint64 ymod);
    public abstract uint64 yang_module_get_first_extension(uint64 ymod);
 // public abstract uint64 yang_module_extension_begin(uint64 ymod);
 // public abstract uint64 yang_module_extension_end(uint64 ymod);
    public abstract uint64 yang_module_get_first_augment(uint64 ymod);
 // public abstract uint64 yang_module_augment_begin(uint64 ymod);
 // public abstract uint64 yang_module_augment_end(uint64 ymod);

    public abstract void yang_node_get_location(uint64 ynode, out string str);
    public abstract string? yang_node_get_description(uint64 ynode);
    public abstract string? yang_node_get_help_short(uint64 ynode);
    public abstract string? yang_node_get_help_full(uint64 ynode);
    public abstract string? yang_node_get_name(uint64 ynode);
    public abstract string? yang_node_get_prefix(uint64 ynode);
    public abstract string? yang_node_get_ns(uint64 ynode);
    public abstract string? yang_node_get_mode_string(uint64 ynode);
    public abstract void yang_node_set_mode(uint64 ynode, owned string display);
    public abstract bool yang_node_is_mode(uint64 ynode);
    public abstract string? yang_node_get_cli_print_hook_string(uint64 ynode);
    public abstract void yang_node_set_cli_print_hook(uint64 ynode, owned string api);
    public abstract bool yang_node_is_cli_print_hook(uint64 ynode);
    public abstract string? yang_node_get_default_value(uint64 ynode);
    public abstract uint32 yang_node_get_max_elements(uint64 ynode);
    public abstract uint32 yang_node_get_stmt_type(uint64 ynode);
    public abstract bool yang_node_is_config(uint64 ynode);
    public abstract bool yang_node_is_leafy(uint64 ynode);
    public abstract bool yang_node_is_listy(uint64 ynode);
    public abstract bool yang_node_is_key(uint64 ynode);
    public abstract bool yang_node_has_keys(uint64 ynode);
    public abstract bool yang_node_is_presence(uint64 ynode);
    public abstract bool yang_node_is_mandatory(uint64 ynode);
    public abstract uint64 yang_node_get_parent(uint64 ynode);
    public abstract uint64 yang_node_get_first_child(uint64 ynode);
 // public abstract uint64 yang_node_child_begin(uint64 ynode);
 // public abstract uint64 yang_node_child_end(uint64 ynode);
    public abstract uint64 yang_node_get_next_sibling(uint64 ynode);
    public abstract uint64 yang_node_search_child(uint64 ynode, owned string name, owned string ns);
 // public abstract uint64 yang_node_search_descendant_path(uint64 ynode, uint64 path);
    public abstract uint64 yang_node_get_type(uint64 ynode);
    public abstract uint64 yang_node_get_first_value(uint64 ynode);
 // public abstract uint64 yang_node_value_begin(uint64 ynode);
 // public abstract uint64 yang_node_value_end(uint64 ynode);
    public abstract uint64 yang_node_get_first_key(uint64 ynode);
 // public abstract uint64 yang_node_key_begin(uint64 ynode);
 // public abstract uint64 yang_node_key_end(uint64 ynode);
    public abstract uint64 yang_node_get_first_extension(uint64 ynode);
 // public abstract uint64 yang_node_extension_begin(uint64 ynode);
 // public abstract uint64 yang_node_extension_end(uint64 ynode);
    public abstract uint64 yang_node_search_extension(uint64 ynode, owned string module, owned string ext);
    public abstract uint64 yang_node_get_module(uint64 ynode);
    public abstract uint64 yang_node_get_module_orig(uint64 ynode);
    public abstract bool yang_node_matches_prefix(uint64 ynode, owned string prefix_string);
 // public abstract uint32 yang_node_parse_value(uint64 ynode, owned string value_string, out uint64 str_matched);
 // public abstract void yang_node_set_mode_path(uint64 ynode, owned string path ??);
    public abstract bool yang_node_is_mode_path(uint64 ynode);
    public abstract bool yang_node_is_rpc(uint64 ynode);
    public abstract bool yang_node_is_rpc_input(uint64 ynode);

    // YangKey C APIs
    public abstract uint64 yang_key_get_list_node(uint64 ykey);
    public abstract uint64 yang_key_get_key_node(uint64 ykey);
    public abstract uint64 yang_key_get_next_key(uint64 ykey);

    // YangAugment C APIs
    // TBD

    // YangType C APIs
    public abstract void yang_type_get_location(uint64 ytype, out string str);
    public abstract string? yang_type_get_description(uint64 ytype);
    public abstract string? yang_type_get_help_short(uint64 ytype);
    public abstract string? yang_type_get_help_full(uint64 ytype);
    public abstract string? yang_type_get_prefix(uint64 ytype);
    public abstract string? yang_type_get_ns(uint64 ytype);
    public abstract uint64 yang_type_get_leaf_type(uint64 ytype);
    public abstract uint64 yang_type_get_first_value(uint64 ytype);
 // public abstract uint64 yang_type_value_begin(uint64 ytype);
 // public abstract uint64 yang_type_value_end(uint64 ytype);
 // public abstract uint32 yang_type_parse_value(uint64 ynode, owned string value_string, out uint64 str_matched);
    public abstract uint32 yang_type_count_matches(uint64 ytype, owned string value_string);
    public abstract uint32 yang_type_count_partials(uint64 ytype, owned string value_string);
    public abstract uint64 yang_type_get_first_extension(uint64 ytype);
 // public abstract uint64 yang_type_extension_begin(uint64 ytype);
 // public abstract uint64 yang_type_extension_end(uint64 ytype);

    // YangValue C APIs
    public abstract string? yang_value_get_name(uint64 yvalue);
    public abstract void yang_value_get_location(uint64 yvalue, out string str);
    public abstract string? yang_value_get_description(uint64 yvalue);
    public abstract string? yang_value_get_help_short(uint64 yvalue);
    public abstract string? yang_value_get_help_full(uint64 yvalue);
    public abstract string? yang_value_get_prefix(uint64 yvalue);
    public abstract string? yang_value_get_ns(uint64 yvalue);
    public abstract uint64 yang_value_get_leaf_type(uint64 yvalue);
    public abstract uint64 yang_value_get_max_length(uint64 yvalue);
    public abstract uint64 yang_value_get_next_value(uint64 yvalue);
    public abstract uint32 yang_value_parse_value(uint64 yvalue, owned string value_string);
    public abstract uint32 yang_value_parse_partial(uint64 yvalue, owned string value_string);
    public abstract bool yang_value_is_keyword(uint64 yvalue);
    public abstract uint64 yang_value_get_first_extension(uint64 yvalue);
 // public abstract uint64 yang_value_extension_begin(uint64 yvalue);
 // public abstract uint64 yang_value_extension_end(uint64 yvalue);

    // YangExtension C APIs
    public abstract void yang_extension_get_location(uint64 yext, out string str);
    public abstract void yang_extension_get_location_ext(uint64 yext, out string str);
    public abstract string? yang_extension_get_value(uint64 yext);
    public abstract string? yang_extension_get_description_ext(uint64 yext);
    public abstract string? yang_extension_get_name(uint64 yext);
    public abstract string? yang_extension_get_module_ext(uint64 yext);
    public abstract string? yang_extension_get_prefix(uint64 yext);
    public abstract string? yang_extension_get_ns(uint64 yext);
    public abstract uint64 yang_extension_get_next_extension(uint64 yext);
    public abstract uint64 yang_extension_search(uint64 yext, owned string module, owned string ext);
    public abstract bool yang_extension_is_match(uint64 yext, owned string module, owned string ext);
 */
  }

}
