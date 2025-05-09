# Logger
<br>

## Что из себя представляет Logger
> [!TIP]
> `Logger` в контексте проекта `vu_backend` считается одним из важных узлов данного продукта. В его задачи входит запись всех аспектов жизненного цикла этой программы в базу данных. В качестве рассмотренной системы сохранения и накапливания данных была выбрана база данных PostgreSQL, под управлением контейнеризации docker. Про управление ядром БД и систем СУБД руководствоваться [этим](docker_man.md) документом.

## Содержание элементов Logger
### Структура Логгера
```
├── core
│   └── logger
│       ├── Logger.cpp
│       └── Logger.h
├── docs
│   └── logger_man.md
├── pch.cpp
├── pch.h
...
```

## Как логировать события
В [прекомпилируемом заголовке](../pch.h) описаны макросы, с помощью которых можно логировать различные аспекты ПО: **инициализация**, **события**, **отправка**/**приём сообщений**

Рассмоотрим подробнее доступные варианты и синтаксис команд:
> [!IMPORTANT]
> ##### Макросы
> * LOG
> * LOG_INIT
> * LOG_INTERNAL
> * LOG_PROTOCOL
 
<details>
<style type="text/css">
.tg  {border-collapse:collapse;border-spacing:0;}
.tg td{border-color:black;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;
  overflow:hidden;padding:10px 5px;word-break:normal;}
.tg th{border-color:black;border-style:solid;border-width:1px;font-family:Arial, sans-serif;font-size:14px;
  font-weight:normal;overflow:hidden;padding:10px 5px;word-break:normal;}
.tg .tg-0pky{border-color:inherit;text-align:left;vertical-align:top}
.tg .tg-0lax{text-align:left;vertical-align:top}
</style>
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

При  запуске программы считывается данные подключения из `config.ini` - конфигурационный файл с настройками для программы.
Формируется строка подключения и передается в `Logger::initialize`. Внутри  выполняется попытка подключения и, если этого не было сделано ранее, создание структуры БД.

Все сообщения помещаются в структуру `log(T) -> enqueueMessage(bakeQuery(T))`, где `Logger::bakeQuery` - конвертер структуры `T` в запрос в соответствии с sql синтаксисом; enqueueMessage кладёт в `QQueue<QString> m_queryQueue`
Запросы размещаются в пачку m_buffer ( `m_queryQueue` внутри `run()` размещаются в `m_buffer`), и при достижении `m_bufferSize` == 100 выполняется `flushBuffer` - выполнение транзакции по отправки данных в БД. 

> [!WARNING]
> В сборке debug отправка в БД происходит на каждое сообщение + вывод запросника в консоль
> Может вызывать проблемы с производительностью при частой отправке. 
> Настоятельно рекомендуется придерживаться концепции "отправка в пачке"
</details>

---

## Для справки

* БД - База Данных. У нас это `postgreSQL`
* СУБД - Система Управления Базами Данных. У нас - `Adminer`