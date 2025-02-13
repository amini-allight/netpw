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
#ifndef NETPW_RING_BUFFER_H
#define NETPW_RING_BUFFER_H

struct lockfree_spsc_queue;

int lockfree_spsc_queue_init(struct lockfree_spsc_queue** queue);
void lockfree_spsc_queue_destroy(struct lockfree_spsc_queue* queue);
int lockfree_spsc_queue_pull(struct lockfree_spsc_queue* queue, unsigned char* data, int size);
int lockfree_spsc_queue_push(struct lockfree_spsc_queue* queue, const unsigned char* data, int size);

#endif
