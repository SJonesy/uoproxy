/*
 * uoproxy
 *
 * Copyright 2005-2020 Max Kellermann <max.kellermann@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * Collect all objects which have to be flushed, wait until
 * flush_end() is called an then flush them all at once.
 *
 * This is useful for buffering data being sent.
 */

#ifndef __UOPROXY_FLUSH_H
#define __UOPROXY_FLUSH_H

#include "util/IntrusiveList.hxx"

/**
 * An operation that has to be flushed.
 */
class PendingFlush : public IntrusiveListHook {
    bool is_linked = false;

public:
    PendingFlush() = default;

    ~PendingFlush() noexcept {
        CancelFlush();
    }

    PendingFlush(const PendingFlush &) = delete;
    PendingFlush &operator=(const PendingFlush &) = delete;

    virtual void DoFlush() noexcept = 0;

protected:
    void ScheduleFlush() noexcept;

    void CancelFlush() noexcept {
        if (is_linked) {
            is_linked = false;
            unlink();
        }
    }
};

void
flush_begin();

void
flush_end();

class ScopeLockFlush {
public:
    ScopeLockFlush() noexcept {
        flush_begin();
    }

    ~ScopeLockFlush() noexcept {
        flush_end();
    }

    ScopeLockFlush(const ScopeLockFlush &) = delete;
    ScopeLockFlush &operator=(const ScopeLockFlush &) = delete;
};

#endif
