Выбор между `QSharedPointer` и `std::unique_ptr` зависит от контекста использования и требований проекта. Вот ключевые аспекты для вашего случая (Qt5 + C++11):

---

### **1. `QSharedPointer` (Qt)**

#### **Преимущества:**

- **Совместимость с Qt**:

  - Интеграция с сигналами/слотами, где требуется копирование указателей.
  - Работа с Qt-контейнерами (например, `QList<QSharedPointer<T>>`).
  - Безопасное использование в многопоточных сценариях (если объект используется в разных потоках).

- **Разделяемое владение**:

  - Объект живёт, пока существует хотя бы один `QSharedPointer` на него.
  - Удобно, если сообщение должно обрабатываться в нескольких местах (например, кэшироваться, передаваться между модулями).

- **Автоматическое управление временем жизни**:

  - Не нужно явно передавать владение, как в `std::unique_ptr`.

- **Совместимость с другими Qt-классами**:
  - Например, `QVariant::fromValue(QSharedPointer<T>)`.

#### **Недостатки:**

- **Накладные расходы**:
  - Подсчёт ссылок (минимальный, но может быть важен в high-performance сценариях).
- **Риск циклических ссылок**:
  - Если два объекта держат `QSharedPointer` друг на друга, они никогда не удалятся.

---

### **2. `std::unique_ptr` (C++11)**

#### **Преимущества:**

- **Производительность**:
  - Нет накладных расходов на подсчёт ссылок (владелец всегда один).
- **Чёткое владение**:

  - Владелец объекта явно определён, что снижает риск ошибок.
  - Компилятор проверяет исключительное владение.

- **Совместимость со стандартной библиотекой**:
  - Лучше интегрируется с STL-контейнерами (`std::vector<std::unique_ptr<T>>`).

#### **Недостатки:**

- **Сложность передачи владения**:

  - Для передачи объекта нужно использовать `std::move()`, что может усложнить код.
  - Не подходит для сценариев, где объект должен быть доступен из нескольких мест.

- **Ограниченная интеграция с Qt**:
  - Нельзя напрямую передать `std::unique_ptr` в сигнал/слот.
  - Сложности с использованием в Qt-контейнерах.

---

### **Когда что выбрать?**

#### **`QSharedPointer` лучше, если:**

- Сообщения используются в нескольких модулях/потоках.
- Нужна интеграция с Qt-механизмами (например, сигналы/слоты).
- Время жизни объекта неочевидно (например, кэширование).

#### **`std::unique_ptr` лучше, если:**

- Владение объектом строго определено (например, фабрика создаёт сообщение и передаёт его в сетевой модуль).
- Требуется максимальная производительность.
- Используются STL-контейнеры.

---

### **Рекомендация для вашего проекта**

Учитывая, что:

1. Это проект на **Qt5**,
2. Сообщения могут передаваться между разными компонентами (например, сетевой модуль → обработчик → кэш),
3. Возможно использование сигналов/слотов для передачи сообщений,

**`QSharedPointer` будет более удобным и естественным выбором**.

Однако, если вы точно знаете, что:

- Сообщение обрабатывается только в одном месте,
- Время жизни объекта полностью контролируется,
- Нет необходимости в интеграции с Qt-механизмами,

то можно использовать `std::unique_ptr` для оптимизации.

---

### **Примеры использования**

#### С `QSharedPointer`:

```cpp
// Передача сообщения в сигнале
QSharedPointer<TcpMessage_0x01> msg = MessageFactory::create<TcpMessage_0x01>();
emit messageReceived(msg); // Копирование безопасно

// Хранение в Qt-контейнере
QList<QSharedPointer<TcpHeader>> messagesCache;
messagesCache.append(msg);
```

#### С `std::unique_ptr`:

```cpp
// Создание и передача владения
std::unique_ptr<TcpMessage_0x01> msg = MessageFactory::create<TcpMessage_0x01>();
networkModule.send(std::move(msg)); // Владение передано

// Хранение в STL-контейнере
std::vector<std::unique_ptr<TcpHeader>> messages;
messages.push_back(std::move(msg));
```

---

### **Итог**

Для проекта на Qt5 с неочевидным временем жизни объектов и необходимостью интеграции с фреймворком **`QSharedPointer` предпочтительнее**. Если же критична производительность и владение явное — используйте `std::unique_ptr`. В смешанных сценариях можно применять оба подхода для разных типов сообщений.

> [!NOTE]
> Если говорить об использовании QSharedPointer, то на примере TcpMessage.h были отлажены следующие блоки кода
>
> ```
> #include <QSharedPointer>
> ...
> // И заменить одноимённые процедуры на:
> /**
>   * @brief Создание сообщения указанного типа (по-умолчанию)
>   * @tparam T Тип сообщения (должен наследовать от TcpHeader)
>   * @return Экземпляр сообщения
>   */
>   template<typename T>
>   static typename std::enable_if<
>       std::is_base_of<TcpHeader, T>::value,
>       QSharedPointer<T>
>       >::type create() {
>       return QSharedPointer<T>::create();
>   }
>
>    /**
>     * @brief Создание сообщения с произвольными параметрами
>     * @tparam T Тип сообщения (должен наследовать от TcpHeader)
>     * @tparam... Args Произвольные аргументы для конструкторов сообщений,
>     * которым необходимы параметры
>     * @return Экземпляр сообщения
>     */
>    template<typename T, typename... Args>
>    static typename std::enable_if<
>        std::is_base_of<TcpHeader, T>::value,
>        QSharedPointer<T>
>        >::type create(Args&&... args) {
>        return QSharedPointer<T>::create(std::forward<Args>(args)...);
>    }
>
>    /**
>     * @brief Парсит входящие данные в сообщение
>     * @param data Входные данные
>     * @return Указатель на сообщение или указатель на заголовок
>     */
>    static QSharedPointer<TcpHeader> parse(const QByteArray& data) {
>        if (data.size() < static_cast<int>(sizeof(TcpHeader))) {
>            return QSharedPointer<TcpHeader>();
>        }
>
>        TcpHeader header = TcpHeader::deserialize(data);
>        if (!header.isValid()) {
>            return QSharedPointer<TcpHeader>();
>        }
>
>        switch (static_cast<PBU_MessageNumbers>(header.typePack)) {
>        case PBU_MessageNumbers::REQ_REGISTRATION:
>            return parseMessage<TcpMessage_0x01>(data);
>
>        // Добавить обработку других типов...
>
>        default:
>            return QSharedPointer<TcpHeader>();
>        }
>    }
> 	...
> private:
>   /**
>    * @brief Парсит сообщение типа T из данных
>    * @private
>    * @tparam T Тип сообщения (наследник TcpHeader)
>    * @param data Входные данные
>    * @return Уникальный указатель на сообщение или nullptr при ошибке
>    */
>   template<typename T>
>   static QSharedPointer<TcpHeader> parseMessage(const QByteArray& data) {
>       if (data.size() < static_cast<int>(sizeof(T))) {
>           return QSharedPointer<TcpHeader>();
>       }>
>       QSharedPointer<T> msg = QSharedPointer<T>::create();
>       memcpy(msg.data(), data.constData(), sizeof(T));>
>       return msg.template staticCast<TcpHeader>();
>   }
> ```

---

### Подход к реализации сообщения TcpMessage_0x21 через std::vector

```
#pragma pack(push, 1)

// Элемент статуса (как ранее)
struct ElementStatus {
    uint16_t IDElem;
    uint8_t status:4;
    uint8_t workFl:4;  
    uint8_t local:1;
    uint8_t isImit:1;
    uint8_t reserv:6;
};

// Основная структура сообщения (только заголовок)
struct TcpMessage_0x21_Header : TcpMessage<TcpMessage_0x21_Header, 
                                         PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS> {
    uint32_t time1;
    uint32_t time2;
    
    TcpMessage_0x21_Header() {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);
    }
};

// Обертка для удобной работы
class TcpMessage_0x21 {
    TcpMessage_0x21_Header header;
    std::vector<ElementStatus> elements;
    
public:
    // Конструкторы
    TcpMessage_0x21() = default;
    
    explicit TcpMessage_0x21(const std::vector<ElementStatus>& elems)
        : elements(elems) {
        updateSize();
    }
    
    // Управление элементами
    void setElements(const std::vector<ElementStatus>& elems) {
        elements = elems;
        updateSize();
    }
    
    void addElement(const ElementStatus& elem) {
        elements.push_back(elem);
        updateSize();
    }
    
    const std::vector<ElementStatus>& getElements() const { return elements; }
    
    // Сериализация
    QByteArray serializeFull() const {
        QByteArray data;
        data.append(reinterpret_cast<const char*>(&header), sizeof(header));
        data.append(reinterpret_cast<const char*>(elements.data()), 
                   elements.size() * sizeof(ElementStatus));
        return data;
    }
    
    // Десериализация
    static TcpMessage_0x21 deserialize(const QByteArray& data) {
        TcpMessage_0x21 msg;
        
        if (data.size() >= static_cast<int>(sizeof(TcpMessage_0x21_Header))) {
            // Читаем заголовок
            memcpy(&msg.header, data.constData(), sizeof(TcpMessage_0x21_Header));
            
            // Читаем элементы
            const size_t elementsSize = (data.size() - sizeof(TcpMessage_0x21_Header)) / 
                                      sizeof(ElementStatus);
            msg.elements.resize(elementsSize);
            
            if (elementsSize > 0) {
                memcpy(msg.elements.data(), 
                      data.constData() + sizeof(TcpMessage_0x21_Header),
                      elementsSize * sizeof(ElementStatus));
            }
        }
        
        return msg;
    }
    
private:
    void updateSize() {
        header.sizeData = sizeof(TcpMessage_0x21_Header) - sizeof(TcpHeader) + 
                         elements.size() * sizeof(ElementStatus);
    }
};

#pragma pack(pop) 
```

### Реализация с динамическим массивом:
 ```  
 #pragma pack(push, 1)
 
 /**
  * @struct ElementStatus
  * @brief Статус одного элемента схемы
  */
 struct ElementStatus {
     uint16_t IDElem;    ///< Условный номер элемента (0-65535)
 
     uint8_t status:4;   ///< Состояние элемента (0-неизвестно, 1-норма,
                         ///< 2-предупреждение, 3-авария)
     uint8_t workFl:4;   ///< Статус работы (0-выключен, 1-включен, 2-включение,
                         ///< 3-выключение)
 
     uint8_t local:1;    ///< Признак местного режима (0/1)
     uint8_t isImit:1;   ///< Признак имитатора (0/1)
     uint8_t reserv:6;   ///< Резервные биты
 };
 
 /**
  * @struct TcpMessage_0x21
  * @brief Сообщение статуса элементов схемы (0x21)
  */
 struct TcpMessage_0x21 : TcpMessage<TcpMessage_0x21,
         PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS> {
     uint32_t time1;     ///< Младшие 32 бита временной метки
     uint32_t time2;     ///< Старшие 32 бита временной метки
 
     ElementStatus* elements; ///< Динамический массив статусов
 
     /**
      * @brief Конструктор
      */
     TcpMessage_0x21() : elements(nullptr) {
         uint64_t time = to_uint64(TimeProvider::instance().getMs());
         time1 = to_uint32(time & 0xFFFFFFFF);
         time2 = to_uint32(time >> 32);
     }
 
     /**
      * @brief Деструктор
      */
     ~TcpMessage_0x21() {
         delete[] elements;
     }
 
     /**
      * @brief Добавляет статусы элементов в сообщение
      * @param statuses Массив статусов
      * @param count Количество элементов
      */
     void setStatuses(const ElementStatus* statuses, size_t count) {
         delete[] elements;
         elements = nullptr;
 
         if (count > 0) {
             elements = new ElementStatus[count];
             std::copy(statuses, statuses + count, elements);
             sizeData = sizeof(TcpMessage_0x21) - sizeof(TcpHeader) +
                        count * sizeof(ElementStatus);
         } else {
             sizeData = sizeof(TcpMessage_0x21) - sizeof(TcpHeader);
         }
     }
 
     /**
      * @brief Возвращает количество элементов в сообщении
      */
     size_t getElementsCount() const {
         return (sizeData - (sizeof(TcpMessage_0x21) - sizeof(TcpHeader))) /
                sizeof(ElementStatus);
     }
 
     /**
      * @brief Полная сериализация сообщения
      */
     QByteArray serializeFull() const {
         QByteArray data;
         const size_t headerSize = sizeof(TcpMessage_0x21);
         const size_t elementsSize = getElementsCount() * sizeof(ElementStatus);
 
         data.append(reinterpret_cast<const char*>(this), headerSize);
         if (elements && elementsSize > 0) {
             data.append(reinterpret_cast<const char*>(elements), elementsSize);
         }
 
         return data;
     }
 
 /// @warning Отключение диагностики комплилятора для метода копирования
 /// @note Мы сами перепроверяем строку при её сериализации/десериализации
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wclass-memaccess"
     /**
      * @brief Десериализация из массива байт
      */
     static TcpMessage_0x21 deserialize(const QByteArray& data) {
         TcpMessage_0x21 msg;
 
         const size_t headerSize = sizeof(TcpMessage_0x21);
         if (data.size() >= static_cast<int>(headerSize)) {
             /// @note Копирование заголовка
             memcpy(&msg, data.constData(), headerSize);
 
             /// @note Копирование элементов
             const size_t elementsCount = msg.getElementsCount();
             if (elementsCount > 0) {
                 const size_t elementsSize = elementsCount *
                                             sizeof(ElementStatus);
                 if (data.size() >=
                     static_cast<int>(headerSize + elementsSize)) {
                     msg.elements = new ElementStatus[elementsCount];
                     memcpy(msg.elements, data.constData() + headerSize,
                            elementsSize);
                 }
             }
         }
 
         return msg;
     }
 #pragma GCC diagnostic pop
 
     /// @note Запрет на копирование и присваивание
     TcpMessage_0x21(const TcpMessage_0x21&) = delete;
     TcpMessage_0x21& operator=(const TcpMessage_0x21&) = delete;
 
     /// @note Разрешение перемещений
     TcpMessage_0x21(TcpMessage_0x21&& other) noexcept
         : TcpMessage<TcpMessage_0x21,
                      PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS>(),
         time1(other.time1), time2(other.time2), elements(other.elements) {
         other.elements = nullptr;
     }
 
     TcpMessage_0x21& operator=(TcpMessage_0x21&& other) noexcept {
         if (this != &other) {
             delete[] elements;
 
             time1 = other.time1;
             time2 = other.time2;
             elements = other.elements;
             other.elements = nullptr;
 
             // Копируем данные из базового класса
             static_cast<TcpHeader&>(*this) =
                 static_cast<const TcpHeader&>(other);
         }
         return *this;
     }
 };
 
 #pragma pack(pop)
 
 static_assert(sizeof(ElementStatus) == 4, "Invalid ElementStatus size");
 static_assert(sizeof(TcpMessage_0x21) == 16 + 8 + sizeof(ElementStatus*),
         "Invalid TcpMessage_0x21 size");
 ```

---

## Для работы по WebSocket

> [!NOTE]
> Нужно установить пакет для работы в WebSockets в Qt5 это:
> `sudo apt-get install libqt5websockets5-dev`

### Как это использовать

- TCP РМО: любые десктопные клиенты могут подключаться к серверу (порт 23456), отправлять бинарные данные (например, команды для ППУ).
- WebSocket РМО: браузерные клиенты (React, JS) подключаются к ws://<ip>:23457, отправляют/получают JSON-объекты.
- Отправка данных: используйте sendMessageRMO() для бинарных TCP-клиентов и sendMessageRMOJson() для браузерных WebSocket-клиентов.
