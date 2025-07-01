#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>
#include <QMutex>
#include <QMetaType>
#include <functional>

class Calculator : public QObject
{
	Q_OBJECT
public:
	// Получить экземпляр синглтона
	static Calculator *instance();

	Calculator(const Calculator &) = delete;
	Calculator &operator=(const Calculator &) = delete;
	Calculator(Calculator &&) = delete;
	Calculator &operator=(Calculator &&) = delete;

    /**
     * @brief Вычисление дальностно-скоротного портрета
     */
    void getDV(const QByteArray& inputData,
               int impulseCount,
               int corrCount,
               const float *windowFunc,
               QVector<QVector<float>> vecResult[4]);

    /**
     * @struct paramCU
     * @brief Параметры ЦУ от ПБУ или РМО
     */
	struct paramCU
	{
        int64_t timeCeil; // Время получения замера (от Сервера)
        int64_t timeSys;  // Время получения пакета (системное)
        int64_t timeBs;   // Время получения пакета (по БС)

        float az;    // Азимут
        float an;    // Угол места
        float r;     // Дальность

        float azV;   // Скорость изменения азимута
        float anV;   // Скорость изменения угла места
        float rV;    // Радиальная скорость

        float azEr;  // Ошибка по азимуту
        float anEr;  // Ошибка по углу места
        float rEr;	 // Ошибка по дальности

        float azVEr; // Ошибка по азимутальной скорости
        float anVEr; // Ошибка по угломестной скорости
        float rVEr;	 // Ошибка по радиальной скорости

		// Информационные параметры (тип цели и т.д.)
        int numRLMN; // Номер РЛ-канала
        int mode;    // Режим работы РЛМН
                     // 0 - ожидание команд
                     // 1 - обнаружение
                     // 2 - сопровождение
                     // 3 - разовый замер
    };

    /**
     * @struct paramTargetPPU
     * @brief Параметры ЦУ для ППУ
     */
    struct paramTargetPPU
    {
        int64_t timeSys;         ///< Время формирования ЦУ для ППУ (системное)
        int64_t timeBs;          ///< Время формирования ЦУ для ППУ (по БС)

        float az;                ///< Назначенная горизонтальная координата, град
        float an;                ///< Назначенная вертикальная координата, град
    };

    /**
     * @struct paramPolinomPPU
     * @brief Рассчитанные данные полинома
     *        на основе текущих параметров ППУ и ЦУ
     */
    struct paramPolinomPPU
    {

    };

    /**
     * @struct paramPPU
     * @brief Параметры ППУ
     */
    struct paramPPU
    {
        int64_t timeSys;         ///< Время обновления координат от ППУ (системное)
        int64_t timeBs;          ///< Время обновления координат от ППУ (по БС)

        float az;                ///< Текущая горизонтальная координата, град
        float an;                ///< Текущая вертикальная координата, град

        uint8_t lastCommand;	 ///< Код последней полученной команды
        uint8_t state;			 ///< Состояние
        uint8_t ispr;			 ///< Исправность оборудования
        uint8_t azDelay;		 ///< Задержка измерения координаты по
                                 ///< азимуту, мс
        uint8_t anDelay;		 ///< Задержка измерения координаты по углу
                                 ///< места, мс
        uint8_t borders;		 ///< Состояние границ
        uint8_t waitCommands;	 ///< Время ожидания команд, мс
        uint8_t waitTarget;		 ///< Время ожидания целеуказаний, мс
        uint8_t driverError[3];	 ///< Ошибки драйверов: 0х31-ошибка Аз,
                                 ///< 0х32-ошибка Ум
        uint8_t signsSwithes[3]; ///< Признаки срабатывания концевых
                                 ///< выключателей
        uint8_t operatingMode;	 ///< Режим работы

        // Информационные параметры
        int numRLMN; // Номер РЛ-канала
    };

    /**
     * @struct paramMCFOS
     * @brief Параметры МЦФОС
     */
    struct paramMCFOS
    {
        int64_t time; // Время предыдущего излучения МЦФОС (системное)

        // Параметры излучаемого сигнала
        int N;                      // Длительность сигнала
        int windowType;             // Тип оконной функции
        float A;                    // Амплитуда сигнала
        float f0;                   // Начальная частота
        float df;                   // Девиация частоты
        quint32 isSignalFromDAC;    // Признак захвата сигнала из буфера ЦАП
        uint16_t periodSize;        // Длительность периода
        uint16_t periodCount;       // Кол-во периодов
        uint32_t ampOff;            // Длительность сигнала разрешения передатчика
        uint16_t pulseOn;           // Задержка начала сигнала
        uint16_t pulseOff;          // Задержка окончания сигнала

        // Параметры сигнальной обработки
        uint8_t procMagShift;       // Сдвиг вправо результата корреляции (0...8)
        uint16_t procDelay;         // Задержка первого коррелятора
        uint16_t procDelta;         // Задержки между корреляторами
        int numRangeCh;             // Кол-во дальностных каналов (1...100)
        float thres_dB;             // Порог для обнаружения в дБ
        uint32_t thres;             // Порог для обнаружения в отсчётах
        bool isCorrEnable;          // Включена ли корреляционная обработка

        // Признак ожидания данных от МЦФОС - используется для того,
        // чтобы понять, можно ли отправить на МЦФОС новое задание.
        // Сбрасывается, как только пришли данные от МЦФОС
        bool busy = false;
    };

    /**
     * @struct paramPointCalc
     * @brief Вычисленные параметры единичного замера
     */
    struct paramPointCalc
    {
        float az;       // Вычисленная горизонтальная координата замера (азимут) в градусах
        float an;       // Вычисленная вертикальная координата замера (угол места) в градусах
        float r;        // Дальность в метрах
        float rV;       // Радиальная скорость в м/с
        float aMean;    // Средняя амплитуда в отсчётах
        float aMean_dB; // Средняя амплитуда в дБ
    };

    /**
     * @struct paramSample
     * @brief Параметры единичного замера
     */
    struct paramSample
    {
        int64_t timeSys;   // Время формирования замера (системное)
        int64_t timeBs;    // Время формирования замера (по БС)

        // Исходные данные на момент формирования замера:
        paramCU cu;        // Данные ЦУ
        paramPPU ppu;      // Данные ППУ
        paramMCFOS mcfos;  // Данные МЦФОС
        QVector<QVector<float>> vecResult[4]; // Дальностно-скоростные портреты
                                              // из 4-х приёмников
        uint32_t a[4];     // Амплитуды в парциальных каналах в отсчётах
        uint32_t ri[4];    // Номера дальностных каналов в 4-х приёмниках
        uint32_t rVi[4];   // Номера скоростных каналов в 4-х приёмниках

        // Вычисленные данные замера
        paramPointCalc point;
    };

    /**
     * @struct paramRLMN
     * @brief Параметры РЛМН 0 и 1
     */
    struct paramRLMN
    {
        int mode;   // Режим работы РЛМН
                    // 0 - ожидание команд
                    // 1 - обнаружение
                    // 2 - сопровождение
                    // 3 - разовый замер

        paramCU m_paramCU;       // Параметры последнего полученного ЦУ
        paramPPU m_paramPPU;     // Параметры ППУ этого РЛМН
        paramMCFOS m_paramMCFOS; // Параметры МЦФОС этого РЛМН
        QVector<paramSample> samples; // Вектор последних замеров

        // Начальное смещение 0-го дальностного канала
        // (смещение из-за задержки сигнала в тракте -
        //  измерено отдельно для каждого РЛМН)
        uint32_t shiftNul = 96; // При нулевой задержке обработки
	};

    const int countRLMN = 2;
    paramRLMN m_paramRLMN[2]; // Структура, содержащая параметры РЛМН 0 и 1

    const float rRay = 0.3f; // Радиус луча в градусах

    // !!!!! Временная мера, пока МЦФОС не разделены по сокетам !!!!!
    //---------------------------------------------------------------
    int numLastMCFOS; // Номер РЛМН, для которого выполняется обмен с МЦФОС
    //---------------------------------------------------------------

    // Заранее рассчитанные оконные функции
    // для сигнальной обработки данных от МЦФОС
    float *windowDAC = nullptr;
    float *windowREF = nullptr;

    /**
     * @brief Генерация окна Натталла (4-терм)
     */
    float *getWindowNuttall(int length);

    /**
     * @brief Генерация окна Блэкмана-Харриса (4-терм)
     */
    float *getWindowBlackmanHarris(int length);

    /**
     * @brief Генерация окна Блэкмана (3-терм)
     */
    float *getWindowBlackman(int length);

    /**
     * @brief Генерация окна Тьюки
     */
    float *getWindowTukey(int length, const float alpha);

    /// Константа для вычисления окна Тьюки
    const float windowTukeyAlpha = 0.15f;

    /**
     * @brief Генерация оконной функции по заданным параметрам
     */
    float *getWindowFunc(int N, int windowType);

    /**
     * @enum MCFOS_WindowType
     * @brief Тип окна, используемого в МЦФОС,
     *        а также для БПФ на backend
     */
    enum MCFOS_WindowType
    {
        MCFOS_WindowType_Nuttall = 0,
        MCFOS_WindowType_BlackmanHarris = 1,
        MCFOS_WindowType_Blackman = 2,
        MCFOS_WindowType_Tukey = 3
    };

    /**
     * @brief Вычисление коэффициентов полинома ППУ
     */
    void calcCoeffPPU(double angleTarget, double velocityTarget,
                      double angleCurrent, double *angleLast,
                      double *accelerationLast, double accelerationMax,
                      double KP, double KD, double dt,
                      double *C0, double *C1, double *C2);

    struct testDataToETH
	{
		QString info = "Test struct from CALC for ETH";
    };

public slots:
	// Слоты для получения структур из других потоков
    void receiveTestToETH(const testDataToETH &testData);

    /**
     * @brief Приняты данные от МЦФОС
     */
    void recDataMCFOS(const QByteArray &data);

    /**
     * @brief Принято ЦУ от ПБУ или от РМО
     */
    void recCU(paramCU cu);

    /**
     * @brief Принято сообщение синхронизации от БС (2.5)
     */
    void recTimeBS();

    /**
     * @brief Приняты координаты ППУ от БС (11.x)
     */
    void recPosPPU(paramPPU ppu);

public:
	// Типы коллбэков
    using TestToETHCallback = std::function<void(const testDataToETH &)>;

    void setTestToETHCallback(TestToETHCallback cb);

	// Методы для передачи структур из Calculator наружу через коллбэк
	// Т.е. просто вызываем эти методы, когда нужно отправить данные
    void emitTestToETH(const testDataToETH &testData);

	// Пример использования передачи из ИНТЕРФЕЙСА в КАЛЬКУЛЯТОР:
	// Возьмём за основу TcpHandler:
	// QMetaObject::invokeMethod(
	// 	Calculator::instance(),
	// 	[cu]() { Calculator::instance()->receiveTestToTCP(cu); },
	// 	Qt::QueuedConnection
	// );

	/// Это гарантирует, что метод receiveTestToTCP будет вызван в главном потоке, где живёт Calculator, даже если вызов происходит из другого потока.
private:
	explicit Calculator(QObject *parent = nullptr);
	static Calculator *m_instance;
	static QMutex m_mutex;

    TestToETHCallback m_testToETHCallback = nullptr;
};

Q_DECLARE_METATYPE(Calculator::testDataToETH)

#endif // CALCULATOR_H
