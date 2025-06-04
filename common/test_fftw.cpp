// Пример использования FFTW для проверки подключения
#include <fftw3.h>
#include <iostream>

void test_fftw() {
    int N = 8;
    double in[8] = {1,1,1,1,0,0,0,0};
    fftw_complex out[8];
    fftw_plan p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    fftw_execute(p);
    std::cout << std::endl << "FFTW output:\n";
    for (int i = 0; i < N/2+1; ++i) {
        std::cout << out[i][0] << "+i" << out[i][1] << std::endl;
    }
    fftw_destroy_plan(p);
}
