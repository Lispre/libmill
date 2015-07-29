/*

  Copyright (c) 2015 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../libmill.h"

void client(const char *addr) {
    unixsock cs = unixconnect(addr, -1);
    assert(cs);

    msleep(now() + 100);

    char buf[16];
    size_t sz = unixrecv(cs, buf, 3, -1);
    assert(sz == 3 && buf[0] == 'A' && buf[1] == 'B' && buf[2] == 'C');

    sz = unixsend(cs, "123\n45\n6789", 11, -1);
    assert(sz == 11 && errno == 0);
    unixflush(cs, -1);
    assert(errno == 0);

    unixclose(cs);
}

int main() {
    const char *sockname = "milltest.sock";
    char buf[16];
    struct stat st;

    if (stat(sockname, &st) == 0) {
        assert(unlink(sockname) == 0);
    }

    unixsock ls = unixlisten(sockname);
    assert(ls);

    go(client(sockname));

    unixsock as = unixaccept(ls, -1);

    /* Test deadline. */
    int64_t deadline = now() + 30;
    size_t sz = unixrecv(as, buf, sizeof(buf), deadline);
    assert(sz == 0 && errno == ETIMEDOUT);
    int64_t diff = now() - deadline;
    assert(diff > -10 && diff < 10);

    sz = unixsend(as, "ABC", 3, -1);
    assert(sz == 3 && errno == 0);
    unixflush(as, -1);
    assert(errno == 0);

    sz = unixrecvuntil(as, buf, sizeof(buf), '\n', -1);
    assert(sz == 4);
    assert(buf[0] == '1' && buf[1] == '2' && buf[2] == '3' && buf[3] == '\n');
    sz = unixrecvuntil(as, buf, sizeof(buf), '\n', -1);
    assert(sz == 3);
    assert(buf[0] == '4' && buf[1] == '5' && buf[2] == '\n');
    sz = unixrecvuntil(as, buf, 3, '\n', -1);
    assert(sz == 3);
    assert(buf[0] == '6' && buf[1] == '7' && buf[2] == '8');

    unixclose(as);
    unixclose(ls);
    unlink(sockname);

    return 0;
}

