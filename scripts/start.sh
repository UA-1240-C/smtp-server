#!/bin/bash

set -e

# Save the current directory
CURRENT_DIR=$(pwd)

# Move to the project root directory
WORKING_DIR=$(dirname "$CURRENT_DIR")
echo "Moving to the project root directory: $WORKING_DIR..."
cd "$WORKING_DIR"

# Define paths for certificates and keys
CERT_DIR=/etc/ssl/certs/smtp-server
KEY_DIR=/etc/ssl/private
CERT_FILE=$CERT_DIR/server.crt
KEY_FILE=$KEY_DIR/server.key

# Create and navigate to the build directory in the project root
echo "Creating build directory in the project root..."
mkdir -p build
cd build

# Configure the project with CMake
echo "Configuring the project with CMake..."
cmake ..

# Build the project
echo "Building the project..."
make -j$(nproc)

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

        # Generate Diffie-Hellman parameters (you can choose a different size, e.g., 4096)
        # sudo openssl dhparam -out $DH_PARAM_FILE 2048

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
ExecStart=$WORKING_DIR/build/SMTP_server
WorkingDirectory=$WORKING_DIR/build
Restart=on-failure
User=root
Group=root

[Install]
WantedBy=multi-user.target
EOF"

# Reload systemd to recognize the new service
echo "Reloading systemd..."
sudo systemctl daemon-reload

# Stop the service if it is running
sudo systemctl stop smtp-server
sudo systemctl disable smtp-server

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
