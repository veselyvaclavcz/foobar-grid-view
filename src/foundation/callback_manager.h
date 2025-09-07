#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "lifecycle_manager.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <Windows.h>

namespace albumart_grid {

// Forward declarations
class callback_token;

// Callback types
enum class callback_type {
    playlist,
    library,
    playback,
    metadb,
    ui_status
};

// Safe callback execution wrapper
template<typename Func>
void safe_callback_execute(const char* callback_name, Func&& func) {
    // Check shutdown state first
    if (LifecycleManager::instance().is_shutting_down()) {
        return;
    }
    
    __try {
        func();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        FB2K_console_formatter() << "Album Art Grid: Exception in callback: " << callback_name;
        LifecycleManager::instance().set_error_state(true);
    }
}

// Base class for callback handlers
class callback_handler_base : public validated_object {
public:
    virtual ~callback_handler_base() = default;
    virtual callback_type get_type() const = 0;
};

// Callback manager - handles all SDK callbacks safely
class CallbackManager : public ILifecycleAware {
public:
    // Get singleton instance
    static CallbackManager& instance();
    
    // Delete copy/move constructors
    CallbackManager(const CallbackManager&) = delete;
    CallbackManager& operator=(const CallbackManager&) = delete;
    CallbackManager(CallbackManager&&) = delete;
    CallbackManager& operator=(CallbackManager&&) = delete;
    
    // ILifecycleAware implementation
    void on_initialize() override;
    void on_shutdown() override;
    bool is_valid() const override;
    
    // Register a callback handler
    callback_token register_callback(std::weak_ptr<callback_handler_base> handler);
    
    // Unregister a callback
    void unregister(const callback_token& token);
    
    // Unregister all callbacks for an object
    void unregister_all_for_object(void* object);
    
    // Execute callback safely on a handler
    template<typename Handler, typename Func>
    void execute_on_handler(std::weak_ptr<Handler> weak_handler, 
                           const char* callback_name,
                           Func&& func) {
        safe_callback_execute(callback_name, [&]() {
            if (auto handler = weak_handler.lock()) {
                if (handler->is_valid() && !is_shutting_down()) {
                    func(handler.get());
                }
            }
        });
    }
    
    // Get all handlers of a specific type
    template<typename Handler>
    std::vector<std::weak_ptr<Handler>> get_handlers(callback_type type) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::weak_ptr<Handler>> result;
        
        for (auto it = m_handlers.begin(); it != m_handlers.end();) {
            if (it->second.expired()) {
                // Clean up expired handlers
                it = m_handlers.erase(it);
            } else {
                if (auto handler = it->second.lock()) {
                    if (handler->get_type() == type) {
                        result.push_back(std::static_pointer_cast<Handler>(handler));
                    }
                }
                ++it;
            }
        }
        
        return result;
    }
    
private:
    CallbackManager();
    ~CallbackManager();
    
    bool is_shutting_down() const {
        return m_shutting_down.load(std::memory_order_acquire);
    }
    
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shutting_down{false};
    std::atomic<uint64_t> m_next_token_id{1};
    
    mutable std::mutex m_mutex;
    std::unordered_map<uint64_t, std::weak_ptr<callback_handler_base>> m_handlers;
    std::unordered_map<void*, std::vector<uint64_t>> m_object_tokens;
};

// Token for callback registration
class callback_token {
public:
    callback_token() : m_id(0), m_valid(false) {}
    explicit callback_token(uint64_t id) : m_id(id), m_valid(true) {}
    
    uint64_t id() const { return m_id; }
    bool is_valid() const { return m_valid; }
    
    void invalidate() { m_valid = false; }
    
private:
    uint64_t m_id;
    bool m_valid;
};

// Template for specific callback handlers
template<typename SDKCallback>
class callback_handler : public callback_handler_base, public SDKCallback {
public:
    explicit callback_handler(callback_type type) : m_type(type) {}
    
    callback_type get_type() const override { return m_type; }
    
protected:
    callback_type m_type;
};

} // namespace albumart_grid