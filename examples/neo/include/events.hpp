/* ===================================================================
 * © Copyright 2025 - Ross Robotics. All rights reserved.
 * Unauthorised duplicating, while flattering, is strictly prohibited.
 * Our robots are watching. 🤖
 *
 * Device: MK4 PTU Interconnect
 * Software Version: 1.0
 * File: include/events.hpp
 * ===================================================================
 */

#pragma once

#include <any>
#include <cstdint>
#include <optional>
#include <queue>
#include <type_traits>
#include <vector>

enum class NotificationBarEventType
{
    kNone = 0,
    kSetModeOff,
    kSetModeBootingDown,
    kSetModeCharging,
    kSetModeOperational,
    kSetModeAutonomous
};

template <typename EventType, typename DataType = std::any>
struct Event
{
    EventType type         = static_cast<EventType>(0);
    uint64_t  timestamp_us = 0;
    uint64_t  deadline_us  = 0;
    DataType  data         = {};
};

template <typename EventType, typename DataType = std::any>
struct EventComparator
{
    bool operator()(Event<EventType, DataType> const& a, Event<EventType, DataType> const& b) const
    {
        return a.timestamp_us > b.timestamp_us;
    }
};

template <typename EventType, typename DataType = std::any, typename Comparator = EventComparator<EventType, DataType>>
class EventQueue
{
public:
    static constexpr std::size_t DEFAULT_MAX_EVENTS = 128;

    explicit EventQueue(std::size_t max_events = DEFAULT_MAX_EVENTS) : queue_capacity_(max_events) {}

    void push(Event<EventType, DataType> ev)
    {
        if (queue_.size() < queue_capacity_)
        {
            queue_.push(std::move(ev));
        }
    }

    std::optional<Event<EventType, DataType>> pop_front()
    {
        if (queue_.empty())
        {
            return std::nullopt;
        }

        auto ev = queue_.top();
        queue_.pop();
        return ev;
    }

    std::size_t size() const { return queue_.size(); }
    bool        empty() const { return queue_.empty(); }

private:
    std::size_t queue_capacity_;
    std::priority_queue<Event<EventType, DataType>, std::vector<Event<EventType, DataType>>, Comparator> queue_;
};