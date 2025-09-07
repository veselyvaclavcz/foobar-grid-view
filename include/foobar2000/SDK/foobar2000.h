#pragma once
// Minimal foobar2000 SDK header for compilation

#include <Windows.h>
#include <string>
#include <vector>

// Basic types
typedef unsigned int t_uint32;
typedef int t_int32;
typedef size_t t_size;

// Service base
class service_base {
public:
    virtual int service_add_ref() = 0;
    virtual int service_release() = 0;
};

// Service pointer
template<typename T>
class service_ptr_t {
    T* p;
public:
    service_ptr_t() : p(nullptr) {}
    service_ptr_t(T* _p) : p(_p) { if (p) p->service_add_ref(); }
    ~service_ptr_t() { if (p) p->service_release(); }
    T* operator->() { return p; }
    T* get_ptr() { return p; }
    bool is_valid() { return p != nullptr; }
};

// UI element
class ui_element_instance : public service_base {
public:
    virtual HWND get_wnd() = 0;
    virtual void set_configuration(const void* data, t_size size) = 0;
    virtual void get_configuration(void* data, t_size size) = 0;
};

class ui_element : public service_base {
public:
    virtual const char* get_name() = 0;
    virtual const char* get_description() = 0;
    virtual ui_element_instance* instantiate(HWND parent, const void* data, t_size size) = 0;
};

// Metadb handle
class metadb_handle {
public:
    virtual void dummy() = 0;
};

typedef service_ptr_t<metadb_handle> metadb_handle_ptr;

// Metadb handle list
class metadb_handle_list {
public:
    virtual t_size get_count() const = 0;
    virtual metadb_handle_ptr get_item(t_size index) const = 0;
};

// Album art
namespace album_art_ids {
    static const GUID cover_front = {0,0,0,{0,0,0,0,0,0,0,0}};
}

class album_art_data : public service_base {
public:
    virtual const void* get_ptr() const = 0;
    virtual t_size get_size() const = 0;
};

class album_art_extractor_instance : public service_base {
public:
    virtual album_art_data* query(const GUID& what, class abort_callback& abort) = 0;
};

class album_art_manager_v2 : public service_base {
public:
    virtual album_art_extractor_instance* open(metadb_handle_list const& items, 
                                               const GUID* what, t_size count,
                                               class abort_callback& abort) = 0;
};

// Abort callback
class abort_callback {
public:
    virtual bool is_aborting() const = 0;
};

class abort_callback_dummy : public abort_callback {
public:
    bool is_aborting() const { return false; }
};

// Console
class console_formatter {
public:
    console_formatter& operator<<(const char* str) { return *this; }
    console_formatter& operator<<(int val) { return *this; }
};

inline console_formatter FB2K_console_formatter() { return console_formatter(); }

// Init/quit
class initquit : public service_base {
public:
    virtual void on_init() = 0;
    virtual void on_quit() = 0;
};

// Component version
#define DECLARE_COMPONENT_VERSION(NAME, VER, INFO)
#define VALIDATE_COMPONENT_FILENAME(NAME)

// Service factory
template<typename T>
class service_factory_single_t {
public:
    service_factory_single_t() {}
};

template<typename T>
class initquit_factory_t {
public:
    initquit_factory_t() {}
};

template<typename T>
class ui_element_impl {
public:
    static const char* g_get_name() { return "Album Art Grid"; }
    static const char* g_get_description() { return "Display albums as grid"; }
};

// Static API
template<typename T>
class static_api_ptr_t {
public:
    static_api_ptr_t() {}
    T* operator->() { return nullptr; }
};

// PFC
namespace pfc {
    template<typename T>
    class list_single_ref_t {
    public:
        list_single_ref_t(const T& item) {}
    };
}
