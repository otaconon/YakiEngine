#pragma once

#include <deque>
#include <functional>

#include "Buffer.h"

class DeletionQueue
{
public:
    DeletionQueue();

    void PushFunction(std::function<void()>&& function);
    void PushBuffer(Buffer&& buffer);
    void Flush();

private:
    std::deque<std::function<void()>> m_deletors;
    std::deque<Buffer> m_buffers;

};
