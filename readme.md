# ВУ Backend (Astra 1.7)

---

**Проект вычислительного устройства (ВУ)** — серверная часть для платформы Astra 1.7. 
Реализует сетевые протоколы, обработку сообщений, логирование, работу с БД и вспомогательные сервисы.

---

## 📚 Документация и справочные материалы

- [Руководство по Git и патчам](./docs/git_man.md)
- [Инструкция по Docker и работе с БД](./docs/docker_man.md)
- [Описание системы логирования](./docs/logger_man.md)
- [Doxygen-комментарии и автодокументация](./docs/doxygen_man.md)
- [Архитектурные и технические заметки](./docs/dev_info.md)

---

## 📁 Структура проекта

<details>
<summary>Директории и основные файлы</summary>

```
/ (корень)
├── build/                # Директория сборки
├── core/                 # Ядро приложения (функциональные модули)
│   ├── logger/           # Класс Logger
│   ├── calculator/       # Модуль калькулятора
│   ├── CoreManager.*     # Менеджер запуска
│   └── ...
├── network/              # Сетевой модуль
│   ├── handlers/         # Обработчики протоколов (TCP, UDP, MAC)
│   ├── messages/         # Протокольные структуры
│   └── NetworkManager.*  # Менеджер подключений
├── utils/                # Вспомогательные утилиты
│   ├── time_provider/    # Работа со временем
│   ├── env_loader/       # Загрузка конфигурации
│   └── AppConfig.h       # Константы и макросы
├── database/             # Docker, SQL, скрипты БД
│   ├── docker-compose.yml
│   ├── init_db.sql
│   └── ...
├── docs/                 # Документация и справочные материалы
│   ├── git_man.md
│   ├── docker_man.md
│   ├── logger_man.md
│   ├── doxygen_man.md
│   └── ...
├── main.cpp              # Главная точка запуска
├── settings.ini          # INI-конфиг для приложения
├── .env                  # Переменные окружения (PBU, Ethernet)
└── ...
```
</details>

---

## 🚀 Быстрый старт

### 1. Клонирование и подготовка

```powershell
git clone https://github.com/DenisKut/vu_backend.git
cd vu_backend
```

### 2. Запуск базы данных и админки через Docker

Подробно: [Docker-инструкция](./docs/docker_man.md)

```powershell
cd database
docker compose up -d
```

- Веб-интерфейс для работы с БД: [http://localhost:5431](http://localhost:5431)
- Данные для входа указаны в `docker-compose.yml` и `.env`

### 3. Сборка и запуск приложения (Qt)

- Откройте проект в Qt Creator (`vu_backend.pro`)
- Соберите и запустите `main.cpp`
- Проверьте настройки в `settings.ini` и `.env`

---

## 🛠️ Основные возможности

- **Сетевые протоколы:** TCP, UDP, MAC, WebSocket (см. [dev_info.md](./docs/dev_info.md))
- **Гибкая система логирования** (см. [logger_man.md](./docs/logger_man.md))
- **Работа с БД PostgreSQL** (через Docker)
- **Конфигурирование через .env и settings.ini**
- **Модульная архитектура** (core, network, utils)

---

## 🛠️ Установка удаленного экрана + оффлайн пакеты

- Для разворачивания архитектуры программы на хосте без доступа к интернету, 
   достаточно сделать всего несколько шагов:
   * Перейти в директорию `dist`
   * Выполнить команду `sudo chmod +x deployAsRoot.sh`
   * Запустить `sudo ./deployAsRoot.sh` или [находясь под рутом]`./deployAsRoot.sh`
- Этот скрипт делает следующее:
   * Проверяет чексумму архива (не предмет "целый ли он")
   * Перемещает архив в /opt
   * Распаковывает его (/opt/packets) + чистит архив + добавляет запись для учета репозитория
   * Устанавливает необходимые пакеты
   * Правит ssh конифигурацию, для поддержки `X11Forwarding`
   * Открывает порты 22, 5900 правилами iptables




## 💡 Полезные ссылки и советы

- [Markdown-шпаргалка(Базисы RU)](https://github.com/sandino/Markdown-Cheatsheet)
- [Markdown-шпаргалка(Официальная EN)](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)
- [Подключение к удалённому стенду (durabook, astra)](#remote-stand)

<details>
<summary id="remote-stand">Подключение к удалённому стенду (durabook, astra)</summary>

1. Проверьте поддержку IPv6: [test-ipv6.com](https://test-ipv6.com), [ipv6-test.com](https://ipv6-test.com)
2. Сгенерируйте ssh-ключ:
   ```powershell
   ssh-keygen -t ed25519 -C "your_email@example.com"
   ```
   Передайте .pub на nikitoniy2468@gmail.com
3. Подключение:
   ```powershell
   ssh -Xv6 user@nikitonz-astra.ddns.net
   ```
</details>

---

<details>
<summary id="remote-stand-vnc">Удалённый рабочий стол</summary>

1. Логин через ssh из пункта [выше](#remote-stand)
2. Запустить службу vnc: `sudo systemctl start x11vnc`
3. На клиенте: 
   - Для Windows: `real vnc view`
      - логин: `[nikitonz-astra.ddns.net]:5900`
      - пароль: `bleh`
4. По окончанию работ убить vnc: `sudo systemctl stop x11vnc`

</details>

---

## 📝 Примечания

- Для подробностей по каждому модулю см. соответствующие файлы в папке `docs/`.
- Для работы с WebSocket в Qt5 установите пакет:
  ```powershell
  sudo apt-get install libqt5websockets5-dev
  ```
- Для вопросов по структуре, сборке или запуску — смотрите [dev_info.md](./docs/dev_info.md) и [docker_man.md](./docs/docker_man.md).

---

_Проект развивается. Документация и структура могут обновляться._
