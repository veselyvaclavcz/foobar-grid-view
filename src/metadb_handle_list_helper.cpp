// Implementation of metadb_handle_list_helper functions without threading
// Based on SDK metadb_handle_list.cpp but with threading functions removed

#include "foobar2000-sdk-pch.h"
#include "titleformat.h"
#include "library_manager.h"
#include <vector>

namespace {
    struct custom_sort_data {
        pfc::string8_fastalloc text;
        size_t index;
    };
    
    template<int direction>
    static int custom_sort_compare(const custom_sort_data& elem1, const custom_sort_data& elem2) {
        int ret = direction * pfc::stringCompareCaseInsensitive(elem1.text, elem2.text);
        if (ret == 0) ret = pfc::sgn_t((t_ssize)elem1.index - (t_ssize)elem2.index);
        return ret;
    }

    class tfhook_simple : public titleformat_hook {
    public:
        bool process_field(titleformat_text_out *, const char *, t_size, bool &) override {
            return false;
        }
        bool process_function(titleformat_text_out *, const char *, t_size, titleformat_hook_function_params *, bool &) override {
            return false;
        }
    };
}

void metadb_handle_list_helper::sort_by_format(metadb_handle_list_ref p_list, const char * spec, titleformat_hook * p_hook) {
    service_ptr_t<titleformat_object> script;
    if (titleformat_compiler::get()->compile(script, spec))
        sort_by_format(p_list, script, p_hook);
}

void metadb_handle_list_helper::sort_by_format_get_order(metadb_handle_list_cref p_list, t_size* order, const char * spec, titleformat_hook * p_hook) {
    service_ptr_t<titleformat_object> script;
    if (titleformat_compiler::get()->compile(script, spec))
        sort_by_format_get_order(p_list, order, script, p_hook);
}

void metadb_handle_list_helper::sort_by_format(metadb_handle_list_ref p_list, const service_ptr_t<titleformat_object> & p_script, titleformat_hook * p_hook, int direction) {
    const t_size count = p_list.get_count();
    pfc::array_t<t_size> order; order.set_size(count);
    sort_by_format_get_order(p_list, order.get_ptr(), p_script, p_hook, direction);
    p_list.reorder(order.get_ptr());
}

void metadb_handle_list_helper::sort_by_format_get_order(metadb_handle_list_cref p_list, t_size* order, const service_ptr_t<titleformat_object> & p_script, titleformat_hook * p_hook, int p_direction) {
    const t_size count = p_list.get_count();
    if (count == 0) return;
    
    // Initialize order array
    for (t_size n = 0; n < count; n++) {
        order[n] = n;
    }
    
    std::vector<custom_sort_data> data;
    data.resize(count);
    
    tfhook_simple defaultHook;
    if (!p_hook) p_hook = &defaultHook;
    
    pfc::string8_fastalloc temp;
    temp.prealloc(512);
    
    // Single-threaded formatting (no threading functions)
    for (t_size n = 0; n < count; n++) {
        metadb_handle_ptr item;
        p_list.get_item_ex(item, n);
        item->format_title(p_hook, temp, p_script, NULL);
        data[n].text = temp;
        data[n].index = order[n];
    }
    
    // Sort the data
    if (p_direction > 0) {
        pfc::sort_t(data, custom_sort_compare<1>, count);
    } else {
        pfc::sort_t(data, custom_sort_compare<-1>, count);
    }
    
    // Extract the order
    for (t_size n = 0; n < count; n++) {
        order[n] = data[n].index;
    }
}

void metadb_handle_list_helper::sort_by_relative_path(metadb_handle_list_ref p_list) {
    const t_size count = p_list.get_count();
    pfc::array_t<t_size> order; order.set_size(count);
    sort_by_relative_path_get_order(p_list, order.get_ptr());
    p_list.reorder(order.get_ptr());
}

void metadb_handle_list_helper::sort_by_relative_path_get_order(metadb_handle_list_cref p_list, t_size* order) {
    const t_size count = p_list.get_count();
    std::vector<custom_sort_data> data;
    data.resize(count);
    auto api = library_manager::get();
    
    pfc::string8_fastalloc temp;
    temp.prealloc(512);
    for(t_size n = 0; n < count; n++) {
        metadb_handle_ptr item;
        p_list.get_item_ex(item, n);
        if (!api->get_relative_path(item, temp)) temp = "";
        data[n].index = n;
        data[n].text = temp;
    }

    pfc::sort_t(data, custom_sort_compare<1>, count);
    
    for(t_size n = 0; n < count; n++) {
        order[n] = data[n].index;
    }
}

void metadb_handle_list_helper::remove_duplicates(metadb_handle_list_ref p_list) {
    t_size count = p_list.get_count();
    if (count > 0) {
        pfc::bit_array_bittable mask(count);
        pfc::array_t<t_size> order; order.set_size(count);
        order_helper::g_fill(order);

        p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>, order.get_ptr());
        
        bool found = false;
        for(t_size n = 0; n < count - 1; n++) {
            if (p_list.get_item(order[n]) == p_list.get_item(order[n+1])) {
                found = true;
                mask.set(order[n+1], true);
            }
        }
        
        if (found) p_list.remove_mask(mask);
    }
}

void metadb_handle_list_helper::sort_by_pointer_remove_duplicates(metadb_handle_list_ref p_list) {
    const t_size count = p_list.get_count();
    if (count > 0) {
        sort_by_pointer(p_list);
        bool b_found = false;
        for(size_t n = 0; n < count - 1; n++) {
            if (p_list.get_item(n) == p_list.get_item(n+1)) {
                b_found = true;
                break;
            }
        }

        if (b_found) {
            pfc::bit_array_bittable mask(count);
            for(size_t n = 0; n < count - 1; n++) {
                if (p_list.get_item(n) == p_list.get_item(n+1))
                    mask.set(n+1, true);
            }
            p_list.remove_mask(mask);
        }
    }
}

void metadb_handle_list_helper::sort_by_path_quick(metadb_handle_list_ref p_list) {
    p_list.sort_t(metadb::path_compare_metadb_handle);
}

void metadb_handle_list_helper::sort_by_pointer(metadb_handle_list_ref p_list) {
    p_list.sort();
}

t_size metadb_handle_list_helper::bsearch_by_pointer(metadb_handle_list_cref p_list, const metadb_handle_ptr & val) {
    t_size blah;
    if (p_list.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>, val, blah)) return blah;
    else return SIZE_MAX;
}

double metadb_handle_list_helper::calc_total_duration(metadb_handle_list_cref p_list) {
    double ret = 0;
    for (auto handle : p_list) {
        double temp = handle->get_length();
        if (temp > 0) ret += temp;
    }
    return ret;
}

void metadb_handle_list_helper::sort_by_path(metadb_handle_list_ref p_list) {
    sort_by_format(p_list, "%path_sort%", NULL);
}

t_filesize metadb_handle_list_helper::calc_total_size(metadb_handle_list_cref p_list, bool skipUnknown) {
    pfc::avltree_t< const char*, metadb::path_comparator > beenHere;
    t_filesize ret = 0;
    t_size n, m = p_list.get_count();
    for(n = 0; n < m; n++) {
        bool isNew;
        metadb_handle_ptr h; p_list.get_item_ex(h, n);
        beenHere.add_ex(h->get_path(), isNew);
        if (isNew) {
            t_filesize t = h->get_filesize();
            if (t == filesize_invalid) {
                if (!skipUnknown) return filesize_invalid;
            } else {
                ret += t;
            }
        }
    }
    return ret;
}

t_filesize metadb_handle_list_helper::calc_total_size_ex(metadb_handle_list_cref p_list, bool & foundUnknown) {
    foundUnknown = false;
    metadb_handle_list list(p_list);
    list.sort_t(metadb::path_compare_metadb_handle);

    t_filesize ret = 0;
    t_size n, m = list.get_count();
    for(n = 0; n < m; n++) {
        if (n == 0 || metadb::path_compare(list[n-1]->get_path(), list[n]->get_path())) {
            t_filesize t = list[n]->get_filesize();
            if (t == filesize_invalid) {
                foundUnknown = true;
            } else {
                ret += t;
            }
        }
    }
    return ret;
}

pfc::string8 metadb_handle_list_helper::format_total_size(metadb_handle_list_cref p_list) {
    pfc::string8 temp;
    bool unknown = false;
    t_filesize val = calc_total_size_ex(p_list, unknown);
    if (unknown) temp << "> ";
    temp << pfc::format_file_size_short(val);
    return temp;
}

bool metadb_handle_list_helper::extract_folder_path(metadb_handle_list_cref list, pfc::string_base & folderOut) {
    const t_size total = list.get_count();
    if (total == 0) return false;
    pfc::string_formatter temp, folder;
    folder = list[0]->get_path();
    folder.truncate_to_parent_path();
    for(size_t walk = 1; walk < total; ++walk) {
        temp = list[walk]->get_path();
        temp.truncate_to_parent_path();
        if (metadb::path_compare(folder, temp) != 0) return false;
    }
    folderOut = folder;
    return true;
}

bool metadb_handle_list_helper::extract_single_path(metadb_handle_list_cref list, const char * &pathOut) {
    const t_size total = list.get_count();
    if (total == 0) return false;
    const char * path = list[0]->get_path();
    for(t_size walk = 1; walk < total; ++walk) {
        if (metadb::path_compare(path, list[walk]->get_path()) != 0) return false;
    }
    pathOut = path;
    return true;
}