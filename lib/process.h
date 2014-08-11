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

#ifndef __LNS_PROCESS_H__
#define __LNS_PROCESS_H__

#include "utils.h"
#include <functional>
#include <vector>

namespace LNSTools {

struct MapRange {
    unsigned parent;
    unsigned child;
    unsigned length;
};

class LNS_EXPORT UserMap: public std::vector<MapRange> {
public:
    using std::vector<MapRange>::vector;
};

class ProcessPrivate;

class LNS_EXPORT Process {
private:
    ProcessPrivate *m_d;
    inline ProcessPrivate *d();
    inline const ProcessPrivate *d() const;
    Process(const Process&) = delete;
public:
    Process(std::function<int()>&&) noexcept;
    inline Process(std::function<int()>&) noexcept;
    inline Process(Process &&) noexcept;
    ~Process() noexcept;

    int extra_flags() const noexcept;
    void set_extra_flags(int) noexcept;
    int flags() const noexcept;

    int run();
    int pid() const noexcept;

    int signal() const noexcept;
    void set_signal(int) noexcept;

    bool userns() const noexcept;
    void set_userns(bool) noexcept;

    UserMap &uid_map() noexcept;
    UserMap &gid_map() noexcept;
private:
    void post_clone_parent(int fds[2]) noexcept;
    void post_clone_child(int fds[2]) noexcept;
};

inline
Process::Process(Process &&other) noexcept
{
    m_d = other.m_d;
    other.m_d = nullptr;
}

inline ProcessPrivate*
Process::d()
{
    return m_d;
}

inline const ProcessPrivate*
Process::d() const
{
    return m_d;
}

inline
Process::Process(std::function<int()> &func) noexcept
    : Process(std::move(std::function<int()>(func)))
{
}

}

#endif
