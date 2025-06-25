# Logger

## Что из себя представляет Logger
> [!TIP]
> **Логгер** является критически важным компонентом архитектуры продукта. В его задачи входит запись всех аспектов жизненного цикла программы в **бинарные файлы специального формата**.
## Ключевые особенности реализации
### Архитектура системы логирования
1. **Бинарный формат хранения**
2. **Асинхронная запись**
   - Отдельный поток для операций ввода-вывода
   - Очередь сообщений между рабочими потоками и логгером
   - Гарантированная запись без блокировки основного потока
3. **Гибкая система типов логов**
   - Поддержка кастомных типов сообщений
   - Иерархия классов логов с общим базовым интерфейсом
   - Регистрация новых типов через фабрику

### Управление логами
- Автоматическое именование файлов по шаблону `ГГГГММДД_ЧЧММСС_log.bin`
- Возможность принудительной смены файла лога во время работы
- Инструменты чтения/анализа бинарных логов (отдельная утилита)


## Содержание элементов Logger
### Структура Логгера
```
├── common
│   └── logger
│       ├── LogFactory.cpp
│       ├── LogFactory.h
│       ├── LoggerMacros.h
│       ├── LogManager.cpp
│       ├── LogManager.h
│       ├── LogSession.cpp
│       ├── LogSession.h
│       ├── LogStorage.cpp
│       ├── LogStorage.h
│       └── messageLogs
│           ├── InitLog.cpp
│           ├── InitLog.h
│           ├── InternalMessageLog.cpp
│           ├── InternalMessageLog.h
│           ├── LogEnums.h
│           ├── LogHeader.cpp
│           ├── LogHeader.h
│           ├── ProtocolMessageLog.cpp
│           ├── ProtocolMessageLog.h
│           └── PrtUtils.h
├── docs
│   └── logger_man.md
├── pch.cpp
└── pch.h
...
```

## Как логировать события
> [!IMPORTANT]
> ### Рекомендуется использовать 1-ый способ (без макросов), так как он универсальный

> [!NOTE]
> ### Изменение названия файлов логирования
> в logManager.h прописан класс LogUtils, в нем есть метод generateLogFileName()
> если хочется другой формат названия, то изменять его
> ```cpp
> class LogUtils
>{
>public:
>    static QString generateLogFileName()
>    {
>        return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_log.bin";
>    }
>};
>```

### 1. Создание лога (в коде приложения) без макросов выглядит следующим образом
Используется вызов функции `LogManager::instance().log(new ТипЛога(параметры конструктора))`<br>

Параметры конструкторов разных типов логов описаны в файлах соотвествующих типов логов в папке `common/logger/messageLogs`
```cpp
LogManager::instance().log(new ТипЛога(параметр_1,параметр_2...,параметр_N));
```
Пример вызова лога с типом ProtocolMessageLog:
```cpp
//допустим у нас есть некоторое сообщение msgEth типа EthMessage_BS_SYNC , которое мы сериализуем        
auto msgEth = EthMessage_BS_SYNC(); //создаем через базовый конструктор
//как-нибудь его заполняем
msgEth.periodNumber = 666;
//сериализуем 
auto dataEth = msgEth.serializeFull();
//Передаем это сообщение (dataEth) в сериализованном виде в тип лога ProtocolMessageLog
//CLASS_NAME - макрос для определения имени класса из котрого вызывается лог, возвращает QString()
//на месте CLASS_NAME можно написать, что-нибудь другое, если нужно
LogManager::instance().log(new ProtocolMessageLog(CLASS_NAME,                
                                                  LogLevel::MessageReceived,
                                                  Protocol::ETH_BINARY,
                                                  dataEth,                  //тут сообщение в сериализованном виде, в формате QByteArray
                                                  "parsedData/пустая строка/любая строковая отладочная информация"));
```
Пример создания лога, его заполнения и вызова функции логирования:
```cpp
// 1. Создаем объект через базовый конструктор
ProtocolMessageLog* ethLog = new ProtocolMessageLog(); // Вызовет ProtocolMessageLog() : LogHeader("", LogLevel::MessageReceived)

// 2. Заполняем базовые поля из LogHeader
ethLog->src = CLASS_NAME;
ethLog->level = LogLevel::MessageReceived;
ethLog->timestampMicroseconds = QDateTime::currentMSecsSinceEpoch() * 1000; //ну или любой нужный timeStampMicroseconds

// 3. Заполняем специфичные для ProtocolMessageLog поля
ethLog->protocol = Protocol::ETH_BINARY;
ethLog->rawData = dataEth; // QByteArray с бинарными данными
ethLog->parsedData = ""; 

// 4. Передаем в логгер
LogManager::instance().log(ethLog);
```
> [!IMPORTANT]
> ### Protocol
> Важно указать правильный Protocol, так как от этого зависит метод парсинга сообщения в читалке (Reader)
> Если будет выбран неправильный протокол, то скорее всего читалка крашнется

### 2. Про макросы
В [прекомпилируемом заголовке](../pch.h) описаны макросы, с помощью которых можно логировать различные аспекты ПО: **инициализация**, **события**, **отправка**/**приём сообщений**
Рассмоотрим подробнее доступные варианты и синтаксис команд:
> [!IMPORTANT]
> ##### Макросы
> * LOG
> * LOG_INIT
> * LOG_INTERNAL
> * LOG_PROTOCOL
 
<details>
<table class="tg"><thead>
  <tr>
    <th class="tg-0pky">Макрос</th>
    <th class="tg-0pky">Принимаемые параметры</th>
    <th class="tg-0pky">Расшифровка параметров</th>
    <th class="tg-0pky">Параметры для параметров</th>
  </tr></thead>
<tbody>
  <tr>
    <td class="tg-0lax" rowspan="3">LOG</td>
    <td class="tg-0pky">* Logger::InitLog</td>
    <td class="tg-0pky">Структура с данными инициализации компонента</td>
    <td class="tg-0pky">* source - QString. Использовать CLASS_NAME - макрос, возвращающий QString - имя класса<br>* &lt;опционально&gt; logLevel - Logger::LogLevel - enum, тип логирования<br>* statusInit - Logger::StatusInit - enum, статус инициализации</td>
  </tr>
  <tr>
    <td class="tg-0pky">* Logger::InternalMessageLog</td>
    <td class="tg-0pky">Структура с данными необходимых событий внутри программы</td>
    <td class="tg-0pky"><span style="font-weight:400;font-style:normal">source - QString. Использовать </span>CLASS_NAME - макрос, возвращающий QString - имя класса<br><span style="font-weight:400;font-style:normal">&lt;опционально&gt; </span>logLevel - Logger::LogLevel - enum, тип логирования<br>messageType - QString тип сообщения, или заголовок события<br>messageItself - QString, подробное описание проблемы/события</td>
  </tr>
  <tr>
    <td class="tg-0lax"><span style="font-weight:400;font-style:normal">* Logger::ProtocolMessageLog</span></td>
    <td class="tg-0lax">Структура с информацией о сообщениях, с которыми работает программа</td>
    <td class="tg-0lax">* source - QString. Использовать CLASS_NAME - макрос, возвращающий QString - имя класса<br>* &lt;опционально&gt; logLevel - Logger::LogLevel - enum, тип логирования<br>* protocol - Logger::Protocol, протокол сообщения<br>* rawData - QByteArray - сырой набор данных, который отправляется/принимается по проводам<br>* jsonData - QString - сырые данные, собранные в структуру, а затем упакованные в формат json</td>
  </tr>
  <tr>
    <td class="tg-0lax">LOG_INIT</td>
    <td class="tg-0lax"><span style="font-weight:400;font-style:normal">* `QString,</span> Logger::LogLevel, Logger::StatusInit`<br><span style="font-weight:400;font-style:normal">* `QString, Logger::StatusInit`</span></td>
    <td class="tg-0lax">см. раздел [Принимаемые параметры], секция [Logger::InitLog] -&gt; Параметры для параметров</td>
    <td class="tg-0lax">* source - QString. Использовать CLASS_NAME - макрос, возвращающий QString - имя класса<br>* &lt;опционально&gt; logLevel - Logger::LogLevel - enum, тип логирования<br>* statusInit - Logger::StatusInit - enum, статус инициализации</td>
  </tr>
  <tr>
    <td class="tg-0lax">LOG_INTERNAL</td>
    <td class="tg-0lax"><span style="font-weight:400;font-style:normal">* `</span>QString, Logger::LogLevel, QString, QString<span style="font-weight:400;font-style:normal">`</span><br><span style="font-weight:400;font-style:normal">* `</span>QString, QString, QString`<br></td>
    <td class="tg-0lax">см. раздел [Принимаемые параметры], секция [Logger::InternalMessageLog] -&gt; Параметры для параметров</td>
    <td class="tg-0lax"><span style="font-weight:400;font-style:normal">source - QString. Использовать </span>CLASS_NAME - макрос, возвращающий QString - имя класса<br><span style="font-weight:400;font-style:normal">&lt;опционально&gt; </span>logLevel - Logger::LogLevel - enum, тип логирования<br>messageType - QString тип сообщения, или заголовок события<br>messageItself - QString, подробное описание проблемы/события</td>
  </tr>
  <tr>
    <td class="tg-0lax">LOG_PROTOCOL</td>
    <td class="tg-0lax">* `QString, Logger::LogLevel, Logger::Protocol, QByteArray, QString`<br><span style="font-weight:400;font-style:normal">* `QString,</span> Logger::Protocol, QByteArray, QString`<br></td>
    <td class="tg-0lax">см. раздел [Принимаемые параметры], секция [Logger::ProtocolMessageLog] -&gt; Параметры для параметров</td>
    <td class="tg-0lax">* source - QString. Использовать CLASS_NAME - макрос, возвращающий QString - имя класса<br>* &lt;опционально&gt; logLevel - Logger::LogLevel - enum, тип логирования<br>* protocol - Logger::Protocol, протокол сообщения<br>* rawData - QByteArray - сырой набор данных, который отправляется/принимается по проводам<br>* jsonData - QString - сырые данные, собранные в структуру, а затем упакованные в формат json. Для конвертирования из QJsonObject в QString использовать `Logger::jobj_toString`</td>
  </tr>
</tbody></table>
</details>

> [!NOTE]
> Для логирования случая достаточно описать макрос после события
---
<details>
<summary>Как это работает?</summary>

---
## 1. Инициализация системы
```cpp
// При первом обращении к LogManager:
LogSession& LogManager::instance() {
    static auto session = createInstance();  // Автосоздание сессии
    return *session;
}
```
  - Создается рабочая сессия (LogSession)
    - Запускается отдельный поток для записи логов
  - Регистрируются типы логов в фабрике
    - InitLog - Структура с данными инициализации компонента
    - InternalMessageLog - Структура с данными необходимых событий внутри программы
    - ProtocolMessageLog - Структура с информацией о сообщениях, с которыми работает программа
    ```cpp
    void LogSession::factoryInit() {
        factory.registerType<InitLog>(LogType::Init);
        factory.registerType<InternalMessageLog>(LogType::InternalMessage);
        factory.registerType<ProtocolMessageLog>(LogType::ProtocolMessage);
        ...
        //регистрация нового типа логов
        factory.registerType<НовыйТипЛога>(LogType::НовыйТипЛога);
    }
    ```
## 2. Создание объекта лога
Пользователь создает любой наследник `LogHeader`
```cpp
auto logEntry = new CustomLog(
    source,         // Источник сообщения
    logLevel,       // Уровень важности
    ...             // Дополнительные параметры
);
```
## 3. Передача лога в функцию
Вызов основного метода логирования
```cpp
LogManager::instance().log(logEntry);
```
Объект передается в синглтон LogSession
  - Отправляется сигнал logRequested
  - Владение объектом передается в рабочий поток
## 4. Обработка
```cpp
void LogSession::handleLogRequest(LogHeader* log) {
    if(log == nullptr) return;
    std::unique_ptr<LogHeader> logPtr(log);
    if (!shuttingDown.load(std::memory_order_acquire))
        storage.write(*logPtr);
}
```
- Гарантированное освобождение памяти через unique_ptr
- Изоляция основного потока от операций I/O
## 5. Сериализация
Внутри метода storage.write вызываются перегруженные операторы <<, в которых
вызывается метод serialize() соответствующего типа логов
```cpp
// Общий алгоритм сериализации:
void LogHeader::serialize(QDataStream& stream) const {
    stream << timestampMicroseconds << src.toUtf8() << static_cast<quint8>(level);
}

QDataStream& operator<<(QDataStream& stream, const LogHeader& header) {
    header.serialize(stream);
    return stream;
}

// Для производных классов:
void CustomLog::serialize(QDataStream& stream) const {
    LogHeader::serialize(stream);  // Базовые поля
    stream << customField1 << customField2;  // Специфичные данные
}

QDataStream& operator<<(QDataStream& stream, const CustomLog& log) {
    log.serialize(stream);
    return stream;
}
```
## 6. Запись в файл
```cpp
bool LogStorage::write(const LogHeader& log) {
    QMutexLocker locker(&m_writeMutex);

    QFile file(filename);
    if (!file.open(QIODevice::Append | QIODevice::WriteOnly)) return false;

    QDataStream stream(&file);
    stream.setVersion(streamVersion);
    stream.setByteOrder(streambyteOrder);


    stream << static_cast<quint8>(log.logType()) << log;
    return stream.status() == QDataStream::Ok;
}
```
**Формат записи**
```
[Тип лога: 1 байт][Данные: переменная длина]
[тип_лога (quint8)] [данные_лога (сериализованные)]
```
</details>

---
