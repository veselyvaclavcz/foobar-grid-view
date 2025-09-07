#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>

namespace albumart_grid {

// Magic number for object validation
constexpr uint64_t GRID_MAGIC_VALID = 0x474F4F44424144ULL;  // "GOODBAD" in hex
constexpr uint64_t GRID_MAGIC_INVALID = 0xDEADBEEFDEADBEEFULL;

// Forward declarations
class ILifecycleAware;
class validated_object;

// Lifecycle manager - singleton that coordinates component lifecycle
class LifecycleManager {
public:
    // Get singleton instance
    static LifecycleManager& instance();
    
    // Delete copy/move constructors
    LifecycleManager(const LifecycleManager&) = delete;
    LifecycleManager& operator=(const LifecycleManager&) = delete;
    LifecycleManager(LifecycleManager&&) = delete;
    LifecycleManager& operator=(LifecycleManager&&) = delete;
    
    // Initialize the lifecycle manager
    bool initialize();
    
    // Register a module for lifecycle management
    // priority: Higher priority modules are shutdown first
    void register_module(ILifecycleAware* module, int priority = 0);
    
    // Unregister a module
    void unregister_module(ILifecycleAware* module);
    
    // Begin shutdown sequence
    void begin_shutdown();
    
    // Check if shutting down
    bool is_shutting_down() const {
        return m_shutting_down.load(std::memory_order_acquire);
    }
    
    // Validate object is alive and safe to use
    bool validate_object(const validated_object* obj) const;
    
    // Set/get global error state
    void set_error_state(bool error) {
        m_has_error.store(error, std::memory_order_release);
    }
    
    bool has_error() const {
        return m_has_error.load(std::memory_order_acquire);
    }
    
private:
    LifecycleManager();
    ~LifecycleManager();
    
    struct ModuleEntry {
        ILifecycleAware* module;
        int priority;
        
        bool operator<(const ModuleEntry& other) const {
            return priority > other.priority; // Higher priority first
        }
    };
    
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shutting_down{false};
    std::atomic<bool> m_has_error{false};
    
    mutable std::mutex m_modules_mutex;
    std::vector<ModuleEntry> m_modules;
};

// Interface for modules that need lifecycle management
class ILifecycleAware {
public:
    virtual ~ILifecycleAware() = default;
    
    // Called during initialization
    virtual void on_initialize() = 0;
    
    // Called during shutdown
    virtual void on_shutdown() = 0;
    
    // Check if object is valid
    virtual bool is_valid() const = 0;
};

// Base class for validated objects with magic number protection
class validated_object {
public:
    validated_object() 
        : m_magic(GRID_MAGIC_VALID)
        , m_destroyed(false) {
    }
    
    virtual ~validated_object() {
        invalidate();
    }
    
    // Check if object is valid
    bool is_valid() const {
        // Quick checks first
        if (m_destroyed.load(std::memory_order_acquire)) {
            return false;
        }
        
        if (m_magic.load(std::memory_order_acquire) != GRID_MAGIC_VALID) {
            return false;
        }
        
        // SEH protection for memory corruption detection
        __try {
            volatile uint64_t test = m_magic.load(std::memory_order_acquire);
            return (test == GRID_MAGIC_VALID);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    
    // Invalidate the object
    void invalidate() {
        m_destroyed.store(true, std::memory_order_release);
        m_magic.store(GRID_MAGIC_INVALID, std::memory_order_release);
    }
    
protected:
    mutable std::atomic<uint64_t> m_magic;
    mutable std::atomic<bool> m_destroyed;
};

// RAII helper for lifecycle-aware objects
template<typename T>
class lifecycle_ptr {
public:
    explicit lifecycle_ptr(T* ptr = nullptr) : m_ptr(ptr) {
        if (m_ptr) {
            LifecycleManager::instance().register_module(m_ptr, 0);
        }
    }
    
    ~lifecycle_ptr() {
        reset();
    }
    
    lifecycle_ptr(const lifecycle_ptr&) = delete;
    lifecycle_ptr& operator=(const lifecycle_ptr&) = delete;
    
    lifecycle_ptr(lifecycle_ptr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }
    
    lifecycle_ptr& operator=(lifecycle_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }
    
    void reset(T* ptr = nullptr) {
        if (m_ptr) {
            LifecycleManager::instance().unregister_module(m_ptr);
            delete m_ptr;
        }
        m_ptr = ptr;
        if (m_ptr) {
            LifecycleManager::instance().register_module(m_ptr, 0);
        }
    }
    
    T* get() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }
    
private:
    T* m_ptr;
};

} // namespace albumart_grid