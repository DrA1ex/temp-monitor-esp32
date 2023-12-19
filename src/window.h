#pragma once

#include "Arduino.h"

#include <memory.h>
#include <algorithm>
#include <type_traits>

typedef long window_t;

class Window {
    const unsigned int CHUNK_T_SIZE = sizeof(window_t);

    long _window_size;
    long _chunk_size;

    long _current_chunk = 0;
    long _count = 0;
    window_t *_chunks = nullptr;

    window_t _accumulated_time = 0;
    long _current_chunk_time = -1;

public:
    explicit Window(long size = 0, long chunk_size = 60);

    ~Window();

    inline int window_size() { return _window_size; }
    inline int chunk_size() { return _chunk_size; }
    inline window_t accumulated_time() { return _accumulated_time; }

    void update(window_t active_sec = 0);
    void resize(long new_size);

    void print_debug() {
        Serial.print("Chunks (" + String(_current_chunk) + "): ");
        for (int i = 0; i < _count; ++i) {
            Serial.print(_chunks[i]);
            Serial.print(" ");
        }

        Serial.println();
    }

private:
    void _shift_chunk(long chunk_shift);
};