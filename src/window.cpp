#include "window.h"

Window::Window(long size, long chunk_size) : _window_size(size), _chunk_size(chunk_size) {
    resize(size);
}

Window::~Window() {
    delete[] _chunks;
}

void Window::update(long active_sec) {
    const auto now = (long) millis() / 1000;
    if (_current_chunk_time == -1l) {
        _current_chunk_time = now;
    }

    const auto chunk_time = now - _current_chunk_time;
    if (chunk_time >= _chunk_size) {
        _shift_chunk(chunk_time / _chunk_size);
        _current_chunk_time = now;
    }

    if (active_sec <= 0) return;

    const auto prev_chunk_time = _chunks[_current_chunk];
    _chunks[_current_chunk] += active_sec;
    if (_chunks[_current_chunk] > _chunk_size) {
        _chunks[_current_chunk] = _chunk_size;
    }

    _accumulated_time += _chunks[_current_chunk] - prev_chunk_time;
}

void Window::resize(long new_size) {
    long new_count = new_size / _chunk_size;
    if (new_size % _chunk_size) ++new_count;

    if (new_count == _count)
        return;

    auto *new_chunks = new window_t[new_count];
    memset(new_chunks, 0, new_count * CHUNK_T_SIZE);

    if (_chunks != nullptr) {
        const long leading_count = std::max(0l, std::min(new_count, _current_chunk + 1));
        const long following_count = std::max(0l, std::min(new_count - leading_count, _count - leading_count));
        const long empty_space = std::max(0l, new_count - _count);

        // Current element
        new_chunks[0] = _chunks[_current_chunk];

        // Following elements
        memcpy(new_chunks + empty_space + 1, // Skip first
               _chunks + (_count - following_count),
               following_count * CHUNK_T_SIZE);

        // Leading elements without current
        memcpy(new_chunks + empty_space + following_count + 1,
               _chunks + (_current_chunk + 1 - leading_count),
               (leading_count - 1) * CHUNK_T_SIZE);

        delete[] _chunks;
    }

    _current_chunk = 0;
    _count = new_count;
    _chunks = new_chunks;
    _window_size = new_size;
    _accumulated_time = std::min(_accumulated_time, _window_size * _chunk_size);
}

void Window::_shift_chunk(long chunk_shift) {
    if (chunk_shift >= _count) {
        _accumulated_time = 0;
        memset(_chunks, 0, _count * CHUNK_T_SIZE);
        _current_chunk = 0;
        return;
    }

    long cnt = 0;
    while (cnt < chunk_shift) {
        int index = (_current_chunk + cnt + 1) % _count;

        _accumulated_time -= _chunks[index];
        _chunks[index] = 0;

        ++cnt;
    }

    _current_chunk = (_current_chunk + chunk_shift) % _count;
}