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

#include "program.h"

#include <unistd.h>

namespace LNSTools {

class ProgramPrivate {
    std::string m_file;
    std::vector<std::string> m_args;
    std::function<void()> m_pre_exec;
    inline
    ProgramPrivate(std::string &&file) noexcept
        : m_file(std::move(file)), m_args(), m_pre_exec([] {})
    {}
    int clone_cb();
    friend class Program;
};

int
ProgramPrivate::clone_cb()
{
    const size_t size = m_args.size();
    std::vector<const char*> argv(size + 1);
    for (size_t i = 0;i < size;i++) {
        argv[i] = m_args[i].c_str();
    }
    m_pre_exec();
    execvp(m_file.c_str(), const_cast<char *const*>(argv.data()));
    return errno;
}

Program::Program(std::string &&file)
    : Process([=] () {
            return d()->clone_cb();
        }), m_d(new ProgramPrivate(std::move(file)))
{
}

Program::~Program() noexcept
{
    if (m_d) {
        delete m_d;
    }
}

std::vector<std::string>&
Program::args() noexcept
{
    return d()->m_args;
}

std::function<void()>&
Program::pre_exec() noexcept
{
    return d()->m_pre_exec;
}

}
