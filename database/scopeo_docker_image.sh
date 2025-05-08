#!/bin/sh

if [ -z "$1" ]; then
  echo "Использование: $0 <image:tag> (docker-образ). Необходим scopeo"
  exit 1
fi

IMAGE="$1"
FILENAME="$(echo "$IMAGE" | tr '/:' '_').tar"

skopeo copy "docker://$IMAGE" "docker-archive:$FILENAME:$IMAGE"
