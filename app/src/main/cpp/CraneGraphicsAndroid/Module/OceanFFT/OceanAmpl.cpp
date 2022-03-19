#include "OceanAmpl.hpp"

using namespace std;
using namespace Crane;

constexpr float pi = 3.1415926f;
constexpr float EPSILON = 1e-6f;
constexpr float g = 9.8f;

OceanAmpl::OceanAmpl(int n, int m, float lx, float lz, float a, float v,
    complex<float> w, float l)
    : N(n), M(m), Lx(lx), Lz(lz), A(a), wind_speed(v), wind_dir(w), lambda(l),
    h_tlide_0(n* m, 0.),
    h_tlide_0_conj(n* m, 0.),
    normal_dist(0., 1.)
{
    rand_generator.seed(static_cast<unsigned>(time(nullptr)));
    wind_dir /= abs(wind_dir);

    for (int m = 0; m < M; ++m)
    {
        for (int n = 0; n < N; ++n)
        {
            h_tlide_0[pos2idx(n, m)] = hTlide0(getK(n, m));
            h_tlide_0_conj[pos2idx(n, m)] = conj(hTlide0(-getK(n, m)));
        }
    }
}

int OceanAmpl::pos2idx(int n, int m) const
{
    return m * N + n;
}

complex<float> OceanAmpl::getK(int n, int m) const
{
    return { 2.f * pi * (static_cast<float>(n) - static_cast<float>(N) / 2.f) / Lx,
            2.f * pi * (static_cast<float>(m) - static_cast<float>(M) / 2.f) / Lz };
}

float OceanAmpl::Ph(complex<float> k) const
{
    float k_abs = abs(k);
    if (k_abs < EPSILON)
        return 0.0f;

    // Phillips spectrum
    float L = pow(wind_speed, 2) / g;
    complex<float> k_hat = k / k_abs;
    float k_hatDotwind_dir = k_hat.real() * wind_dir.real() + k_hat.imag() * wind_dir.imag();
    float P = A * expf(-1.f / (pow(k_abs * L, 2))) / pow(k_abs, 4) * pow(k_hatDotwind_dir, 2);
    float l = 1.f; // suppress
    P *= expf(-k_abs * k_abs * l * l);

    return P;
}

std::complex<float> OceanAmpl::hTlide0(complex<float> k)
{
    float e_r = normal_dist(rand_generator);
    float e_i = normal_dist(rand_generator);

    return sqrtf(0.5f) * std::complex<float>{e_r, e_i} *sqrtf(Ph(k));
}

std::complex<float> OceanAmpl::hTlide(int n, int m, float t) const
{
    float k_norm = abs(getK(n, m));
    float omega = sqrtf(g * k_norm);
    complex<float> c = exp(complex<float>(0.0f, omega * t));

    return h_tlide_0[pos2idx(n, m)] * c + h_tlide_0_conj[pos2idx(n, m)] * conj(c);
}