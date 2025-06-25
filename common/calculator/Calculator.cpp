#include "Calculator.h"
#include <QMutexLocker>

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
	qRegisterMetaType<Calculator::testToETH>("Calculator::testToETH");
	qRegisterMetaType<Calculator::testToTCP>("Calculator::testToTCP");
	qRegisterMetaType<Calculator::testToUDP>("Calculator::testToUDP");
}

void Calculator::setTestToETHCallback(TestToETHCallback cb)
{
	m_testToETHCallback = std::move(cb);
}
void Calculator::setTestToTCPCallback(TestToTCPCallback cb)
{
	m_testToTCPCallback = std::move(cb);
}
void Calculator::setTestToUDPCallback(TestToUDPCallback cb)
{
	m_testToUDPCallback = std::move(cb);
}

void Calculator::emitTestToETH(const testToETH &test)
{
	if (m_testToETHCallback)
		m_testToETHCallback(test);
}

void Calculator::emitTestToTCP(const testToTCP &test)
{
	if (m_testToTCPCallback)
		m_testToTCPCallback(test);
}

void Calculator::emitTestToUDP(const testToUDP &test)
{
	if (m_testToUDPCallback)
		m_testToUDPCallback(test);
}

void Calculator::receiveTestToETH(const testToETH &test)
{
	// Здесь обработка структуры из других потоков
	// Например, сохранить, обработать, и т.д.
	// ...
	// Можно вызвать коллбэк, если он установлен
	// emitTestToETH(test);
}

void Calculator::receiveTestToTCP(const testToTCP &test)
{
	// Здесь обработка структуры из других потоков
	// Например, сохранить, обработать, и т.д.
	// ...

	// Если нужно сразу отправить данные, можно вызвать коллбэк
	// emitTestToTCP(test);
}

void Calculator::receiveTestToUDP(const testToUDP &test)
{
	// Здесь обработка структуры из других потоков
	// Например, сохранить, обработать, и т.д.
	// ...

	// Если нужно сразу отправить данные, можно вызвать коллбэк
	// emitTestToUDP(test);
}
