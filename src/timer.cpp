#include "Arduino.h"

#include "debug.h"
#include "timer.h"

Timer::~Timer() {
    if (_entries == nullptr) return;

    delete[] _entries;
    _entries = nullptr;

    _count = 0;
    _free_count = 0;
}

unsigned long Timer::add_timeout(TimerFn callback, unsigned long interval, void *parameter) {
    return _add(callback, interval, false, parameter);
}

void Timer::clear_timeout(unsigned long timer_id) {
    _clear(timer_id);
}

unsigned long Timer::add_interval(TimerFn callback, unsigned long interval, void *parameter) {
    return _add(callback, interval, true, parameter);
}

void Timer::clear_interval(unsigned long timer_id) {
    _clear(timer_id);
}

void Timer::handle_timers() {
    if (_entries == nullptr || _count == _free_count) return;

    const auto now = millis();
    for (unsigned long i = 0; i < _count; ++i) {
        auto &entry = _entries[i];
        if (!entry.active || (now - entry.created_at) < entry.interval) continue;

#ifdef DEBUG
        Serial.print("Call timer: ");
        Serial.println(i);
#endif

        entry.callback(entry.parameter);
        if (entry.repeat) {
            entry.created_at = now;
        } else {
            _clear(i);
        }
    }
}

unsigned long Timer::_add(TimerFn callback, unsigned long interval, bool repeat, void *parameter) {
    if (_free_count == 0) _grow();

    for (unsigned long i = 0; i < _count; ++i) {
        auto &entry = _entries[i];
        if (entry.active) continue;

        entry.active = true;
        entry.created_at = millis();
        entry.interval = interval;
        entry.repeat = repeat;
        entry.callback = callback;
        entry.parameter = parameter;

        _free_count--;

#ifdef DEBUG
        Serial.print("Add timer: ");
        Serial.print(i);
        Serial.print(". Used: ");
        Serial.print(_free_count);
        Serial.print(" / ");
        Serial.println(_count);
#endif

        return i;
    }

    // We shouldn't be here
    return -1ul;
}

void Timer::_clear(unsigned long timer_id) {
    auto &entry = _entries[timer_id];
    if (!entry.active) return;

    entry = TimerEntry();
    _free_count++;

#ifdef DEBUG
    Serial.print("Remove timer: ");
    Serial.print(timer_id);
    Serial.print(". Used: ");
    Serial.print(_free_count);
    Serial.print(" / ");
    Serial.println(_count);
#endif
}

void Timer::_grow() {
    const unsigned long new_count = _count + GROW_AMOUNT;
    TimerEntry *new_data = new TimerEntry[new_count];

    if (_entries != nullptr) {
        for (unsigned long i = 0; i < _count; ++i) {
            new_data[i] = _entries[i];
        }

        delete[] _entries;
    }

#ifdef DEBUG
    Serial.print("Grow timer memory from ");
    Serial.print(_count);
    Serial.print(" to ");
    Serial.println(new_count);
#endif

    _entries = new_data;
    _count = new_count;
    _free_count += GROW_AMOUNT;
}
