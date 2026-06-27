#pragma once
#include <list>
#include <unordered_map>
namespace Cubed {
template <typename T> class RecentQueue {
private:
    std::list<T> m_list;
    std::unordered_map<T, typename std::list<T>::iterator> m_map;

public:
    void enqueue(T key) {
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            m_list.splice(m_list.end(), m_list, it->second);
            return;
        }
        m_list.emplace_back(std::move(key));
        auto iter = std::prev(m_list.end());
        m_map.emplace(*iter, iter);
    }

    void pop() {
        if (m_list.empty()) {
            return;
        }
        m_map.erase(m_list.front());
        m_list.pop_front();
    }
    void clear() {
        m_list.clear();
        m_map.clear();
    }

    const T& front() const {
        assert(!empty());
        return m_list.front();
    }
    const T& back() const {
        assert(!empty());
        return m_list.back();
    }
    [[nodiscard]]
    bool empty() const {
        return m_list.empty();
    }
    [[nodiscard]]
    size_t size() const {
        return m_list.size();
    }
    [[nodiscard]]
    bool contains(const T& key) const {
        return m_map.find(key) != m_map.end();
    }
};
} // namespace Cubed