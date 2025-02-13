/*
Copyright 2025 Amini Allight

This file is part of netpw.

netpw is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

netpw is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with netpw. If not, see <https://www.gnu.org/licenses/>.
*/
#include "constants.h"

#include <boost/lockfree/spsc_queue.hpp>

extern "C" {
#include "lockfree_spsc_queue.h"

struct lockfree_spsc_queue
{
    boost::lockfree::spsc_queue<unsigned char, boost::lockfree::capacity<NETPW_QUEUE_SIZE>> queue;
};

int lockfree_spsc_queue_init(struct lockfree_spsc_queue** queue)
{
    *queue = new lockfree_spsc_queue;

    return *queue == nullptr;
}

void lockfree_spsc_queue_destroy(struct lockfree_spsc_queue* queue)
{
    delete queue;
}

int lockfree_spsc_queue_pull(struct lockfree_spsc_queue* queue, unsigned char* data, int size)
{
    return queue->queue.pop(data, size);
}

int lockfree_spsc_queue_push(struct lockfree_spsc_queue* queue, const unsigned char* data, int size)
{
    return queue->queue.push(data, size);
}

}
