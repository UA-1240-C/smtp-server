#!/bin/bash

set -e

# Define paths for certificates and keys
CERT_DIR=/etc/ssl/certs/smtp-server
KEY_DIR=/etc/ssl/private
CERT_FILE=$CERT_DIR/server.crt
KEY_FILE=$KEY_DIR/server.key

echo "Installing dependencies..."
sudo apt-get install -y cmake build-essential libssl-dev pkg-config libboost-all-dev libsodium-dev

# Create and navigate to build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure the project with CMake
echo "Configuring the project with CMake..."
cmake ..

# Build the project
echo "Building the project..."
make

# Install the project
echo "Installing the project..."
sudo make install

# Create certificates and keys if they don't exist
if [ ! -f "$CERT_FILE" ] || [ ! -f "$KEY_FILE" ]; then
    echo "Generating self-signed certificates..."

    # Create directories if they do not exist
    sudo mkdir -p $CERT_DIR
    sudo mkdir -p $KEY_DIR

    # Set proper permissions on directories
    sudo chmod 755 $CERT_DIR
    sudo chmod 700 $KEY_DIR

    # Generate a private key without password
    sudo openssl genpkey -algorithm RSA -out $KEY_FILE

    # Generate a self-signed certificate
    sudo openssl req -new -x509 -key $KEY_FILE -out $CERT_FILE -days 365 -subj "/C=US/ST=State/L=City/O=Organization/OU=Unit/CN=localhost"

    # Set proper permissions
    sudo chmod 600 $KEY_FILE
    sudo chmod 644 $CERT_FILE
fi

# Create a systemd service file for the SMTP server
SERVICE_FILE=/etc/systemd/system/smtp-server.service

echo "Creating systemd service file..."
sudo bash -c "cat > $SERVICE_FILE <<EOF
[Unit]
Description=SMTP Server
After=network.target

[Service]
ExecStart=/usr/local/bin/SMTP_server
Restart=on-failure
User=root
Group=root

[Install]
WantedBy=multi-user.target
EOF"

# Reload systemd to recognize the new service
echo "Reloading systemd..."
sudo systemctl daemon-reload

# Start the service
echo "Starting the SMTP server service..."
if ! sudo systemctl start smtp-server; then
    echo "Failed to start SMTP server. Check logs for details."
    exit 1
fi

# Enable the service to start on boot
echo "Enabling the SMTP server service..."
sudo systemctl enable smtp-server

echo "Installation and setup complete."

sudo systemctl status smtp-server