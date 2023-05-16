#include <types.h>
#include <tk1_mem.h>

#define SLEEPTIME  100000
#define LED_BLACK  0
#define LED_RED    (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN  (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE   (1 << TK1_MMIO_TK1_LED_B_BIT)
#define LED_WHITE  (LED_RED | LED_GREEN | LED_BLUE)
#define UINT32_MAX (4294967295U)
#define MAX_SCORE UINT32_MAX

static volatile uint32_t *led = (volatile uint32_t *)TK1_MMIO_TK1_LED;
static volatile uint32_t *trng_status  = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
static volatile uint32_t *trng_entropy = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;

enum player {
    red = LED_RED,
    blue = LED_BLUE,
    //green = LED_GREEN,
    //black = LED_BLACK,

    tie = LED_WHITE,
};

void sleep(uint32_t n)
{
    for (volatile int i = 0; i < n; i++);
}

void toggle_rgb(void)
{
    *led = LED_RED;
    sleep(SLEEPTIME);
    *led = LED_GREEN;
    sleep(SLEEPTIME);
    *led = LED_BLUE;
    sleep(SLEEPTIME);
}

/*
 * Perform coin toss
 *  Who will win today?
 */
enum player get_winner(void)
{
    uint32_t rand = 0;

    sleep(SLEEPTIME*4);

    /* wait for entropy */
    while(*trng_status == 0) {
        /* (this shouldn't happen really) */
        *led = LED_GREEN;
        sleep(SLEEPTIME*10);
    }

    //TODO this should use Hash_DRBG instead of raw TRNG?
    rand = *trng_entropy;
    sleep(SLEEPTIME);

    /*
     * Are you readyyy?
     *  Time for coin flip!
     */

    //TODO this is a bit stupid (only checks last bit)
    if (rand % 2 == 0) {
        return red;
    } else {
        return blue;
    }
}

int main(void)
{
    /*
     *  Welcome to coin race!
     */
    enum player winner;
    int32_t score_red=0, score_blue = 0;

    *led = LED_WHITE;
    sleep(SLEEPTIME);
    toggle_rgb();
    toggle_rgb();

    /* Start race, current leader is shown on LED (white for equal) */
    while (1) {
        winner = get_winner();
        if (winner == red) {
            score_red++;
        } else {
            score_blue++;
        }

        if (score_red > score_blue) {
            *led = red;
        } else if (score_blue > score_red) {
            *led = blue;
        } else {
            *led = tie;
        }

        if (score_red == MAX_SCORE || score_blue == MAX_SCORE) {
            /* Congratulations to the final winner! */
            while (1);
        }

        sleep(SLEEPTIME*10);
    }
}
