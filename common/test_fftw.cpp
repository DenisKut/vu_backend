// Пример использования FFTW для проверки подключения
#include <cmath>
#include <vector>
#include <fftw3.h>
#include <iostream>
#include "utils/time_provider/TimeProvider.h"
#include <QDebug>

void test_fft(std::vector<double> &signal, int N,
              fftw_complex* in, fftw_complex* out, fftw_plan *plan,
              std::vector<double> &window, std::vector<double> magnitudes[]) {
    // Копирование данных с применением окна
    for (int n = 0; n < N; ++n) {
        // Применяем одно и то же окно к обеим частям
        in[n][0] = signal[2 * n]     * window[n]; // Re
        in[n][1] = signal[2 * n + 1] * window[n]; // Im
    }

    // Выполнение БПФ
    fftw_execute(*plan);

    // Расчет амплитудного спектра
//    std::vector<double> magnitudes(N);
    for (int k = 0; k < N; ++k) {
        double re = out[k][0];
        double im = out[k][1];
        magnitudes->push_back(sqrt(re * re + im * im));
    }
}

void test_fftw() {
//    int N = 8;
//    double in[8] = {1,1,1,1,0,0,0,0};
//    fftw_complex out[8];
//    fftw_plan p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
//    fftw_execute(p);
//    std::cout << std::endl << "FFTW output:\n";
//    for (int i = 0; i < N/2+1; ++i) {
//        std::cout << out[i][0] << "+i" << out[i][1] << std::endl;
//    }
//    fftw_destroy_plan(p);

    int N = 1024; // Количество комплексных точек
    int TOTAL_SAMPLES = 2 * N; // Общее количество элементов в массиве

    // Инициализация входного массива (чередование Re, Im)
    std::vector<double> signal(TOTAL_SAMPLES);

    // Заполнение тестовыми комплексными данными (пример)
    for (int n = 0; n < TOTAL_SAMPLES; n += 2) {
        double t = static_cast<double>(n/2) / N;
        signal[n]   = 0.8 * cos(2.0 * M_PI * 50.0 * t); // Re
        signal[n+1] = 0.8 * sin(2.0 * M_PI * 50.0 * t); // Im
    }

    // Выделение памяти для FFTW
    fftw_complex* in = fftw_alloc_complex(N);
    fftw_complex* out = fftw_alloc_complex(N);
    fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // Предварительный расчет окна Блэкмана
    std::vector<double> window(N);
    for (int n = 0; n < N; ++n) {
        window[n] = 0.42
                  - 0.50 * cos(2.0 * M_PI * n / (N - 1))
                  + 0.08 * cos(4.0 * M_PI * n / (N - 1));
    }

    std::vector<double> magnitudes(N);

    int64_t time1, time2;
    time1 = TimeProvider::instance().getMks();
    for (int i = 0; i < 256; i++) {
        test_fft(signal, N, in, out, &plan, window, &magnitudes);
    }
    time2 = TimeProvider::instance().getMks();

    qDebug() << "time1 = " << time1;
    qDebug() << "time2 = " << time2;
    qDebug() << "dt = " << time2 - time1;

    // Очистка ресурсов
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}
