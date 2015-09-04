#include <linux/fb.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
int x = 0, y = 0;
long int location = 0;

// Utility functions
static int kbhit(void)
{
    struct timeval tv = {0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

void gui_init(int argc, char **argv)
{
 // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }

   // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");
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

void render(void)
{
    int XOFFSET = 50;
    int YOFFSET = 50;
    int SIDELEN = 100;

    int location = (XOFFSET + SIDELEN * (x % 10) + vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
        (YOFFSET + SIDELEN * (y / 10) + vinfo.yoffset) * finfo.line_length;

    // Width will be 20 pixels per block
    for (int x = 0; x < SIDELEN * 10; ++x) {
        for (int y = 0; y < SIDELEN * 21; ++y) {

            // Map x, y to a linear index system and flip
            int i = 220 - (1 + (x + y * 10) / SIDELEN);

            memset(&fbp[location], 0, 4);

            /* Only change the non zero fields */
            if (mpz_tstbit(field, i) | mpz_tstbit(block, i)) {
                fbp[location + i + 0] = 255;
            }
            else if (mpz_tstbit(ghost, i)) {
                fbp[location + i + 0] = 255;
                fbp[location + i + 3] = 50;
            }
            else {
                /* Set to black earlier */
            }
        }
    }
}

void gui_free(void)
{
    munmap(fbp, screensize);
    close(fbfd);
}
