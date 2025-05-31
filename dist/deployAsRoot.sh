#!/bin/bash

set -e

ARCHIVE="offlinePack.tar"
CHECKSUM_FILE="offlinePack.tar.md5"

if [[ ! -f "$CHECKSUM_FILE" ]]; then
  echo "❌ Checksum file not found: $CHECKSUM_FILE"
  exit 1
fi

if ! md5sum -c "$CHECKSUM_FILE"; then
  echo "❌ Checksum verification failed."
  exit 1
fi

echo "✅ Checksum OK. Installing..."

cp "$ARCHIVE" /opt/
cd /opt
tar -xvf "$ARCHIVE"
rm -f "$ARCHIVE"

echo "deb [trusted=yes] file:/opt/packages ./" | tee /etc/apt/sources.list.d/offline.list > /dev/null

apt update && sudo apt install \
libqt5websockets5-dev \
libqt5sql5-psql \
build-essential \
qt5-default \
qttools5-dev \
libpq-dev \
qtcreator \
docker.io \
docker-compose \
git \
x11vnc



cat << 'EOF' > /lib/systemd/system/x11vnc.service
[Unit]
Description=x11vnc service
After=display-manager.service network.target syslog.target

[Service]
Type=simple
ExecStart=/usr/bin/x11vnc -forever -display :0 -auth guess -passwd my-password
ExecStop=/usr/bin/killall x11vnc
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

systemctl enable --now x11vnc


cat << 'EOF' > /lib/systemd/system/x11vnc.service
[Unit]
Description=x11vnc service
After=display-manager.service network.target syslog.target

[Service]
Type=simple
ExecStart=/usr/bin/x11vnc -forever -display :0 -auth guess -passwd my-password
ExecStop=/usr/bin/killall x11vnc
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF


sed -i -E '
0,/^#?\s*Port / s/^#?\s*Port .*/Port 22/
0,/^#?\s*AddressFamily / s/^#?\s*AddressFamily .*/AddressFamily any/
0,/^#?\s*ListenAddress 0\.0\.0\.0/ s/^#?\s*ListenAddress .*/ListenAddress 0.0.0.0/
0,/^#?\s*PermitRootLogin / s/^#?\s*PermitRootLogin .*/PermitRootLogin yes/
0,/^#?\s*PasswordAuthentication / s/^#?\s*PasswordAuthentication .*/PasswordAuthentication yes/
0,/^#?\s*PermitEmptyPasswords / s/^#?\s*PermitEmptyPasswords .*/PermitEmptyPasswords yes/
0,/^#?\s*X11Forwarding / s/^#?\s*X11Forwarding .*/X11Forwarding yes/
' /etc/ssh/sshd_config

systemctl restart ssh
systemctl restart sshd

sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 5900 -j ACCEPT




