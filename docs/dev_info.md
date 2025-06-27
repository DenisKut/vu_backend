# Выбор между `QSharedPointer` и `std::unique_ptr` в Qt5 + C++11

---

Выбор между этими умными указателями зависит от контекста и требований проекта. Ниже приведены ключевые аспекты для вашего случая.

---

## 1. `QSharedPointer` (Qt)

**Преимущества:**
- Интеграция с сигналами/слотами (копирование указателей).
- Работа с Qt-контейнерами (`QList<QSharedPointer<T>>`).
- Безопасное использование в многопоточных сценариях.
- Разделяемое владение: объект живёт, пока есть хотя бы один `QSharedPointer`.
- Автоматическое управление временем жизни.
- Совместимость с другими Qt-классами (`QVariant::fromValue(QSharedPointer<T>)`).

**Недостатки:**
- Накладные расходы на подсчёт ссылок.
- Риск циклических ссылок (объекты не удалятся).

---

## 2. `std::unique_ptr` (C++11)

**Преимущества:**
- Нет накладных расходов на подсчёт ссылок.
- Чёткое владение объектом (один владелец).
- Компилятор проверяет исключительное владение.
- Хорошо интегрируется с STL-контейнерами (`std::vector<std::unique_ptr<T>>`).

**Недостатки:**
- Для передачи объекта нужно использовать `std::move()`.
- Не подходит для сценариев с несколькими владельцами.
- Ограниченная интеграция с Qt (нельзя напрямую передать в сигнал/слот, сложности с Qt-контейнерами).

---

## Когда что выбрать?

**`QSharedPointer` лучше, если:**
- Сообщения используются в нескольких модулях/потоках.
- Нужна интеграция с Qt-механизмами (сигналы/слоты).
- Время жизни объекта неочевидно (например, кэширование).

**`std::unique_ptr` лучше, если:**
- Владение объектом строго определено.
- Требуется максимальная производительность.
- Используются STL-контейнеры.

---

## Рекомендация для вашего проекта

- Если проект на **Qt5**, сообщения передаются между компонентами, используется сигнально-слотовый механизм — **`QSharedPointer`** будет удобнее.
- Если сообщение обрабатывается только в одном месте, время жизни полностью контролируется, нет интеграции с Qt — используйте **`std::unique_ptr`**.

---

## Примеры использования

**С `QSharedPointer`:**
```cpp
QSharedPointer<TcpMessage_0x01> msg = MessageFactory::create<TcpMessage_0x01>();
emit messageReceived(msg); // Копирование безопасно
QList<QSharedPointer<TcpHeader>> messagesCache;
messagesCache.append(msg);
```

**С `std::unique_ptr`:**
```cpp
std::unique_ptr<TcpMessage_0x01> msg = MessageFactory::create<TcpMessage_0x01>();
networkModule.send(std::move(msg));
std::vector<std::unique_ptr<TcpHeader>> messages;
messages.push_back(std::move(msg));
```

---

## Итог

Для Qt5 с неочевидным временем жизни объектов и необходимостью интеграции с фреймворком — **`QSharedPointer` предпочтительнее**. Если критична производительность и владение явное — используйте `std::unique_ptr`. В смешанных сценариях можно применять оба подхода для разных типов сообщений.

> **Пример использования QSharedPointer:**
> 
> ```cpp
> #include <QSharedPointer>
> // ...
> template<typename T>
> static typename std::enable_if<
>     std::is_base_of<TcpHeader, T>::value,
>     QSharedPointer<T>
> >::type create() {
>     return QSharedPointer<T>::create();
> }
> // ...
> template<typename T, typename... Args>
> static typename std::enable_if<
>     std::is_base_of<TcpHeader, T>::value,
>     QSharedPointer<T>
> >::type create(Args&&... args) {
>     return QSharedPointer<T>::create(std::forward<Args>(args)...);
> }
> // ...
> static QSharedPointer<TcpHeader> parse(const QByteArray& data) {
>     if (data.size() < static_cast<int>(sizeof(TcpHeader))) {
>         return QSharedPointer<TcpHeader>();
>     }
>     TcpHeader header = TcpHeader::deserialize(data);
>     if (!header.Validate()) {
>         return QSharedPointer<TcpHeader>();
>     }
>     switch (static_cast<PBU_MessageNumbers>(header.typePack)) {
>     case PBU_MessageNumbers::REQ_REGISTRATION:
>         return parseMessage<TcpMessage_0x01>(data);
>     // ...
>     default:
>         return QSharedPointer<TcpHeader>();
>     }
> }
> // ...
> template<typename T>
> static QSharedPointer<TcpHeader> parseMessage(const QByteArray& data) {
>     if (data.size() < static_cast<int>(sizeof(T))) {
>         return QSharedPointer<TcpHeader>();
>     }
>     QSharedPointer<T> msg = QSharedPointer<T>::create();
>     memcpy(msg.data(), data.constData(), sizeof(T));
>     return msg.template staticCast<TcpHeader>();
> }
> ```

---

## Пример реализации TcpMessage_0x21 через std::vector

```cpp
#pragma pack(push, 1)
struct ElementStatus {
    uint16_t IDElem;
    uint8_t status:4;
    uint8_t workFl:4;
    uint8_t local:1;
    uint8_t isImit:1;
    uint8_t reserv:6;
};
struct TcpMessage_0x21_Header : TcpMessage<TcpMessage_0x21_Header, PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS> {
    uint32_t time1;
    uint32_t time2;
    TcpMessage_0x21_Header() {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);
    }
};
class TcpMessage_0x21 {
    TcpMessage_0x21_Header header;
    std::vector<ElementStatus> elements;
public:
    TcpMessage_0x21() = default;
    explicit TcpMessage_0x21(const std::vector<ElementStatus>& elems)
        : elements(elems) { updateSize(); }
    void setElements(const std::vector<ElementStatus>& elems) {
        elements = elems; updateSize();
    }
    void addElement(const ElementStatus& elem) {
        elements.push_back(elem); updateSize();
    }
    const std::vector<ElementStatus>& getElements() const { return elements; }
    QByteArray serializeFull() const {
        QByteArray data;
        data.append(reinterpret_cast<const char*>(&header), sizeof(header));
        data.append(reinterpret_cast<const char*>(elements.data()), elements.size() * sizeof(ElementStatus));
        return data;
    }
    static TcpMessage_0x21 deserialize(const QByteArray& data) {
        TcpMessage_0x21 msg;
        if (data.size() >= static_cast<int>(sizeof(TcpMessage_0x21_Header))) {
            memcpy(&msg.header, data.constData(), sizeof(TcpMessage_0x21_Header));
            const size_t elementsSize = (data.size() - sizeof(TcpMessage_0x21_Header)) / sizeof(ElementStatus);
            msg.elements.resize(elementsSize);
            if (elementsSize > 0) {
                memcpy(msg.elements.data(), data.constData() + sizeof(TcpMessage_0x21_Header), elementsSize * sizeof(ElementStatus));
            }
        }
        return msg;
    }
private:
    void updateSize() {
        header.sizeData = sizeof(TcpMessage_0x21_Header) - sizeof(TcpHeader) + elements.size() * sizeof(ElementStatus);
    }
};
#pragma pack(pop)
```

---

## Реализация с динамическим массивом

```cpp
#pragma pack(push, 1)
struct ElementStatus {
    uint16_t IDElem;
    uint8_t status:4;
    uint8_t workFl:4;
    uint8_t local:1;
    uint8_t isImit:1;
    uint8_t reserv:6;
};
struct TcpMessage_0x21 : TcpMessage<TcpMessage_0x21, PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS> {
    uint32_t time1;
    uint32_t time2;
    ElementStatus* elements;
    TcpMessage_0x21() : elements(nullptr) {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);
    }
    ~TcpMessage_0x21() { delete[] elements; }
    void setStatuses(const ElementStatus* statuses, size_t count) {
        delete[] elements; elements = nullptr;
        if (count > 0) {
            elements = new ElementStatus[count];
            std::copy(statuses, statuses + count, elements);
            sizeData = sizeof(TcpMessage_0x21) - sizeof(TcpHeader) + count * sizeof(ElementStatus);
        } else {
            sizeData = sizeof(TcpMessage_0x21) - sizeof(TcpHeader);
        }
    }
    size_t getElementsCount() const {
        return (sizeData - (sizeof(TcpMessage_0x21) - sizeof(TcpHeader))) / sizeof(ElementStatus);
    }
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    static TcpMessage_0x21 deserialize(const QByteArray& data) {
        TcpMessage_0x21 msg;
        const size_t headerSize = sizeof(TcpMessage_0x21);
        if (data.size() >= static_cast<int>(headerSize)) {
            memcpy(&msg, data.constData(), headerSize);
            const size_t elementsCount = msg.getElementsCount();
            if (elementsCount > 0) {
                const size_t elementsSize = elementsCount * sizeof(ElementStatus);
                if (data.size() >= static_cast<int>(headerSize + elementsSize)) {
                    msg.elements = new ElementStatus[elementsCount];
                    memcpy(msg.elements, data.constData() + headerSize, elementsSize);
                }
            }
        }
        return msg;
    }
#pragma GCC diagnostic pop
    TcpMessage_0x21(const TcpMessage_0x21&) = delete;
    TcpMessage_0x21& operator=(const TcpMessage_0x21&) = delete;
    TcpMessage_0x21(TcpMessage_0x21&& other) noexcept
        : TcpMessage<TcpMessage_0x21, PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS>(),
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
            static_cast<TcpHeader&>(*this) = static_cast<const TcpHeader&>(other);
        }
        return *this;
    }
};
#pragma pack(pop)
static_assert(sizeof(ElementStatus) == 4, "Invalid ElementStatus size");
static_assert(sizeof(TcpMessage_0x21) == 16 + 8 + sizeof(ElementStatus*), "Invalid TcpMessage_0x21 size");
```

---

## Для работы по WebSocket

> Нужно установить пакет для работы с WebSockets в Qt5:
> ```bash
> sudo apt-get install libqt5websockets5-dev
> ```

**Использование:**
- TCP РМО: десктопные клиенты подключаются к серверу (порт 23456), отправляют бинарные данные.
- WebSocket РМО: браузерные клиенты (React, JS) подключаются к ws://<ip>:23457, отправляют/получают JSON.
- Для отправки используйте `sendMessageRMO()` (TCP) и `sendMessageRMOJson()` (WebSocket).
