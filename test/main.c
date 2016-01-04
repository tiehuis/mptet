/* Test tetris functions for accuracy */

#include <assert.h>
#include <stdio.h>
#include <string.h>

mpstate ms;
int errors = 0;

enum {
    I_, T_, L_, J_, S_, Z_, O_
};

void set_layout(mpstate *ms, int id, int br, const char *layout)
{
    mem256_zero(&ms->field);
    mem256_zero(&ms->block);

    size_t llen = strlen(layout) - 1;

    for (size_t i = 0; i < llen; ++i) {
        switch (layout[llen - i]) {
        case '#':
            mem256_set(&ms->field, i);
            break;
        case 'X':
            mem256_set(&ms->field, i);
            /* Fallthrough */
        case 'x':
            ms->id = id;
            ms->br = br;
            ms->bx = 8 - (i % 10);
            ms->by = i / 10;
            ms->block.limb[0] = mptetd_block[ms->id][ms->br];
            mem256_bshift(&ms->block, -ms->bx + 10 * (ms->by - 3));
            break;
        case 'o':
        default:
            break;
        }
    }
}

void print_layout(const char *layout, size_t len)
{
    for (size_t i = 0; i <= len; ++i) {
        fprintf(stderr, "%c", layout[i]);
        if ((len - i) % 10 == 0)
            fprintf(stderr, "\n");
    }
}

void print_limb(mpstate *ms, int upper)
{
    for (int i = upper; i >= 0; --i) {
        if (mem256_get(&ms->field, i))
            fprintf(stderr, "#");
        else if (mem256_get(&ms->block, i))
            fprintf(stderr, "o");
        else
            fprintf(stderr, " ");

        if ((i % 10) == 0)
            fprintf(stderr, "\n");
    }
}

bool assert_layout(mpstate *ms, const char *layout)
{
    int failure = 0;

    size_t llen = strlen(layout) - 1;

    for (size_t i = 0; i < llen; ++i) {
        const int x = 8 - (i % 10);
        const int y = i / 10;

        switch (layout[llen - i]) {
        case '#':
            if (!mem256_get(&ms->field, i)) {
                fprintf(stderr, "Field failure %d: (%d, %d)\n", i, x, y);
                failure++;
            }
            break;
        case 'X':
            if (!mem256_get(&ms->field, i)) {
                fprintf(stderr, "Field failure %d: (%d, %d)\n", i, x, y);
                failure++;
            }
            /* Fallthrough */
        case 'x':
            if (x != ms->bx || y != ms->by) {
                fprintf(stderr, "Coord failure %d: (%d, %d) but expected (%d, %d)\n",
                        i, x, ms->bx, ms->by);
                failure++;
            }
            break;
        case 'o':
            if (!mem256_get(&ms->block, i)) {
                fprintf(stderr, "Block failure %d: (%d, %d)\n", i, x, y);
                failure++;
            }
            break;
        default:
            break;
        }
    }

    if (failure) {
        fprintf(stderr, "Assertion Failure:\nFound:");
        print_limb(ms, (llen + 10 - (llen % 10)));
        fprintf(stderr, "Expected:\n");
        print_layout(layout, llen);
        errors++;
        return false;
    }

    return true;
}

void test1(void)
{
    set_layout(&ms, 0, 0,
            "##########"
            "##########"
            "##########"
            "##########");

    mptet_lineclear(&ms);

    assert_layout(&ms,
            "          "
            "          "
            "          "
            "          ");
}

void test2(void)
{
    set_layout(&ms, 0, 0,
            "##########"
            "# ########"
            "##########"
            "######## #");

    mptet_lineclear(&ms);

    assert_layout(&ms,
            "          "
            "          "
            "# ########"
            "######## #");
}

void test3(void)
{
    set_layout(&ms, 0, 0,
            "      ##  "
            "###   ##  "
            "#####  ## "
            "##########"
            " #########"
            "###### ###"
            " #########"
            " #########"
            " ##### ###");

    mptet_lineclear(&ms);

    assert_layout(&ms,
            "          "
            "      ##  "
            "###   ##  "
            "#####  ## "
            " #########"
            "###### ###"
            " #########"
            " #########"
            " ##### ###");
}

int main(void)
{
    mpstate_init(&ms);

    test1();
    test2();
    test3();

    mpstate_free(&ms);

    return errors;
}
