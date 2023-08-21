#include <cmath>

#include "amplitude.h"

namespace fxdsp::amplitude {

float db_to_linear(float db) {
    return pow(10.0f, db / 20.0f);
}

float linear_to_db(float linear) {
    // Our pipeline uses [-1, 1] normalized floats, so ref is always 1
    // k = 20 because PCM is amplitude
    return 20.0f * log10(linear);
}

}
