/* Termios settings */
static struct termios tnew, told;

// Utility functions
static int kbhit(void)
{
    struct timeval tv = {0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

void gui_init(void)
{
    // Change console mode on program entry
    tcgetattr(0, &told);
    memcpy(&tnew, &told, sizeof(struct termios));
    tnew.c_lflag &= ~(ICANON | ECHO);
    tnew.c_cc[VEOL] = 1;
    tnew.c_cc[VEOF] = 2;
    tcsetattr(0, TCSANOW, &tnew);

    // Buffer entire field contents
    static char __print_buffer[256];
    setvbuf(stdout, __print_buffer, _IOFBF, sizeof(__print_buffer));
}

// Game loop
bool update(void)
{
#ifdef NDEBUG
    if (kbhit()) {
#endif
        int ch = getchar();
        switch (ch) {
            case 'h':
                move_horizontal(1);
                break;
            case 'l':
                move_horizontal(-1);
                break;
            case 'j':
                move_down();
                break;
            case 'z':
                move_rotate(-1);
                break;
            case 'x':
                move_rotate(1);
                break;
            case 'b':
                move_harddrop();
                clear_lines();
                break;
            case 'q':
                return false;
        }

#ifdef NDEBUG
        // Discard all keypresses stil in queue so we don't buffer endlessly
        while (kbhit()) ch = getchar();
    }
#endif

    return true;
}

void render()
{
    printf("\033[2J%.2f Frames:\n", current_frames);
    printf("|");
    for (int i = 219; i >= 0; --i) {
        printf("%c", (mpz_tstbit(field, i) | mpz_tstbit(block, i)) ? '.' :
                mpz_tstbit(ghost, i) ? '#' : ' ');
        if (i % 10 == 0)
            printf("|\n|");
    }

    printf("----------|\n");
    fflush(stdout);
}

void gui_free(void)
{
    // Revert console move on program exit
    tcsetattr(0, TCSANOW, &told);
}
