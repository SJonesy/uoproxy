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

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "util/IntrusiveList.hxx"
#include "PacketStructs.hxx"
#include "World.hxx"
#include "Client.hxx"
#include "StatefulClient.hxx"

#include <event.h>

#define MAX_WALK_QUEUE 4

struct Instance;
struct Connection;
struct LinkedServer;

namespace UO {
class Client;
class Server;
}

struct WalkState {
    struct Item {
        /**
         * The walk packet sent by the client.
         */
        struct uo_packet_walk packet;

        /**
         * The walk sequence number which was sent to the server.
         */
        uint8_t seq;
    };

    LinkedServer *server = nullptr;
    Item queue[MAX_WALK_QUEUE];
    unsigned queue_size = 0;
    uint8_t seq_next = 0;
};

struct Connection final : IntrusiveListHook, UO::ClientHandler {
    Instance &instance;

    /* flags */
    const bool background;

    /* reconnect */
    const bool autoreconnect;

    /* client stuff (= connection to server) */

    StatefulClient client;

    /**
     * A timer which is used to schedule a reconnect.
     */
    struct event reconnect_event;

    /* state */
    UO::CredentialsFragment credentials{};

    unsigned server_index = 0;

    unsigned character_index = 0;

    WalkState walk;

    /* sub-objects */

    IntrusiveList<LinkedServer> servers;

    Connection(Instance &_instance, bool _background,
               bool _autoreconnect)
        :instance(_instance), background(_background),
         autoreconnect(_autoreconnect)
    {
        evtimer_set(&reconnect_event, ReconnectTimerCallback, this);
    }

    ~Connection() noexcept;

    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    void Destroy() noexcept {
        unlink();
        delete this;
    }

    bool IsInGame() const noexcept {
        return client.IsInGame();
    }

    bool CanAttach() const noexcept {
        return IsInGame() && client.char_list;
    }

    /**
     * @return 0 on success or an errno value
     */
    int Connect(const struct sockaddr *server_address,
                size_t server_address_length,
                uint32_t seed);
    void Disconnect() noexcept;
    void Reconnect();
    void ScheduleReconnect() noexcept;

    void Add(LinkedServer &ls) noexcept;
    void Remove(LinkedServer &ls) noexcept;

    /**
     * Remove the specified #LinkedServer and check if this is the
     * last connection from a client; if so, it may destroy the whole
     * #Connection.
     */
    void RemoveCheckEmpty(LinkedServer &ls) noexcept;

    void BroadcastToInGameClients(const void *data, size_t length) noexcept;
    void BroadcastToInGameClientsExcept(const void *data, size_t length,
                                        LinkedServer &except) noexcept;
    void BroadcastToInGameClientsDivert(enum protocol_version new_protocol,
                                        const void *old_data, size_t old_length,
                                        const void *new_data, size_t new_length) noexcept;

    LinkedServer *FindZombie(const struct uo_packet_game_login &game_login) noexcept;

    void ClearWorld() noexcept {
        DeleteItems();
        DeleteMobiles();
    }

    void DeleteItems() noexcept;
    void DeleteMobiles() noexcept;

private:
    void DoReconnect() noexcept;
    static void ReconnectTimerCallback(int, short, void *ctx) noexcept;

    /* virtual methods from UO::ClientHandler */
    bool OnClientPacket(const void *data, size_t length) override;
    void OnClientDisconnect() noexcept override;
};

int connection_new(Instance *instance,
                   int server_socket,
                   Connection **connectionp);

void connection_speak_console(Connection *c, const char *msg);

/* walk */

void
connection_walk_server_removed(WalkState &state,
                               LinkedServer &ls) noexcept;

void
connection_walk_request(LinkedServer &ls,
                        const struct uo_packet_walk &p);

void
connection_walk_cancel(Connection &c,
                       const struct uo_packet_walk_cancel &p);

void
connection_walk_ack(Connection &c,
                    const struct uo_packet_walk_ack &p);

/* attach */

void
attach_send_world(LinkedServer *ls);

/* command */

void
connection_handle_command(LinkedServer &ls, const char *command);

#endif
