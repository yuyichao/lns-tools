/***************************************************************************
 *   Copyright (C) 2014~2014 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "process.h"

#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <signal.h>
#include <sched.h>
#include <unistd.h>

#include <fstream>

namespace LNSTools {

class ProcessPrivate {
    int flags;
    int extra_flags;
    int pid;
    std::function<int()> func;
    UserMap uid_map;
    UserMap gid_map;
    inline
    ProcessPrivate(std::function<int()> &&f) noexcept
        : flags(SIGCHLD), extra_flags(0), pid(-1), func(f), uid_map(), gid_map()
    {
    }
    friend class Process;
};

Process::Process(std::function<int()> &&f) noexcept
    : m_d(new ProcessPrivate(std::move(f)))
{
}

int
Process::extra_flags() const noexcept
{
    return m_d->extra_flags;
}

void
Process::set_extra_flags(int flags) noexcept
{
    d()->extra_flags = flags;
}

int
Process::pid() const noexcept
{
    return d()->pid;
}

int
Process::flags() const noexcept
{
    return d()->extra_flags | d()->flags;
}

int
Process::run()
{
    struct rlimit rlp;
    if (lns_unlikely(getrlimit(RLIMIT_STACK, &rlp) == -1)) {
        rlp.rlim_cur = 8 * 1024 * 1024;
    }

    struct CloneData {
        Process *const that;
        int fds[2];
    } data = {this, {0, 0}};
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, data.fds) == -1) {
        return -errno;
    }
    if ((setsockopt(data.fds[0], SOL_SOCKET, SO_RCVTIMEO,
                    (char*)&tv, sizeof(tv)) == -1) ||
        (setsockopt(data.fds[1], SOL_SOCKET, SO_RCVTIMEO,
                    (char*)&tv, sizeof(tv)) == -1)) {
        close(data.fds[0]);
        close(data.fds[1]);
        return -errno;
    }

    void *stack = mmap(nullptr, rlp.rlim_cur, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN,
                       -1, 0);
    if (!stack) {
        close(data.fds[0]);
        close(data.fds[1]);
        return -errno;
    }

    int _flags = flags();
    d()->pid = clone([] (void *p) -> int {
            auto &data = *reinterpret_cast<CloneData*>(p);
            auto &func = data.that->d()->func;
            data.that->post_clone_child(data.fds);
            return func();
        }, (char*)stack + rlp.rlim_cur, _flags, &data);

    if (d()->pid == -1) {
        int err = errno;
        close(data.fds[0]);
        close(data.fds[1]);
        munmap(stack, rlp.rlim_cur);
        return -err;
    }
    if (!(_flags & CLONE_VM)) {
        munmap(stack, rlp.rlim_cur);
    }
    post_clone_parent(data.fds);
    if (!(_flags & CLONE_FILES)) {
        close(data.fds[0]);
        close(data.fds[1]);
    }
    return d()->pid;
}

static void
write_map(const UserMap &map, const std::string &file)
{
    std::fstream stm(file, std::ios_base::out);
    for (const auto &item: map) {
        stm << item.child << " " << item.parent << " "
            << item.length << std::endl;
    }
}

void
Process::post_clone_parent(int fds[2]) noexcept
{
    if (userns() && (!d()->uid_map.empty() || !d()->gid_map.empty())) {
        auto proc = "/proc/" + std::to_string(d()->pid);
        if (!d()->uid_map.empty()) {
            write_map(d()->uid_map, proc + "/uid_map");
        }
        if (!d()->gid_map.empty()) {
            write_map(d()->gid_map, proc + "/gid_map");
        }
    }

    char buf = 0;
    read(fds[0], &buf, 1);
    write(fds[0], &buf, 1);
}

void
Process::post_clone_child(int fds[2]) noexcept
{
    char buf = 0;
    write(fds[1], &buf, 1);
    read(fds[1], &buf, 1);
    close(fds[1]);
    close(fds[0]);
}

int
Process::signal() const noexcept
{
    return d()->flags & CSIGNAL;
}

void
Process::set_signal(int s) noexcept
{
    d()->flags = (d()->flags & (~CSIGNAL)) | (s & CSIGNAL);
}

bool
Process::userns() const noexcept
{
    return d()->flags & CLONE_NEWUSER;
}

void
Process::set_userns(bool user) noexcept
{
    d()->flags = (d()->flags & (~CLONE_NEWUSER)) | (user ? CLONE_NEWUSER : 0);
}

Process::~Process() noexcept
{
    if (m_d) {
        delete m_d;
    }
}

UserMap&
Process::uid_map() noexcept
{
    return d()->uid_map;
}

UserMap&
Process::gid_map() noexcept
{
    return d()->gid_map;
}

int
Process::wait(int *status, int options)
{
    int pid = d()->pid;
    if (lns_unlikely(pid == -1)) {
        errno = EINVAL;
        return -1;
    }
    return waitpid(d()->pid, status, options);
}

}
