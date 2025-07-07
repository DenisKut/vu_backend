#include "Calculator.h"
#include <QMutexLocker>
#include <QByteArray>
#include <QVector>
#include <cmath>
#include <fftw3.h>

// Функция для извлечения 32-битного целого из QByteArray
inline qint32 extractInt32(const QByteArray& data, int offset) {
    return (0x000000FF & (static_cast<qint32>(static_cast<unsigned char>(data[offset]))))          |
           (0x0000FF00 & (static_cast<qint32>(static_cast<unsigned char>(data[offset + 1])) << 8)) |
           (0x00FF0000 & (static_cast<qint32>(static_cast<unsigned char>(data[offset + 2])) << 16))|
           (0xFF000000 & (static_cast<qint32>(static_cast<unsigned char>(data[offset + 3])) << 24));
}

Calculator *Calculator::m_instance = nullptr;
QMutex Calculator::m_mutex;

Calculator *Calculator::instance()
{
	if (!m_instance)
	{
		QMutexLocker locker(&m_mutex);
		if (!m_instance)
		{
			m_instance = new Calculator();
		}
	}
	return m_instance;
}

Calculator::Calculator(QObject *parent)
	: QObject(parent)
{
    qRegisterMetaType<Calculator::testDataToETH>("Calculator::testDataToETH");
}

void Calculator::setTestToETHCallback(TestToETHCallback cb)
{
	m_testToETHCallback = std::move(cb);
}

void Calculator::emitTestToETH(const testDataToETH &testData)
{
	if (m_testToETHCallback)
        m_testToETHCallback(testData);
}

/// Генерация окна Натталла (4-терм)
float *Calculator::getWindowNuttall(int length)
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

/// Генерация окна Блэкмана-Харриса (4-терм)
float *Calculator::getWindowBlackmanHarris(int length)
{
    if (length <= 0)
        return nullptr;

    float *window = (float *)malloc(length * sizeof(float));
    if (!window)
        return nullptr;

    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;

    for (int n = 0; n < length; n++)
    {
        float angle = 2.0f * (float)M_PI * n / (length - 1);
        window[n] = a0 - a1 * cosf(angle) + a2 * cosf(2 * angle) - a3 * cosf(3 * angle);
    }
    return window;
}

/// Генерация окна Блэкмана (3-терм)
float *Calculator::getWindowBlackman(int length)
{
    if (length <= 0)
        return nullptr;

    float *window = (float *)malloc(length * sizeof(float));
    if (!window)
        return nullptr;

    const float a0 = 0.42f;
    const float a1 = 0.5f;
    const float a2 = 0.08f;

    for (int n = 0; n < length; n++)
    {
        float angle = 2.0f * (float)M_PI * n / (length - 1);
        window[n] = a0 - a1 * cosf(angle) + a2 * cosf(2 * angle);
    }
    return window;
}

/// Генерация окна Тьюки
float *Calculator::getWindowTukey(int length, const float alpha) {
    if (length <= 0)
        return nullptr;

    float* window = static_cast<float*>(malloc(length * sizeof(float)));
    if (!window)
        return nullptr;

    // Обработка частных случаев
    if (length == 1) {
        window[0] = 1.0f;
        return window;
    }

    const int N = length - 1;  // Для правильной индексации (0..N)
    const float width = alpha * N / 2.0f;  // Ширина "плеча" окна

    for (int i = 0; i < length; ++i) {
        if (i < width) {
            // Возрастающая часть (левое плечо)
            float phase = static_cast<float>(i) / width - 1.0f;
            window[i] = 0.5f * (1.0f + cosf(static_cast<float>(M_PI) * phase));
        } else if (i > N - width) {
            // Убывающая часть (правое плечо)
            float phase = static_cast<float>(i - N) / width + 1.0f;
            window[i] = 0.5f * (1.0f + cosf(static_cast<float>(M_PI) * phase));
        } else {
            // Центральная часть (константа)
            window[i] = 1.0f;
        }
    }

    return window;
}

// Генерация оконной функции по заданным параметрам
float *Calculator::getWindowFunc(int N, int windowType)
{
    float *window = nullptr;
    switch (windowType)
    {
    case MCFOS_WindowType_Nuttall:
    {
        window = getWindowNuttall(N);
        break;
    }
    case MCFOS_WindowType_BlackmanHarris:
    {
        window = getWindowBlackmanHarris(N);
        break;
    }
    case MCFOS_WindowType_Blackman:
    {
        window = getWindowBlackman(N);
        break;
    }
    case MCFOS_WindowType_Tukey:
    {
        window = getWindowTukey(N, windowTukeyAlpha);
        break;
    }
    default:
    {
        window = nullptr;
        break;
    }
    }
    return window;
}

// Вычисление коэффициентов полинома ППУ
void Calculator::calcCoeffPPU(double angleTarget,
                              double velocityTarget,
                              double angleCurrent, double *angleLast,
                              double *accelerationLast, double accelerationMax,
                              // double angleMin, double velocityMin,
                              double KP, double KD, /*double dtLast,*/ double dt,
                              double *C0, double *C1, double *C2)
{

    double xDist = angleTarget - angleCurrent;
    if (xDist > 180.0)
    {
        xDist -= 360.0;
    }
    else if (xDist < -180.0)
    {
        xDist += 360.0;
    }
    double dx = angleCurrent - (*angleLast);
    if (dx > 180.0)
    {
        dx -= 360.0;
    }
    else if (dx < -180.0)
    {
        dx += 360.0;
    }
    double v1 = dx / dt; // + 0.5 * (*accelerationLast) * dt;
    double dv = velocityTarget - v1;
    double a1 = 0.0;

    //        double breaking_dist = dv * dv / (2.0 * accelerationMax) + angleMin;

    //        if (abs(xDist) > angleMin) {
    //            // Максимальное ускорение в направлении цели
    //            a1 = (xDist > 0) ? accelerationMax : -accelerationMax;

    //            // Если расстояние торможения достигнуто, то тормозим:
    //            if (abs(xDist) < breaking_dist) {
    //                a1 = (dv < 0) ? -accelerationMax : accelerationMax;
    //            }

    //            // Плавное изменение ускорения:
    //            if (abs(dv) < velocityMin) {
    //                a1 = (*accelerationLast) + 0.5 * (a1 - (*accelerationLast));
    //            }
    //        } else {
    // ПД-регулятор
    a1 = KP * xDist + KD * dv;
    //        }

    // Ограничение ускорения
    a1 = fmax(fmin(a1, accelerationMax), -accelerationMax);

    //        // Плавное изменение ускорения:
    //        if (abs(dv) < velocityMin) {
    //            a1 = (*accelerationLast) + 0.8 * (a1 - (*accelerationLast));
    //        }

    // Запоминаем предыдущие значения положения и ускорения точки 1
    *angleLast = angleCurrent;
    *accelerationLast = a1;

    // Установка коэффициентов
    *C0 = angleCurrent; // Начальное смещение (не используется)
    *C1 = v1;			// Текущая скорость (начальная для интервала)
    *C2 = 0.5 * a1;		// Ускорение, деленное на 2
}

void Calculator::receiveTestToETH(const testDataToETH &testData)
{
    Q_UNUSED(testData);
	// Здесь обработка структуры из других потоков
	// Например, сохранить, обработать, и т.д.
	// ...
	// Можно вызвать коллбэк, если он установлен
	// emitTestToETH(test);
}

// Приняты данные от МЦФОС
void Calculator::recDataMCFOS(const QByteArray &data)
{
    // Проверка номера МЦФОС, с которым сейчас идёт обмен, на граничные условия
    if (numLastMCFOS < 0) {
        qInfo() << "WARNING in Calculator::recDataMCFOS(): "
                   "numLastMCFOS < 0, numLastMCFOS ="
                << numLastMCFOS;
        numLastMCFOS = 0;
    }
    if (numLastMCFOS > countRLMN - 1) {
        qInfo() << "WARNING in Calculator::recDataMCFOS(): "
                   "numLastMCFOS > countRLMN - 1, numLastMCFOS ="
                << numLastMCFOS << "; countRLMN - 1 ="
                << countRLMN - 1;
        numLastMCFOS = countRLMN - 1;
    }

    // Проверка на размер массива data
    if (data.size() == 0) {
        qInfo() << "ERROR in Calculator::recDataMCFOS(): "
                   "data.size() == 0. Aborting function...";
        return;
    }

    // ОТЛАДКА - ввод исходных данных руками
    //-----------------------------------------------------------------
    // ЦУ
    //-------------------------------------
    m_paramRLMN[numLastMCFOS].m_paramCU.timeCeil = TimeProvider::instance().getMs() - 505;
    m_paramRLMN[numLastMCFOS].m_paramCU.timeSys = TimeProvider::instance().getMks() - 500000;
    m_paramRLMN[numLastMCFOS].m_paramCU.timeBs = TimeProvider::instance().getBsPeriodTime() - 5000000;
    m_paramRLMN[numLastMCFOS].m_paramCU.az = 130.5;
    m_paramRLMN[numLastMCFOS].m_paramCU.an = 0.2;
    m_paramRLMN[numLastMCFOS].m_paramCU.r = 1640;
    m_paramRLMN[numLastMCFOS].m_paramCU.azV = 0;
    m_paramRLMN[numLastMCFOS].m_paramCU.anV = 0;
    m_paramRLMN[numLastMCFOS].m_paramCU.rV = 0;
    m_paramRLMN[numLastMCFOS].m_paramCU.azEr = 10;
    m_paramRLMN[numLastMCFOS].m_paramCU.anEr = 10;
    m_paramRLMN[numLastMCFOS].m_paramCU.rEr = 10;
    m_paramRLMN[numLastMCFOS].m_paramCU.azVEr = 10;
    m_paramRLMN[numLastMCFOS].m_paramCU.anVEr = 10;
    m_paramRLMN[numLastMCFOS].m_paramCU.rVEr = 10;
    m_paramRLMN[numLastMCFOS].m_paramCU.numRLMN = 1;
    m_paramRLMN[numLastMCFOS].m_paramCU.mode = 1;

    // Режим работы РЛМН
    m_paramRLMN[numLastMCFOS].mode = 1;
    //-------------------------------------

    // ППУ
    //-------------------------------------
    m_paramRLMN[numLastMCFOS].m_paramPPU.timeBs = TimeProvider::instance().getBsPeriodTime() - 1000000;       // Время обновления координат от ППУ (по БС)
    m_paramRLMN[numLastMCFOS].m_paramPPU.timeSys = TimeProvider::instance().getMks() - 100000;     // Время приёма пакета от БС (системное)
    m_paramRLMN[numLastMCFOS].m_paramPPU.az = 130.5;               // Текущая горизонтальная координата, град
    m_paramRLMN[numLastMCFOS].m_paramPPU.an = 0.2;               // Текущая вертикальная координата, град
    m_paramRLMN[numLastMCFOS].m_paramPPU.lastCommand = 0;	 // Код последней полученной команды
    m_paramRLMN[numLastMCFOS].m_paramPPU.state = 0;			// Состояние
    m_paramRLMN[numLastMCFOS].m_paramPPU.ispr = 0;			// Исправность оборудования
    m_paramRLMN[numLastMCFOS].m_paramPPU.azDelay = 0;		// Задержка измерения координаты по азимуту, мс
    m_paramRLMN[numLastMCFOS].m_paramPPU.anDelay = 0;		// Задержка измерения координаты по углу места, мс
    m_paramRLMN[numLastMCFOS].m_paramPPU.borders = 0;		// Состояние границ
    m_paramRLMN[numLastMCFOS].m_paramPPU.waitCommands = 0; // Время ожидания команд, мс
    m_paramRLMN[numLastMCFOS].m_paramPPU.waitTarget = 0;	 // Время ожидания целеуказаний, мс

    // Ошибки драйверов: 0х31-ошибка Аз, 0х32-ошибка Ум
    m_paramRLMN[numLastMCFOS].m_paramPPU.driverError[0] = 0;
    m_paramRLMN[numLastMCFOS].m_paramPPU.driverError[1] = 0;
    m_paramRLMN[numLastMCFOS].m_paramPPU.driverError[2] = 0;

    // Признаки срабатывания концевых выключателей
    m_paramRLMN[numLastMCFOS].m_paramPPU.signsSwithes[0] = 0;
    m_paramRLMN[numLastMCFOS].m_paramPPU.signsSwithes[1] = 0;
    m_paramRLMN[numLastMCFOS].m_paramPPU.signsSwithes[2] = 0;

    m_paramRLMN[numLastMCFOS].m_paramPPU.operatingMode = 0;	 // Режим работы
    m_paramRLMN[numLastMCFOS].m_paramPPU.numRLMN = 0; // Номер РЛ-канала
    //-------------------------------------

    // МЦФОС
    //-------------------------------------
    // Время предыдущего излучения МЦФОС (системное)
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.timeSys = TimeProvider::instance().getMks() - 200000;

    // Параметры излучаемого сигнала
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.N = 800;             // Длительность сигнала
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.windowType = 0;      // Тип оконной функции
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.A = 32760;           // Амплитуда сигнала
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.f0 = 15;             // Начальная частота
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.df = 20;             // Девиация частоты
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.isSignalFromDAC = 0; // Признак захвата сигнала из буфера ЦАП
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.periodSize = 6240;   // Длительность периода
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.periodCount = 128;   // Кол-во периодов
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.ampOff = 860;        // Длительность сигнала разрешения передатчика
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.pulseOn = 2;         // Задержка начала сигнала
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.pulseOff = 802;      // Задержка окончания сигнала

    // Параметры сигнальной обработки
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.procMagShift = 8;    // Сдвиг вправо результата корреляции (0...8)
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.procDelay = 1001;    // Задержка первого коррелятора
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.procDelta = 5;       // Задержки между корреляторами
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.numRangeCh = 100;    // Кол-во дальностных каналов (1...100)
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.thres_dB = 107.f;    // Порог для обнаружения в дБ
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.thres = 223872;      // Порог для обнаружения в отсчётах
    m_paramRLMN[numLastMCFOS].m_paramMCFOS.isCorrEnable = true; // Включена ли корреляционная обработка
    //-------------------------------------
    //-----------------------------------------------------------------

    // Если включена корреляционная обработка
    if (m_paramRLMN[numLastMCFOS].m_paramMCFOS.isCorrEnable) {
        // Если оконная функция для обработки сигнала
        // от МЦФОС не была создана, то создаём её
        if (windowREF == nullptr) {
            int N = m_paramRLMN[numLastMCFOS].m_paramMCFOS.N;
            int windowType = m_paramRLMN[numLastMCFOS].m_paramMCFOS.windowType;
            windowREF = getWindowFunc(N, windowType);
        }

        // ОШИБКА - не удалось создать новую
        // оконную функцию для опорного сигнала
        if (windowREF == nullptr)
        {
            qInfo() << "ERROR in Calculator::recDataMCFOS(...): "
                       "cannot create NEW window function for MCFOS REF! "
                       "Aborting function...";
            return;
        }

        // Заготовка массива с результатами обрботки сигнала
        QVector<QVector<float>> vecResult[4];

        // Обработка сигнала с получением
        // 4-х дальностно-скоростных портретов
        int numRangeCh = static_cast<int>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.numRangeCh);
        int periodCount = static_cast<int>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.periodCount);
        getDV(data,
              static_cast<int>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.periodCount),
              static_cast<int>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.numRangeCh),
              windowREF, vecResult);

        // Формирование замера:
        // Находим пики максимального уровня
        // во всех 4-х вычисленных массивах
        int receiverCount = 4;
        uint32_t aMax[4] = {0,};
        uint32_t rMax[4] = {0,};
        uint32_t vMax[4] = {0,};
        for (int r = 0; r < receiverCount; ++r) {
            for (int ir = 0; ir < numRangeCh; ++ir) {
                for (int iv = 0; iv < periodCount; ++iv) {
                    uint32_t a = static_cast<uint32_t>(vecResult[r][ir][iv]);
                    if (a > aMax[r]) {
                        aMax[r] = a;
                        rMax[r] = ir;
                        vMax[r] = iv;
                    }
                }
            }
        }
        qInfo() << "aMax[0] =" << aMax[0];
        qInfo() << "rMax[0] =" << rMax[0];
        qInfo() << "vMax[0] =" << vMax[0];
        // Проверка на превышение порога хотя бы одним из максимумов
        int check = -1;
        uint32_t thres = periodCount * m_paramRLMN[numLastMCFOS].m_paramMCFOS.thres; // Нормировка
        qInfo() << "thres =" << thres;
        for (int r = 0; r < receiverCount; ++r) {
            if (aMax[r] >= thres) {
                check = r;
                break;
            }
        }
        // Если хотя бы один из максимумов превысил порог,
        // то приступаем к формированию замера
        if (check >= 0) {
            // Создаём новый замер
            paramSample sample;

            // Записываем время формирования нового замера
            sample.timeSys = TimeProvider::instance().getMks(); // Текущее время (системное)
            sample.timeBs = TimeProvider::instance().getBsPeriodTime(); // Текущее время (по БС)

            // Исходные данные на момент формирования замера
            //-------------------------------------------------------------------------------
            // Данные ЦУ
            sample.cu = m_paramRLMN[numLastMCFOS].m_paramCU;
            // Данные ППУ
            sample.ppu = m_paramRLMN[numLastMCFOS].m_paramPPU;
            // Данные МЦФОС
            sample.mcfos = m_paramRLMN[numLastMCFOS].m_paramMCFOS;
            // Дальностно-скоростные портреты из 4-х приёмников,
            // амплитуды и номера дальности и скорости в парциальных каналах
            for (int r = 0; r < receiverCount; ++r) {
                sample.vecResult[r] = vecResult[r];
                sample.a[r] = static_cast<uint32_t>(vecResult[r][rMax[check]][vMax[check]]);
                sample.ri[r] = rMax[check];
                sample.rVi[r] = vMax[check];
            }
            //-------------------------------------------------------------------------------

            // Вычисление замера на основе исходных данных
            //-------------------------------------------------------------------------------
            // Вычисление углов
            //-----------------------------------------------------------------------
            // Расположение приёмников: // 3---0
                                        // |   |
                                        // 2---1
            uint32_t azDiff = sample.a[0] + sample.a[1] - sample.a[2] - sample.a[3];
            uint32_t anDiff = sample.a[0] + sample.a[3] - sample.a[1] - sample.a[2];
            uint32_t aSum   = sample.a[0] + sample.a[1] + sample.a[2] + sample.a[3];
            if (aSum == 0) aSum = 1; // Сумма не должна быть нулевая, чтобы не делить на нуль
            sample.point.az = rRay * static_cast<float>(azDiff) / static_cast<float>(aSum); // Азимут замера
            sample.point.an = rRay * static_cast<float>(anDiff) / static_cast<float>(aSum); // Угол места замера
            qInfo() << "sample.point.az =" << sample.point.az;
            qInfo() << "sample.point.an =" << sample.point.an;
            //-----------------------------------------------------------------------

            // Вычисление средней амплитуды
            //-----------------------------------------------------------------------
            float aMean = static_cast<float>(aSum / (4 * periodCount)); // С нормировкой
            if (aMean == 0.0f) aMean = 0.25f; // Средняя амплитуда не должна быть нулевой,
                                              // иначе не посчитается логарифм (если хотя бы
                                              // в одном из каналов есть 1, то 1/4 = 0.25)
            float aMean_dB = 20 * log10f(aMean);
            sample.point.aMean = aMean;       // Средняя амплитуда в отсчётах
            sample.point.aMean_dB = aMean_dB; // Средняя амплитуда в дБ
            qInfo() << "sample.point.aMean =" << sample.point.aMean;
            qInfo() << "sample.point.aMean_dB =" << sample.point.aMean_dB;
            //-----------------------------------------------------------------------

            // Вычисление дальности
            //-----------------------------------------------------------------------
            int shiftNul = m_paramRLMN[numLastMCFOS].shiftNul;
            int delay = static_cast<int>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.procDelay);
            int delta = static_cast<int>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.procDelta);
            int ri = delay + delta * rMax[check] - shiftNul; // Дальность в отсчётах по 10 нс
            // Радиоволна за 10 нс проходит 300000000 м/с * 10 нс = 3 м,
            // задержка максимума учитывает путь радиоволны до цели и обратно,
            // поэтому нужно ещё поделить расстояние на 2, чтобы определить
            // расстояние в одну сторону до цели
            sample.point.r = static_cast<float>((ri * 3)) / 2.f; // Дальность в метрах
            qInfo() << "sample.point.r =" << sample.point.r;
            //-----------------------------------------------------------------------

            // Вычисление радиальной скорости
            //-----------------------------------------------------------------------
            const float alpha = 225000.f; // 9 мм / (4 * 10 нс) - константа
            // Период повторения импульсов в пачке в отсчётах по 10 нс (константа это учитывает)
            float period = static_cast<float>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.periodSize);
            // Диапазон однозначного измерения скорости
            // (максимальная измеряемая скорость) в м/с
            float vDelta = alpha / period;
            // Определяем, сколько м/с приходится
            // на один скоростной отсчёт
            float dv = vDelta / static_cast<float>(m_paramRLMN[numLastMCFOS].m_paramMCFOS.periodCount);
            // Вычисляем радиальную скорость
            sample.point.rV = dv * vMax[check];   // Радиальная скорость в м/с
            qInfo() << "sample.point.rV =" << sample.point.rV;
            //-----------------------------------------------------------------------
            //-------------------------------------------------------------------------------

            // Укладываем новый замер в массив
            m_paramRLMN[numLastMCFOS].samples.append(sample);

            // Траекторная обработка
            // ...

            // Выдача отметок траектории на ПБУ
            // ...

            qInfo() << "TIME END MCFOS: " << TimeProvider::instance().getMks();
        }

        // Запись массива чисел в текстовый файл для удобства загрузки в MatLab
        //--------------------------------------------------------------------
        for (int num = 0; num < receiverCount; num++) {
            QFile *file = new QFile(QString("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\"
                                    "MCFOS\\FromDimaFrom_25_06_2025\\resultDV%1.txt").arg(num));
            if (file->open(QIODevice::WriteOnly))
            {
                QTextStream out(file);
                out << QString("%1, %2").arg(numRangeCh).arg(periodCount);
                for (int ir = 0; ir < numRangeCh; ++ir) {
                    for (int iv = 0; iv < periodCount; ++iv) {
                        out << QString(", %1").arg(vecResult[num][ir][iv]);
                    }
                }
                qInfo() << "[MCFOS] File has wrote by data from DDR";
            }
            else
            {
                qInfo() << "[MCFOS] ERROR: cannot open file for write data from DDR";
            }
            file->close();
        }
        //--------------------------------------------------------------------
    }
    // Если считана сигнальная дорожка
    else {
        // ...
    }

//            // Запись массива чисел в текстовый файл для удобства загрузки в MatLab
//            //--------------------------------------------------------------------
//            if (isWriteFile) {
//                QFile *file = new QFile("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\MCFOS\\resultFromDDR.txt");
//                if (file->open(QIODevice::WriteOnly))
//                {
//                    QTextStream out(file);
//                    for (int i = 0; i < vecResult.size(); i++)
//                    {
//                        out << QString("%1, ").arg(vecResult[i]);
//                    }
//                    qInfo() << "[MCFOS] File has wrote by data from DDR";
//                }
//                else
//                {
//                    qInfo() << "[MCFOS] ERROR: cannot open file for write data from DDR";
//                }
//                file->close();
//            }
//            //--------------------------------------------------------------------
}



// Принято ЦУ от ПБУ или от РМО
void Calculator::recCU(paramCU cu)
{
//    // Отладочный вывод принятых данных
//    qInfo() << Qt::endl << "Received CU !!!";
//    qInfo() << "cu.timeCeil =" << cu.timeCeil;  // Время получения замера (от Сервера)
//    qInfo() << "cu.timeSys =" << cu.timeSys;    // Время приёма сообщения в мкс (системное)
//    qInfo() << "cu.timeBs =" << cu.timeBs;      // Время приёма сообщения в мкс (по БС)
//    qInfo() << "cu.az =" << cu.az;       // Азимут
//    qInfo() << "cu.an =" << cu.an;       // Угол места
//    qInfo() << "cu.r =" << cu.r;         // Дальность
//    qInfo() << "cu.azV =" << cu.azV;     // Скорость изменения азимута
//    qInfo() << "cu.anV =" << cu.anV;     // Скорость изменения угла места
//    qInfo() << "cu.rV =" << cu.rV;       // Радиальная скорость
//    qInfo() << "cu.azEr =" << cu.azEr;   // Ошибка по азимуту
//    qInfo() << "cu.anEr =" << cu.anEr;   // Ошибка по углу места
//    qInfo() << "cu.rEr =" << cu.rEr;     // Ошибка по дальности
//    qInfo() << "cu.azVEr =" << cu.azVEr; // Ошибка по азимутальной скорости
//    qInfo() << "cu.anVEr =" << cu.anVEr; // Ошибка по угломестной скорости
//    qInfo() << "cu.rVEr =" << cu.rVEr;   // Ошибка по радиальной скорости
//    qInfo() << "cu.numRLMN =" << cu.numRLMN;   // Номер РЛ-канала
//    qInfo() << Qt::endl;

    // Проверка номера РЛМН, для которого получено ЦУ, на граничные условия
    int numRLMN = cu.numRLMN;
    if (numRLMN < 0) {
        qInfo() << "WARNING in Calculator::recCU(): "
                   "numRLMN < 0, numRLMN ="
                << numRLMN;
        numRLMN = 0;
    }
    if (numRLMN > countRLMN - 1) {
        qInfo() << "WARNING in Calculator::recCU(): "
                   "numRLMN > countRLMN - 1, numRLMN ="
                << numRLMN << "; countRLMN - 1 ="
                << countRLMN - 1;
        numRLMN = countRLMN - 1;
    }

    // Сохранение данных в общую структуру:
    //-------------------------------------------------------------------------
    m_paramRLMN[numRLMN].m_paramCU.timeCeil = cu.timeCeil;
    m_paramRLMN[numRLMN].m_paramCU.timeSys = cu.timeSys;
    m_paramRLMN[numRLMN].m_paramCU.timeBs = cu.timeBs;
    m_paramRLMN[numRLMN].m_paramCU.az = cu.az;
    m_paramRLMN[numRLMN].m_paramCU.an = cu.an;
    m_paramRLMN[numRLMN].m_paramCU.r = cu.r;
    m_paramRLMN[numRLMN].m_paramCU.azV = cu.azV;
    m_paramRLMN[numRLMN].m_paramCU.anV = cu.anV;
    m_paramRLMN[numRLMN].m_paramCU.rV = cu.rV;
    m_paramRLMN[numRLMN].m_paramCU.azEr = cu.azEr;
    m_paramRLMN[numRLMN].m_paramCU.anEr = cu.anEr;
    m_paramRLMN[numRLMN].m_paramCU.rEr = cu.rEr;
    m_paramRLMN[numRLMN].m_paramCU.azVEr = cu.azVEr;
    m_paramRLMN[numRLMN].m_paramCU.anVEr = cu.anVEr;
    m_paramRLMN[numRLMN].m_paramCU.rVEr = cu.rVEr;
    m_paramRLMN[numRLMN].m_paramCU.numRLMN = cu.numRLMN;
    m_paramRLMN[numRLMN].m_paramCU.mode = cu.mode;
    //-------------------------------------------------------------------------

    // Режим работы РЛМН
    m_paramRLMN[numRLMN].mode = cu.mode;
}



// Принято сообщение синхронизации от БС (2.5)
void Calculator::recTimeBS()
{
    for (int i = 0; i < countRLMN; i++) {
        // Если есть какое-то задание на РЛМН i
        switch (m_paramRLMN[i].mode) {
        case 1: { // 1 - обнаружение
            // Если последнее ЦУ пришло более 2-х секунд назад,
            // то сброситься в ожидание команд:
            const int64_t timeMaxCU = 2000000; // 2 секунды в мкс
            int64_t timeCurrent = TimeProvider::instance().getMks(); // Текущее время
            int64_t timeCU = m_paramRLMN[i].m_paramCU.timeSys; // Время получения ЦУ
            if (timeCurrent - timeCU > timeMaxCU) {
                m_paramRLMN[i].mode = 0;

                // Отправить нулевой полином на ППУ:
                // ...
            } else {
                // Отправить полином на ППУ по координатам последнего ЦУ:
                // ...
            }

            // Проверка на попадание цели в луч:
            float azCU = m_paramRLMN[i].m_paramCU.az;
            float anCU = m_paramRLMN[i].m_paramCU.an;
            float azEr = m_paramRLMN[i].m_paramCU.azEr;
            float anEr = m_paramRLMN[i].m_paramCU.anEr;
            float azPPU = m_paramRLMN[i].m_paramPPU.az;
            float anPPU = m_paramRLMN[i].m_paramPPU.an;
            if (azPPU > azCU - azEr - rRay && azPPU < azCU + azEr + rRay &&
                anPPU > anCU - anEr - rRay && anPPU < anCU + anEr + rRay) {
                // Если попадаем, то рассчитать параметры для МЦФОС и излучить:
                // ...
            }
            break;
        }
        case 2: { // 2 - сопровождение
            break;
        }
        case 3: { // 3 - разовый замер
            break;
        }
        default:
            break;
        }
    }
}



// Приняты координаты ППУ от БС (11.x)
void Calculator::recPosPPU(paramPPU ppu)
{
    // Проверка номера РЛМН, для которого получено ЦУ, на граничные условия
    int numRLMN = ppu.numRLMN;
    if (numRLMN < 0) {
        qInfo() << "WARNING in Calculator::recPosPPU(): "
                   "numRLMN < 0, numRLMN ="
                << numRLMN;
        numRLMN = 0;
    }
    if (numRLMN > countRLMN - 1) {
        qInfo() << "WARNING in Calculator::recPosPPU(): "
                   "numRLMN > countRLMN - 1, numRLMN ="
                << numRLMN << "; countRLMN - 1 ="
                << countRLMN - 1;
        numRLMN = countRLMN - 1;
    }

    // Сохранение данных в общую структуру:
    //-------------------------------------------------------------------------
    m_paramRLMN[numRLMN].m_paramPPU.timeBs = ppu.timeBs;       // Время обновления координат от ППУ (по БС)
    m_paramRLMN[numRLMN].m_paramPPU.timeSys = ppu.timeSys;     // Время приёма пакета от БС (системное)
    m_paramRLMN[numRLMN].m_paramPPU.az = ppu.az;               // Текущая горизонтальная координата, град
    m_paramRLMN[numRLMN].m_paramPPU.an = ppu.an;               // Текущая вертикальная координата, град
    m_paramRLMN[numRLMN].m_paramPPU.lastCommand = ppu.lastCommand;	 // Код последней полученной команды
    m_paramRLMN[numRLMN].m_paramPPU.state = ppu.state;			// Состояние
    m_paramRLMN[numRLMN].m_paramPPU.ispr = ppu.ispr;			// Исправность оборудования
    m_paramRLMN[numRLMN].m_paramPPU.azDelay = ppu.azDelay;		// Задержка измерения координаты по азимуту, мс
    m_paramRLMN[numRLMN].m_paramPPU.anDelay = ppu.anDelay;		// Задержка измерения координаты по углу места, мс
    m_paramRLMN[numRLMN].m_paramPPU.borders = ppu.borders;		// Состояние границ
    m_paramRLMN[numRLMN].m_paramPPU.waitCommands = ppu.waitCommands; // Время ожидания команд, мс
    m_paramRLMN[numRLMN].m_paramPPU.waitTarget = ppu.waitTarget;	 // Время ожидания целеуказаний, мс

    // Ошибки драйверов: 0х31-ошибка Аз, 0х32-ошибка Ум
    m_paramRLMN[numRLMN].m_paramPPU.driverError[0] = ppu.driverError[0];
    m_paramRLMN[numRLMN].m_paramPPU.driverError[1] = ppu.driverError[1];
    m_paramRLMN[numRLMN].m_paramPPU.driverError[2] = ppu.driverError[2];

    // Признаки срабатывания концевых выключателей
    m_paramRLMN[numRLMN].m_paramPPU.signsSwithes[0] = ppu.signsSwithes[0];
    m_paramRLMN[numRLMN].m_paramPPU.signsSwithes[1] = ppu.signsSwithes[1];
    m_paramRLMN[numRLMN].m_paramPPU.signsSwithes[2] = ppu.signsSwithes[2];

    m_paramRLMN[numRLMN].m_paramPPU.operatingMode = ppu.operatingMode; // Режим работы
    m_paramRLMN[numRLMN].m_paramPPU.numRLMN = ppu.numRLMN;             // Номер РЛ-канала
    //-------------------------------------------------------------------------
}





// Вычисление дальностно-скоротного портрета
void Calculator::getDV(const QByteArray& inputData,
                       int impulseCount,
                       int corrCount,
                       const float* windowFunc,
                       QVector<QVector<float>> vecResult[4])
{
    const int receiverCount = 4;
    const int bytesPerReceiver = 8; // re+im по 4 байта
    const int bytesPerBlock = 32;   // 4 приемника * 8 байт

    // Проверка входных данных
    //-----------------------------------------------------------------
    // ОШИБКА - несоответствие размера входного буфера ожидаемому
    if (inputData.size() != bytesPerBlock * corrCount * impulseCount) {
        qInfo("ERROR in Calculator::getDV(): invalid recDDR size! "
              "Aborting function...");
        return;
    }

    // ОШИБКА - пустая оконная функция для опорного сигнала
    if (windowFunc == nullptr)
    {
        qInfo() << "ERROR in Calculator::getDV(): empty window function for MCFOS REF! "
                   "Aborting function...";
        return;
    }
    //-----------------------------------------------------------------

    // Выделение памяти для FFTW
    fftwf_complex* in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * impulseCount);
    fftwf_complex* out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * impulseCount);
    fftwf_plan plan = fftwf_plan_dft_1d(impulseCount, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // Инициализация выходных структур
    for (int r = 0; r < receiverCount; ++r) {
        vecResult[r] = QVector<QVector<float>>(corrCount);
        for (int s = 0; s < corrCount; ++s) {
            vecResult[r][s] = QVector<float>(impulseCount);
        }
    }

    // Данные расположены в массиве inputData следующим образом:
    // reXYZ - действительная составляющая X-го отсчёта Y-го приёмного канала Z-го импульса
    // imXYZ - мнимая составляющая X-го отсчёта Y-го приёмного канала Z-го импульса
    // X = 0...corrCount - 1
    // Y = 0...receiverCount - 1
    // Z = 0...impulseCount - 1
    // re305 - действительная составляющая 3-го отсчёта 0-го приёмного канала 5-го импульса
    // im305 - мнимая составляющая 3-го отсчёта 0-го приёмного канала 5-го импульса
    // Структура массива recDDR (первые 14 элементов слева направо, начиная с 0-го):
    // [re000 im000 re010 im010 re020 im020 re030 im030 re100 im100 re110 im110 re120 im120 ...],
    // где inputData[0] == re000, inputData[1] == im000, inputData[2] == re010 и т.д.

//    QFile *file = new QFile(QString("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\"
//                            "MCFOS\\FromDimaFrom_25_06_2025\\resultDVorigin.txt"));
//    QFile *file1 = new QFile(QString("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\"
//                            "MCFOS\\FromDimaFrom_25_06_2025\\resultDVwindowed.txt"));
//    if (file->open(QIODevice::WriteOnly) && file1->open(QIODevice::WriteOnly))
//    {
//        QTextStream outS(file);
//        QTextStream outS1(file1);
//        outS << QString("%1, %2").arg(corrCount).arg(impulseCount);
//        outS1 << QString("%1, %2").arg(corrCount).arg(impulseCount);

    // Обработка данных
    for (int receiver = 0; receiver < receiverCount; ++receiver) {
        for (int sample = 0; sample < corrCount; ++sample) {
            // Заполнение входного массива
            for (int pulse = 0; pulse < impulseCount; ++pulse) {
                // Расчет позиции в данных
                const int blockOffset = bytesPerBlock * (pulse * corrCount + sample);
                const int byteOffset = blockOffset + receiver * bytesPerReceiver;

                // Извлечение данных
                const float re = static_cast<float>(extractInt32(inputData, byteOffset));
                const float im = static_cast<float>(extractInt32(inputData, byteOffset + 4));

//                outS << QString(", %1, %2").arg(re).arg(im);

                // Умножение на оконную функцию
                in[pulse][0] = re * windowFunc[pulse];
                in[pulse][1] = im * windowFunc[pulse];

//                outS1 << QString(", %1, %2").arg(re * windowFunc[pulse]).arg(im * windowFunc[pulse]);
            }

            // Выполнение БПФ
            fftwf_execute(plan);

            // Сохранение результатов - модуль комплексной амплитуды
            // без вычисления квадратного корня (экономим время на
            // вычислении корня - сравниваем результат с квадратом порога)
            for (int k = 0; k < impulseCount; ++k) {
//                const float magnitude = std::sqrt(out[k][0]*out[k][0] + out[k][1]*out[k][1]);
                const float magnitude = out[k][0]*out[k][0] + out[k][1]*out[k][1];
                vecResult[receiver][sample][k] = magnitude;
            }
        }
    }
//        qInfo() << "[MCFOS] File has wrote by data from DDR";
//    }
//    else
//    {
//        qInfo() << "[MCFOS] ERROR: cannot open file for write data from DDR";
//    }
//    file->close();
//    file1->close();
    // Освобождение ресурсов FFTW
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);
}





