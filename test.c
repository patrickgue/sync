#include "test.h"

#include "sync.h"

#include <assert.h>
#include <stdio.h>
void test_endianness()
{
    long a = 0x1234567890abcdef;
    long b = 0xefcdab9078563412;

    int c = 0x12345678;
    int d = 0x78563412;

    assert(a == swap_endianness_long(b));
    assert(c == swap_endianness_int(d));

}




void test()
{
    test_endianness();
}
