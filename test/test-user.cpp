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

#include <lns_tools/process.h>
#include <assert.h>
#include <sched.h>
#include <unistd.h>

int
main()
{
    LNSTools::Process p([] () {
            assert(getuid() == 0);
            assert(getgid() == 0);
            return 0;
        });
    p.set_userns(true);
    p.add_uid_map(0, getuid());
    p.add_gid_map(0, getgid());
    int pid = p.run();
    assert_perror(pid <= 0 ? errno : 0);
    int status = 0;
    p.wait(&status);
    assert_perror(status);
    return 0;
}
