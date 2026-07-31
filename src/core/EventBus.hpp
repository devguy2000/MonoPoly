#pragma once
#include <any>
#include <functional>
#include <mutex>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>

// Thread-safe event bus.
// Emit() dispatches immediately on the calling thread.
// EmitDeferred() queues for dispatch on the main thread via FlushDeferred().
class EventBus {
public:
    template<typename EventT, typename HandlerFn>
    void Subscribe(HandlerFn&& handler) {
        auto key = std::type_index(typeid(EventT));
        m_handlers[key].emplace_back(
            [h = std::forward<HandlerFn>(handler)](const std::any& e) {
                h(std::any_cast<const EventT&>(e));
            }
        );
    }

    template<typename EventT>
    void Emit(EventT&& event) {
        auto key = std::type_index(typeid(std::decay_t<EventT>));
        auto it  = m_handlers.find(key);
        if (it == m_handlers.end()) return;
        std::any wrapped = std::forward<EventT>(event);
        for (auto& h : it->second)
            h(wrapped);
    }

    // Safe to call from worker threads – dispatches on next FlushDeferred().
    template<typename EventT>
    void EmitDeferred(EventT&& event) {
        std::lock_guard lock(m_queueMutex);
        m_queue.emplace([this, e = std::forward<EventT>(event)]() mutable {
            Emit(std::move(e));
        });
    }

    // Post an arbitrary callable to run on the main thread on next FlushDeferred().
    // The cleanest way for worker threads to invoke main-thread code without
    // defining a one-off event type.
    void PostToMainThread(std::function<void()> fn) {
        std::lock_guard lock(m_queueMutex);
        m_queue.push(std::move(fn));
    }

    // Must be called from the main thread each frame.
    void FlushDeferred() {
        std::queue<std::function<void()>> local;
        {
            std::lock_guard lock(m_queueMutex);
            std::swap(local, m_queue);
        }
        while (!local.empty()) {
            local.front()();
            local.pop();
        }
    }

    static EventBus& Get() {
        static EventBus s_instance;
        return s_instance;
    }

private:
    using Handler = std::function<void(const std::any&)>;
    std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;
    std::mutex                              m_queueMutex;
    std::queue<std::function<void()>>       m_queue;
};
