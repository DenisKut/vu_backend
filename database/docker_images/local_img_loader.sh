#!/bin/bash

if [ -z "$1" ]; then
    echo "Использование: $0 /путь/к/директории/с/образами"
    exit 1
fi

tar_dir="$1"

# Проверяем, существует ли директория
if [ ! -d "$tar_dir" ]; then
    echo "Ошибка: Директория не существует!"
    exit 1
fi

# Цикл по каждому tar файлу в директории
for tar_file in "$tar_dir"/*.tar; do
    if [ -f "$tar_file" ]; then
        # Проверка, является ли файл Docker образом
        if docker load -i "$tar_file" > /dev/null 2>&1; then
            echo "Образ успешно загружен из файла: $tar_file"
        else
            echo "Ошибка при загрузке образа из файла: $tar_file"
        fi
    fi
done
