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

#include "Connection.hxx"
#include "LinkedServer.hxx"
#include "PacketStructs.hxx"
#include "Log.hxx"

#include <assert.h>

void
Connection::Add(LinkedServer &ls) noexcept
{
    assert(ls.connection == nullptr);

    servers.push_front(ls);
    ls.connection = this;
}

void
Connection::Remove(LinkedServer &ls) noexcept
{
    assert(ls.connection == this);

    connection_walk_server_removed(walk, ls);

    ls.connection = nullptr;
    ls.unlink();
}

void
Connection::RemoveCheckEmpty(LinkedServer &ls) noexcept
{
    Remove(ls);

    if (!servers.empty()) {
        ls.LogF(2, "client disconnected, server connection still in use");
    } else if (background && client.IsConnected() && client.IsInGame()) {
        ls.LogF(1, "client disconnected, backgrounding");
    } else {
        ls.LogF(1, "last client disconnected, removing connection");
        Destroy();
    }
}

LinkedServer *
Connection::FindZombie(const struct uo_packet_game_login &game_login) noexcept
{
    if (game_login.credentials != credentials)
        return nullptr;

    for (auto &i : servers) {
        if (i.IsZombie() && i.auth_id == game_login.auth_id)
            return &i;
    }

    return nullptr;
}
