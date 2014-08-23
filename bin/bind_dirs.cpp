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

#include <lns_tools/program.h>
#include <sys/stat.h>
#include <assert.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>

template<typename... Ts>
static inline void
exit_error(const char *fmt, Ts&&... args)
{
    fprintf(stderr, fmt, std::forward<Ts>(args)...);
    exit(-1);
}

static void
make_dir(const char *name)
{
    struct stat stat_buf;
    if (lstat(name, &stat_buf) == 0) {
        if (!S_ISDIR(stat_buf.st_mode)) {
            exit_error("bind_dirs: path '%s' already exist and is "
                       "not a directory.\n", name);
        }
    } else if (mkdir(name, 0750) == -1) {
        assert_perror(errno);
    }
}

int
main(int argc, char **argv)
{
    LNSTools::MountMap mount_map;
    char *file = nullptr;
    bool mkdir_opt = false;

    int i = 1;
    for (;i < argc;i++) {
        if (strcmp(argv[i], "-m") == 0) {
            if (i + 2 >= argc) {
                exit_error("bind_dirs: not enough arguments for -m.\n");
            }
            mount_map.push_back(argv[i + 1], argv[i + 2]);
            i += 2;
        } else if (strcmp(argv[i], "--arg0") == 0) {
            if (i + 1 >= argc) {
                exit_error("bind_dirs: not enough arguments for --arg0.\n");
            }
            i++;
            file = argv[i];
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                exit_error("bind_dirs: not enough arguments for --arg0.\n");
            }
            mkdir_opt = true;
        } else {
            break;
        }
    }
    char **args = (i >= argc) ? &file : (argv + i);
    if (lns_unlikely(!args || !args[0])) {
        exit_error("bind_dirs: no command to execute.\n");
    }
    if (!file) {
        file = args[0];
    }

    LNSTools::Program p(file);
    if (geteuid() != 0) {
        p.set_userns(true);
        auto uid = getuid();
        auto gid = getgid();
        p.add_uid_map(uid, uid);
        p.add_gid_map(gid, gid);
    }
    for (int j = i;j < argc;j++) {
        p.args().push_back(argv[j]);
    }

    p.chroot("/");
    if (mkdir_opt) {
        for (auto &mount: mount_map) {
            make_dir(mount.parent.c_str());
            make_dir(mount.child.c_str());
        }
    }
    p.mount_map() = std::move(mount_map);

    int pid = p.run();
    assert_perror(pid <= 0 ? errno : 0);
    int status = 0;
    p.wait(&status);
    assert_perror(status);
    return 0;
}
