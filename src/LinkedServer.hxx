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

#pragma once

#include "Server.hxx"
#include "CVersion.hxx"
#include "util/Compiler.h"
#include "util/IntrusiveList.hxx"

#include <event.h>

#include <cstdint>

struct Connection;

namespace UO {
class Server;
}

/**
 * This object manages one connection from a UO client to uoproxy
 * (where uoproxy acts as a UO "server", therefore the class name).
 * It may be one of of several clients sharing the connection to the
 * real UO server.
 */
struct LinkedServer final : IntrusiveListHook, UO::ServerHandler {
    Connection *connection = nullptr;

    UO::Server *server = nullptr;

    ClientVersion client_version;

    struct event zombie_timeout; /**< zombies time out and auto-reap themselves
                                    after 5 seconds using this timer */

    /**
     * Identifier for this object in log messages.
     */
    const unsigned id;

    static unsigned id_counter;

    uint32_t auth_id; /**< unique identifier for this linked_server used in
                           redirect handling to locate the zombied
                           linked_server */

    bool welcome = false;

    enum class State : uint8_t {
        /**
         * The initial state, nothing has been received yet.  We're
         * waiting for AccountLogin or GameLogin.
         */
        INIT,

        /**
         * We have received AccountLogin from this client, and now
         * we're waiting for a server list from the real login server.
         * As soon as we receive it, we forward it to this client, and
         * the state changes to #SERVER_LIST.
         */
        ACCOUNT_LOGIN,

        /**
         * We have sent the server list to this client, and we're
         * waiting for PlayServer from this client.
         */
        SERVER_LIST,

        /**
         * We have received PlayServer from this client, and now we're
         * waiting for a character list from the real game server.  As
         * soon as we receive it, we forward it to this client, and
         * the state changes to #CHAR_LIST.
         */
        PLAY_SERVER,

        /**
         * We have sent RelayServer to this client, and we expecting it to
         * close the connection and create a new one to us.  When the
         * connection is closed, this object remains for a few seconds, as
         * a placeholder for the new connection.
         */
        RELAY_SERVER,

        /**
         * We have received GameLogin from this client, and now we're
         * waiting for a character list from the real game server.  As
         * soon as we receive it, we forward it to this client, and
         * the state changes to #CHAR_LIST.
         */
        GAME_LOGIN,

        /**
         * We have sent the character list to this client, and we're
         * waiting for PlayCharacter from this client.
         */
        CHAR_LIST,

        /**
         * We have received PlayCharacter from this client, and we're
         * waiting for the real game server to send world data to us.
         */
        PLAY_CHAR,

        /**
         * This client has received world data, and is playing.
         */
        IN_GAME,
    } state = State::INIT;

    explicit LinkedServer(int fd)
        :server(uo_server_create(fd, *this)),
         id(++id_counter)
    {
        evtimer_set(&zombie_timeout, ZombieTimeoutCallback, this);
    }

    ~LinkedServer() noexcept;

    LinkedServer(const LinkedServer &) = delete;
    LinkedServer &operator=(const LinkedServer &) = delete;

    bool IsZombie() const noexcept {
        return state == State::RELAY_SERVER && server == nullptr;
    }

    /**
     * Can we forward in-game packets to the client connected to this
     * object?
     */
    bool IsInGame() const noexcept {
        return state == State::IN_GAME;
    }

    gcc_printf(3, 4)
    void LogF(unsigned level, const char *fmt, ...) noexcept;

private:
    static void ZombieTimeoutCallback(int, short, void *ctx) noexcept;

    /* virtual methods from UO::ServerHandler */
    bool OnServerPacket(const void *data, size_t length) override;
    void OnServerDisconnect() noexcept override;
};
