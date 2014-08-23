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
#include <sys/mount.h>

#include <signal.h>
#include <sched.h>
#include <unistd.h>

#include <fstream>

namespace LNSTools {

class ProcessPrivate {
    int m_flags;
    int m_extra_flags;
    int m_pid;
    std::function<int()> m_func;
    UserMap m_uids;
    UserMap m_gids;
    std::string m_chroot;
    std::string m_chdir;
    MountMap m_mounts;
    inline
    ProcessPrivate(std::function<int()> &&f) noexcept :
        m_flags(SIGCHLD), m_extra_flags(0), m_pid(-1), m_func(std::move(f)),
        m_uids(), m_gids(), m_chroot(), m_chdir(), m_mounts()
    {
    }
    int post_clone_child() noexcept;
    int post_clone_parent() noexcept;
    friend class Process;
};

Process::Process(std::function<int()> &&f) noexcept
    : m_d(new ProcessPrivate(std::move(f)))
{
}

int
Process::extra_flags() const noexcept
{
    return m_d->m_extra_flags;
}

void
Process::set_extra_flags(int flags) noexcept
{
    d()->m_extra_flags = flags;
}

int
Process::pid() const noexcept
{
    return d()->m_pid;
}

int
Process::flags() const noexcept
{
    return d()->m_extra_flags | d()->m_flags;
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
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, data.fds) == -1) {
        return -errno;
    }
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
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
    auto &fds = data.fds;
    d()->m_pid = clone([] (void *p) -> int {
            auto &data = *reinterpret_cast<CloneData*>(p);
            auto &func = data.that->d()->m_func;
            // needed for CLONE_VM, address of data.fds might become invalid
            // after the parent function returns. This can happen anywhere
            // after the write() below.
            int fds[2] = {data.fds[0], data.fds[1]};
            int err = data.that->d()->post_clone_child();
            // send error to parent
            write(fds[1], &err, sizeof(int));
            if (err == 0) {
                // read error from parent
                err = 1;
                read(fds[1], &err, sizeof(int));
            }
            close(fds[1]);
            close(fds[0]);
            if (err) {
                return -1;
            }
            return func();
        }, (char*)stack + rlp.rlim_cur, _flags, &data);

    if (d()->m_pid == -1) {
        int err = errno;
        close(fds[0]);
        close(fds[1]);
        munmap(stack, rlp.rlim_cur);
        return -err;
    }
    if (!(_flags & CLONE_VM)) {
        munmap(stack, rlp.rlim_cur);
    }
    int err = d()->post_clone_parent();
    int child_err = 1;
    if (!(_flags & CLONE_FILES)) {
        close(data.fds[1]);
    }
    read(fds[0], &child_err, sizeof(int));
    write(fds[0], &err, sizeof(int));
    if (!(_flags & CLONE_FILES)) {
        close(data.fds[0]);
    }
    if (!(err || child_err))
        return d()->m_pid;
    d()->m_pid = -1;
    errno = err ? err : child_err;
    return -1;
}

LNS_EXPORT int
write_user_map(const UserMap &map, const std::string &file)
{
    FILE *stm = fopen(file.c_str(), "w");
    if (!stm)
        return errno;
    for (const auto &item: map) {
        if (fprintf(stm, "%u %u %u\n", item.child,
                    item.parent, item.length) < 0 ||
            fflush(stm) < 0) {
            int err = errno;
            fclose(stm);
            return err;
        }
    }
    fclose(stm);
    return 0;
}

int
ProcessPrivate::post_clone_parent() noexcept
{
    if ((m_flags & CLONE_NEWUSER) && (!m_uids.empty() || !m_gids.empty())) {
        auto proc = "/proc/" + std::to_string(m_pid);
        if (!m_uids.empty()) {
            if (int err = write_user_map(m_uids, proc + "/uid_map")) {
                return err;
            }
        }
        if (!m_gids.empty()) {
            if (int err = write_user_map(m_gids, proc + "/gid_map")) {
                return err;
            }
        }
    }
    return 0;
}

int
ProcessPrivate::post_clone_child() noexcept
{
    if (!m_chroot.empty()) {
        for (auto &m: m_mounts) {
            auto &child = *(m.child.empty() ? &m.parent : &m.child);
            if (mount(m.parent.c_str(), (m_chroot + '/' + child).c_str(),
                      nullptr, MS_BIND, nullptr) == -1) {
                return errno;
            }
        }
        if (chdir(m_chroot.c_str()) == -1 || ::chroot(".") == -1) {
            return errno;
        }
    }
    if (!m_chdir.empty()) {
        if (chdir(m_chdir.c_str()) == -1) {
            return errno;
        }
    }
    return 0;
}

int
Process::signal() const noexcept
{
    return d()->m_flags & CSIGNAL;
}

void
Process::set_signal(int s) noexcept
{
    d()->m_flags = (d()->m_flags & (~CSIGNAL)) | (s & CSIGNAL);
}

bool
Process::userns() const noexcept
{
    return d()->m_flags & CLONE_NEWUSER;
}

void
Process::set_userns(bool user) noexcept
{
    d()->m_flags = (d()->m_flags & (~CLONE_NEWUSER)) | (user ? CLONE_NEWUSER : 0);
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
    return d()->m_uids;
}

UserMap&
Process::gid_map() noexcept
{
    return d()->m_gids;
}

int
Process::wait(int *status, int options)
{
    int pid = d()->m_pid;
    if (lns_unlikely(pid == -1)) {
        errno = EINVAL;
        return -1;
    }
    return waitpid(pid, status, options);
}

const std::string&
Process::new_root() const
{
    return d()->m_chroot;
}

const std::string&
Process::new_curdir() const
{
    return d()->m_chdir;
}

void
Process::chroot(std::string &&root)
{
    d()->m_flags = ((d()->m_flags & (~CLONE_NEWNS)) |
                    (root.empty() ? 0 : CLONE_NEWNS));
    d()->m_chroot = std::move(root);
}

void
Process::chdir(std::string &&dir)
{
    d()->m_chdir = std::move(dir);
}

MountMap&
Process::mount_map() noexcept
{
    return d()->m_mounts;
}

}
