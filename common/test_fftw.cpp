// Пример использования FFTW
#include <cmath>
#include <vector>
#include <fftw3.h>
#include <iostream>
#include "utils/time_provider/TimeProvider.h"
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <chrono>

/// Генерация окна Натталла (4-терм)
float *test_getWindowNuttall(int length)
{
    if (length <= 0)
        return nullptr;

    float *window = (float *)malloc(length * sizeof(float));
    if (!window)
        return nullptr;

    const float a0 = 0.3635819f;
    const float a1 = 0.4891775f;
    const float a2 = 0.1365995f;
    const float a3 = 0.0106411f;

    for (int n = 0; n < length; n++)
    {
        float angle = 2.0f * (float)M_PI * n / (length - 1);
        window[n] = a0 - a1 * cosf(angle) + a2 * cosf(2 * angle) - a3 * cosf(3 * angle);
    }
    return window;
}

void test_fft(std::vector<float> &signal, int N,
              fftwf_complex *in, fftwf_complex *out, fftwf_plan *plan,
              float *magnitudes)
{
    // Копирование данных
    for (int n = 0; n < N; ++n)
    {
        in[n][0] = signal[2 * n];     // Re
        in[n][1] = signal[2 * n + 1]; // Im
    }

    // Выполнение БПФ
    fftwf_execute(*plan);

    // Расчет амплитудного спектра
    for (int k = 0; k < 2*N; k+=2)
    {
        magnitudes[k] = out[k/2][0];
        magnitudes[k+1] = out[k/2][1];
    }
}

void test_fftw()
{
    int N = 128;               // Количество комплексных точек
    int TOTAL_SAMPLES = 2 * N; // Общее количество элементов в массиве

    // Предварительный расчет окна Натталла
    float *window = test_getWindowNuttall(N);

    // Инициализация входного массива (чередование Re, Im)
    std::vector<float> signal(TOTAL_SAMPLES);

    // Заполнение тестовыми комплексными данными (пример)
    for (int n = 0; n < TOTAL_SAMPLES; n += 2)
    {
        float t = static_cast<float>(n / 2);
        signal[n]     = 0.8f * static_cast<float>(cos(0.01f * 2.f * M_PI * 15.f * t)) * window[n/2]; // Re
        signal[n + 1] = 0.8f * static_cast<float>(sin(0.01f * 2.f * M_PI * 15.f * t)) * window[n/2]; // Im
    }
    signal[0] = 0.f;
    signal[1] = 0.f;
    signal[TOTAL_SAMPLES - 2] = 0.f;
    signal[TOTAL_SAMPLES - 1] = 0.f;

    // Запись отсчётов тестового сигнала в файл:
    //--------------------------------------------------------------------
    QFile *file = new QFile(QString("test_signal.txt"));
//    QFile *file = new QFile(QString("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\"
//                                    "MCFOS\\FromDimaFrom_25_06_2025\\test_signal.txt"));
    if (file->open(QIODevice::WriteOnly))
    {
        QTextStream out(file);
        out << QString("%1").arg(TOTAL_SAMPLES);
        for (int i = 0; i < TOTAL_SAMPLES; ++i) {
            out << QString(", %1").arg(signal[i]);
        }
        qInfo() << "[TEST] File has wrote by data from DDR";
    }
    else
    {
        qInfo() << "[TEST] ERROR: cannot open file for write data from DDR";
    }
    file->close();
    //--------------------------------------------------------------------

    // Выделение памяти для FFTW
    fftwf_complex *in_fft = fftwf_alloc_complex(N);
    fftwf_complex *out_fft = fftwf_alloc_complex(N);
    fftwf_plan plan = fftwf_plan_dft_1d(N, in_fft, out_fft, FFTW_FORWARD, FFTW_ESTIMATE);

    float *magnitudes = (float *)malloc(TOTAL_SAMPLES * sizeof(float));
    float *magnitudes_abs = (float *)malloc(N * sizeof(float));

    // Замер времени выполнения БПФ:
    //----------------------------------------------------
    auto start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    for (int i = 0; i < 100; i++)
    {
        test_fft(signal, N, in_fft, out_fft, &plan, magnitudes);

        for (int k = 0; k < N; ++k)
        {
            // Вычисление амплитуд с квадратным корнем
//            magnitudes_abs[k] = std::sqrt(magnitudes[2*k] * magnitudes[2*k] +
//                                magnitudes[2*k+1] * magnitudes[2*k+1]);

            // Вычисление амплитуд без квадратного корня (до 3.5 раз быстрее)
            magnitudes_abs[k] = magnitudes[2*k] * magnitudes[2*k] +
                                magnitudes[2*k+1] * magnitudes[2*k+1];
        }
    }
    auto end = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    //----------------------------------------------------

    // Время выполнения БПФ:
    qInfo() << "dt =" << (end - start) / 1000.0f << "ms";

    // Запись результата БПФ в файл:
    //--------------------------------------------------------------------
    file = new QFile(QString("test_result_fft.txt"));
//    file = new QFile(QString("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\"
//                             "MCFOS\\FromDimaFrom_25_06_2025\\test_result_fft.txt"));
    if (file->open(QIODevice::WriteOnly))
    {
        QTextStream out(file);
        out << QString("%1").arg(TOTAL_SAMPLES);
        for (int i = 0; i < TOTAL_SAMPLES; ++i) {
            out << QString(", %1").arg(magnitudes[i]);
        }
        qInfo() << "[TEST] File has wrote by data from DDR";
    }
    else
    {
        qInfo() << "[TEST] ERROR: cannot open file for write data from DDR";
    }
    file->close();
    //--------------------------------------------------------------------

    // Запись результата БПФ в файл:
    //--------------------------------------------------------------------
    file = new QFile(QString("test_result_fft_abs.txt"));
//    file = new QFile(QString("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\"
//                             "MCFOS\\FromDimaFrom_25_06_2025\\test_result_fft_abs.txt"));
    if (file->open(QIODevice::WriteOnly))
    {
        QTextStream out(file);
        out << QString("%1").arg(N);
        for (int i = 0; i < N; ++i) {
            out << QString(", %1").arg(magnitudes_abs[i]);
        }
        qInfo() << "[TEST] File has wrote by data from DDR";
    }
    else
    {
        qInfo() << "[TEST] ERROR: cannot open file for write data from DDR";
    }
    file->close();
    //--------------------------------------------------------------------

    // Очистка ресурсов
    free(window);
    free(magnitudes);
    free(magnitudes_abs);

    fftwf_destroy_plan(plan);
    fftwf_free(in_fft);
    fftwf_free(out_fft);
}
