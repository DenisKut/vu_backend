#!/bin/bash

set -e

tar_dir="${1:-.}"  # если не указано — берём текущую директорию
cd "$tar_dir"

echo "Рабочая директория: $(pwd)"

shopt -s nullglob

for gz_file in *.tar.gz; do
    base_name="${gz_file%.tar.gz}"

    md5_file="md5sums/$base_name.tar.gz.md5"
    tar_file="$base_name.tar"
    tar_md5_file="md5sums/$base_name.tar.md5"

    echo "▶ Обработка: $gz_file"

    # 1) Проверка чексуммы .tar.gz
    if [ -f "$md5_file" ]; then
        echo "🔍 Проверка MD5: $md5_file"
        md5sum -c "$md5_file"
    else
        echo "⚠️  Нет файла контрольной суммы: $md5_file"
        continue
    fi

    # 2) Распаковка
    echo "📦 Распаковка $gz_file"
    tar -xzf "$gz_file"

    # 3) Проверка чексуммы .tar
    if [ -f "$tar_md5_file" ]; then
        echo "🔍 Проверка MD5: $tar_md5_file"
        md5sum -c "$tar_md5_file"
    else
        echo "⚠️  Нет файла контрольной суммы: $tar_md5_file"
        continue
    fi

    # 4) Загрузка docker образа
    echo "🐳 Загрузка Docker-образа из $tar_file"
    if docker load -i "$tar_file"; then
        echo "✅ Образ успешно загружен: $tar_file"
    else
        echo "❌ Ошибка при загрузке: $tar_file"
        continue
    fi

    # 5) Удаление .tar
    echo "🧹 Удаление: $tar_file"
    rm -f "$tar_file"

    echo ""
done
