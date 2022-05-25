#include "dsp.h"

namespace dsp
{
namespace waveshaper
{
typedef union fp32_to_u32 {
    float f;
    uint32_t u;
} fp32_to_u32;

inline float fast_atan(float x)
{
    fp32_to_u32 f2u;
    f2u.f = x;
    f2u.u &= 0x7FFFFFFF;
    return PI_DIV_4 * x - x * (f2u.f - 1.0f) * (0.2447f + 0.0663f * f2u.f);

    // another option
    // const int abs_x_bits = *(uint32_t*)&x & 0x7FFFFFFF;
    // const float abs_x = *(float*)&abs_x_bits;
    // return M_PI_4 * x - x * (abs_x - 1.0f) * (0.2447f + 0.0663f * abs_x);
}

void process(buffer_container &buffer, const params &p)
{
    for (std::vector<float> &b : buffer) {
        for (float &f : b) {
            float sample = f;
            for (uint32_t j = 0; j < p.num_stages; j++) {
                const int32_t mask = *(int32_t *)&sample >> 0x1f;
                fp32_to_u32 coeff;
                coeff.u = (~mask & *(uint32_t *)&p.coef_pos) | (mask & *(uint32_t *)&p.coef_neg);
                sample = (1.0f / fast_atan(coeff.f)) * fast_atan(coeff.f * sample);
                const uint32_t inverted = *(uint32_t *)&sample ^ (0x80000000 & ~((p.invert_stages & j) - 0x01));
                sample = *(float *)&inverted;
            }
            sample *= p.gain;
            f = sample;
        }
    }
}

} // namespace waveshaper
} // namespace dsp