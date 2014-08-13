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

#ifndef __LNS_PROGRAM_H__
#define __LNS_PROGRAM_H__

#include "process.h"

namespace LNSTools {

class ProgramPrivate;

class LNS_EXPORT Program : public Process {
private:
    ProgramPrivate *m_d;
    inline ProgramPrivate *d();
    inline const ProgramPrivate *d() const;
    Program(const Program&) = delete;
    inline std::string&& _to_str(std::string &&v);
    inline std::string _to_str(std::string &v);
    template<int idx=0, typename T2, typename... T>
    inline void _set_args(std::vector<std::string>&, T2&&, T&&...) noexcept;
    template<int idx=0, typename T2>
    inline void _set_args(std::vector<std::string>&, T2&&) noexcept;
public:
    Program(std::string&&);
    inline Program(std::string&);
    inline Program(Program &&) noexcept;
    ~Program() noexcept override;

    std::vector<std::string> &args() noexcept;
    template<typename... T> inline void set_args(T&&... v) noexcept;

    std::function<void()> &pre_exec() noexcept;
};

inline std::string
Program::_to_str(std::string &v)
{
    return std::string(v);
}

inline std::string&&
Program::_to_str(std::string &&v)
{
    return std::move(v);
}

template<typename... T>
inline void
Program::set_args(T&&... v) noexcept
{
    size_t len = sizeof...(v);
    auto &_args = args();
    _args.resize(len);
    _set_args(_args, std::forward<T>(v)...);
}

template<int idx, typename T2, typename... T>
inline void
Program::_set_args(std::vector<std::string> &_args, T2 &&v2, T&&... v) noexcept
{
    _args[idx] = _to_str(v2);
    _set_args<idx + 1>(_args, std::forward<T>(v)...);
}

template<int idx, typename T2>
inline void
Program::_set_args(std::vector<std::string> &_args, T2 &&v2) noexcept
{
    _args[idx] = _to_str(v2);
}

inline
Program::Program(std::string &file)
    : Program(std::string(file))
{
}

inline
Program::Program(Program &&other) noexcept
    : Process(std::move(other)), m_d(other.m_d)
{
    other.m_d = nullptr;
}

inline ProgramPrivate*
Program::d()
{
    return m_d;
}

inline const ProgramPrivate*
Program::d() const
{
    return m_d;
}

}

#endif
