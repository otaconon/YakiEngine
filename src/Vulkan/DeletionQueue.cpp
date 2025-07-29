#include "DeletionQueue.h"

DeletionQueue::DeletionQueue() {}

void DeletionQueue::PushFunction(std::function<void()>&& function) {
    m_deletors.push_back(function);
}

void DeletionQueue::PushBuffer(Buffer&& buffer)
{
    m_buffers.push_back(std::move(buffer));
}

void DeletionQueue::Flush() {
    m_buffers.clear();

    for (auto it = m_deletors.rbegin(); it != m_deletors.rend(); ++it)
        (*it)();

    m_deletors.clear();
}
