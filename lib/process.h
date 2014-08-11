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
#include <string>

namespace LNSTools {

struct MapRange {
    unsigned child;
    unsigned parent;
    unsigned length;
    MapRange(unsigned _child=0,
             unsigned _parent=0,
             unsigned _length=1)
        : child(_child), parent(_parent), length(_length)
    {}
};

typedef std::vector<MapRange> UserMap;

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
    int wait(int *status=nullptr, int options=0);

    int signal() const noexcept;
    void set_signal(int) noexcept;

    bool userns() const noexcept;
    void set_userns(bool) noexcept;
    UserMap &uid_map() noexcept;
    UserMap &gid_map() noexcept;
    template<typename... T> inline void add_uid_map(T&&... v);
    template<typename... T> inline void add_gid_map(T&&... v);

    bool mountns() const noexcept;
    void set_mountns(bool) noexcept;
private:
    void post_clone_parent(int fds[2]) noexcept;
    void post_clone_child(int fds[2]) noexcept;
};

template<typename... T>
inline void
Process::add_uid_map(T&&... v)
{
    uid_map().push_back(MapRange(std::forward<T>(v)...));
}

template<>
inline void
Process::add_uid_map<const MapRange&>(const MapRange &v)
{
    uid_map().push_back(v);
}

template<typename... T>
inline void
Process::add_gid_map(T&&... v)
{
    gid_map().push_back(MapRange(std::forward<T>(v)...));
}

template<>
inline void
Process::add_gid_map<const MapRange&>(const MapRange &v)
{
    gid_map().push_back(v);
}

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
