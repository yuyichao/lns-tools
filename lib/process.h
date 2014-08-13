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

struct BindMount {
    std::string parent;
    std::string child;
    template<typename Tp> inline BindMount(Tp &&_parent);
    template<typename Tp, typename Tc>
    inline BindMount(Tp &&_parent, Tc &&_child);
};

template<typename Tp> inline
BindMount::BindMount(Tp &&_parent)
    : parent(std::forward<Tp>(_parent)), child()
{
}

template<typename Tp, typename Tc> inline
BindMount::BindMount(Tp &&_parent, Tc &&_child)
    : parent(std::forward<Tp>(_parent)), child(std::forward<Tc>(_child))
{
}

typedef std::vector<BindMount> MountMap;

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
    virtual ~Process() noexcept;

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

    const std::string& new_root() const;
    inline void chroot(std::string&);
    void chroot(std::string&&);
    MountMap &mount_map() noexcept;
    template<typename... T> inline void add_mount_map(T&&... v);
};

inline void
Process::chroot(std::string &root)
{
    chroot(std::move(std::string(root)));
}

template<typename... T>
inline void
Process::add_mount_map(T&&... v)
{
    mount_map().push_back(BindMount(std::forward<T>(v)...));
}

template<>
inline void
Process::add_mount_map<const BindMount&>(const BindMount &v)
{
    mount_map().push_back(v);
}

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
    : m_d(other.m_d)
{
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

int write_user_map(const UserMap &map, const std::string &file);

static inline int
write_user_map(const MapRange &range, const std::string &file)
{
    return write_user_map(UserMap({range}), file);
}

}

#endif
