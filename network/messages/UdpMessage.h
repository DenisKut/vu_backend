#ifndef UDPMESSAGE_H
#define UDPMESSAGE_H

#include <QByteArray>
#include <QtEndian>

/**
 * @struct UdpMessage
 * @brief Сообщение для обмена с МЦФОС (UDP или TCP)
 */
struct UdpMessage
{
	quint64 timestamp;
	QByteArray payload;
};

/**
 * @struct UdpMessage_MCFOS_Cmd
 * @brief Командное сообщение на МЦФОС (UDP или TCP)
 */
struct UdpMessage_MCFOS_Cmd
{
	uint32_t reserv : 20;	 ///< Резерв (запоняется нулями)
	uint32_t CMD_C_SH : 4;	 ///< Сдвиг разрядности корреляторов
	uint32_t CMD_COR_EN : 1; ///< Включение корреляционной обработки сигнала
	uint32_t CMD_ERR_R : 1;	 ///< Чтение регистра ошибок записи DDR
	uint32_t CMD_DDR_R : 1;	 ///< Чтение данных из DDR
	uint32_t CMD_BUF_R : 1;	 ///< Сброс I и Q буферов ЦАП и REF
	uint32_t CMD_MUX : 2;	 ///< Переключатель захвата данных в DDR:
							 ///<   0 — захват данных с АЦП,
							 ///<   1 — захват тестовых данных (счетчик),
							 ///<   2 — Захват данных с ЦАП
	uint32_t CMD_DAC_S : 1;	 ///< Cтарт выдачи данных на ЦАП из буфера ЦАП
	uint32_t CMD_ADC_S : 1;	 ///< Cтарт записи даннных из АЦП в DDR

	uint32_t DDR_ADDR;			///< Адрес начала записи в DDR
	uint32_t DDR_LEN;			///< Кол-во записываемых данных (байтов)
	uint32_t PERIOD_SIZE : 16;	///< Кол-во периодов в пачке
	uint32_t PERIOD_COUNT : 16; ///< Длительность одного периода в 10 нс
	uint32_t AMP_OFF;			///< Длительность сигнала разрешения
								///<   от начала периода
	uint32_t PULSE_ON : 16;		///< Задержка окончания выдачи сигнала после
								///<   начала периода
	uint32_t PULSE_OFF : 16;	///< Задержка выдачи сигнала после начала периода
	uint32_t PROC_DELAY : 16;	///< Задержка старта следующего коррелятора
	uint32_t PROC_DELTA : 16;	///< Задержка старта корреляционной обработки
	uint32_t BUF_SIZE;			///< Количество дополнительных слов в данном пакете,
								///<   которые являются отсчетами для ЦАП
	UdpMessage_MCFOS_Cmd() : reserv(0),
							 CMD_C_SH(0),
							 CMD_COR_EN(0),
							 CMD_ERR_R(0),
							 CMD_DDR_R(0),
							 CMD_BUF_R(0),
							 CMD_MUX(0),
							 CMD_DAC_S(0),
							 CMD_ADC_S(0),
							 DDR_ADDR(0),
							 DDR_LEN(0),
							 PERIOD_SIZE(0),
							 PERIOD_COUNT(0),
							 AMP_OFF(0),
							 PULSE_ON(0),
							 PULSE_OFF(0),
							 PROC_DELAY(0),
							 PROC_DELTA(0),
							 BUF_SIZE(0) {}

	// Методы для сериализации/десериализации
	//    QByteArray serialize() const {
	//        return QByteArray(reinterpret_cast<const char*>(this), sizeof(UdpMessage_MCFOS_Cmd));
	//    }
	QByteArray serialize() const
	{
		QByteArray data;
		data.resize(8 * sizeof(8));

		// Извлекаем битовые поля в явные переменные
		const quint32 cmd =
			static_cast<quint32>(CMD_ADC_S) |
			(static_cast<quint32>(CMD_DAC_S) << 1) |
			(static_cast<quint32>(CMD_MUX) << 2) |
			(static_cast<quint32>(CMD_BUF_R) << 4) |
			(static_cast<quint32>(CMD_DDR_R) << 5) |
			(static_cast<quint32>(CMD_ERR_R) << 6) |
			(static_cast<quint32>(CMD_COR_EN) << 7) |
			(static_cast<quint32>(CMD_C_SH) << 8) |
			(static_cast<quint32>(reserv) << 12);
		const quint32 period =
			static_cast<quint32>(PERIOD_SIZE) |
			(static_cast<quint32>(PERIOD_COUNT) << 16);
		const quint32 pulse =
			static_cast<quint32>(PULSE_ON) |
			(static_cast<quint32>(PULSE_OFF) << 16);
		const quint32 proc =
			static_cast<quint32>(PROC_DELAY) |
			(static_cast<quint32>(PROC_DELTA) << 16);

		// Сериализация с little-endian
		qToLittleEndian<quint32>(cmd, data.data() + 0 * sizeof(quint32));
		qToLittleEndian<quint32>(DDR_ADDR, data.data() + 1 * sizeof(quint32));
		qToLittleEndian<quint32>(DDR_LEN, data.data() + 2 * sizeof(quint32));
		qToLittleEndian<quint32>(period, data.data() + 3 * sizeof(quint32));
		qToLittleEndian<quint32>(AMP_OFF, data.data() + 4 * sizeof(quint32));
		qToLittleEndian<quint32>(pulse, data.data() + 5 * sizeof(quint32));
		qToLittleEndian<quint32>(proc, data.data() + 6 * sizeof(quint32));
		qToLittleEndian<quint32>(BUF_SIZE, data.data() + 7 * sizeof(quint32));

		return data;
	}
};

#endif // UDPMESSAGE_H
