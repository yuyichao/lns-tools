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
#include <sys/capability.h>
#include <assert.h>
#include <sched.h>
#include <unistd.h>

static void
make_dir(const char *name)
{
    struct stat stat_buf;
    if (lstat(name, &stat_buf) == 0) {
        if (!S_ISDIR(stat_buf.st_mode)) {
            fprintf(stderr,
                    "path '%s' already exist and is not a directory.", name);
            exit(-1);
        }
    } else if (mkdir(name, 0750) == -1) {
        assert_perror(errno);
    }
}

int
main2(int top_uid, int top_gid)
{
    int overflow_uid = getuid();
    int overflow_gid = getgid();
    int status = LNSTools::write_user_map(LNSTools::MapRange(0, top_uid),
                                          "/proc/self/uid_map");
    assert_perror(status ? errno : 0);
    status = LNSTools::write_user_map(LNSTools::MapRange(0, top_gid),
                                      "/proc/self/gid_map");
    assert_perror(status ? errno : 0);
    assert_perror(seteuid(1000) ? errno : 0);

    printf("uid: %ld\n", (long)getuid());
    printf("gid: %ld\n", (long)getgid());
    printf("eUID = %ld;  eGID = %ld;\n", (long)geteuid(), (long)getegid());
    cap_t cap = cap_get_proc();
    char *cap_txt = cap_to_text(cap, NULL);
    cap_free(cap);
    printf("capabilities: %s\n", cap_txt);
    cap_free(cap_txt);

    LNSTools::Program p("ls");
    p.set_args("ls", "-AlF", "--color", "/");

    p.set_userns(true);
    // p.add_uid_map(1000, 0);
    // p.add_gid_map(1000, 0);
    p.add_uid_map(2048, 1048);
    p.add_gid_map(2048, 1048);
    // p.add_uid_map(0, overflow_uid);
    // p.add_gid_map(0, overflow_gid);

    p.chroot(".");
    make_dir("usr");
    make_dir("lib");
    make_dir("lib64");
    p.add_mount_map("/usr");
    p.add_mount_map("/lib");
    p.add_mount_map("/lib64");

    int pid = p.run();
    assert_perror(pid <= 0 ? errno : 0);
    status = 0;
    p.wait(&status);
    assert_perror(status);
    return 0;
}

int
main()
{
    int top_uid = getuid();
    int top_gid = getgid();
    LNSTools::Process p([=] () {
            return main2(top_uid, top_gid);
        });
    p.set_userns(true);
    // p.add_uid_map(0, getuid());
    // p.add_gid_map(0, getgid());

    int pid = p.run();
    assert_perror(pid <= 0 ? errno : 0);
    int status = 0;
    p.wait(&status);
    assert_perror(status);
    return 0;
}
