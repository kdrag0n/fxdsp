#include "fft.h"

namespace fxdsp {

// Logic copied from kiss_fft_next_fast_size, with an added requirement that the number is even
// because kiss_fftr requires an even size
int next_fft_size(int n) {
    // Start by making the number even if necessary
    if (n % 2 != 0) {
        n += 1;
    }

    while (true) {
        int m = n;
        while (m % 2 == 0) m /= 2;
        while (m % 3 == 0) m /= 3;
        while (m % 5 == 0) m /= 5;

        if (m <= 1) {
            // Even and completely factorable by twos, threes, and fives
            return n;
        }

        // Make sure the number stays even
        n += 2;
    }
}

}
