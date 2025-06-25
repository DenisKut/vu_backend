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

	struct Point
	{
	};

	// Параметры ЦУ
	struct paramCU
	{
		int64_t time; // Время получения замера (системное)

		double az; // Азимут
		double an; // Угол места
		double r;  // Дальность

		double azV; // Скорость изменения азимута
		double anV; // Скорость изменения угла места
		double rV;	// Скорость изменения дальности

		double azEr; // Ошибка по азимуту
		double anEr; // Ошибка по углу места
		double rEr;	 // Ошибка по дальности

		// Информационные параметры (тип цели и т.д.)
	};

	// Параметры ППУ
	struct paramPPU
	{
		int number;	  // Номер ППУ
		int64_t time; // Время обновления координат от ППУ (системное)
		paramCU m_paramCU;
	};

	struct testToETH
	{
		QString info = "Test struct from CALC for ETH";
	};

	struct testToTCP
	{
		QString info = "Test struct from CALC for TCP";
	};

	struct testToUDP
	{
		QString info = "Test struct from CALC for UDP";
	};
public slots:
	// Слоты для получения структур из других потоков
	void receiveTestToETH(const testToETH &test);
	void receiveTestToTCP(const testToTCP &test);
	void receiveTestToUDP(const testToUDP &test);

public:
	// Типы коллбэков
	using TestToETHCallback = std::function<void(const testToETH &)>;
	using TestToTCPCallback = std::function<void(const testToTCP &)>;
	using TestToUDPCallback = std::function<void(const testToUDP &)>;

	void setTestToETHCallback(TestToETHCallback cb);
	void setTestToTCPCallback(TestToTCPCallback cb);
	void setTestToUDPCallback(TestToUDPCallback cb);

	// Методы для передачи структур из Calculator наружу через коллбэк
	// Т.е. просто вызываем эти методы, когда нужно отправить данные
	void emitTestToETH(const testToETH &test);
	void emitTestToTCP(const testToTCP &test);
	void emitTestToUDP(const testToUDP &test);

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
	TestToTCPCallback m_testToTCPCallback = nullptr;
	TestToUDPCallback m_testToUDPCallback = nullptr;
};

Q_DECLARE_METATYPE(Calculator::testToETH)
Q_DECLARE_METATYPE(Calculator::testToTCP)
Q_DECLARE_METATYPE(Calculator::testToUDP)

#endif // CALCULATOR_H
