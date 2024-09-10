#!/bin/bash

set -e

# Save the current directory
CURRENT_DIR=$(pwd)
ls

# Move to the project root directory
WORKING_DIR=$(dirname "$CURRENT_DIR")
echo "Moving to the project root directory: $WORKING_DIR..."
# cd "$WORKING_DIR"

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
make

# Create certificates and keys if they don't exist
if [ ! -f "$CERT_FILE" ] || [ ! -f "$KEY_FILE" ]; then
    echo "Generating self-signed certificates..."

    # Create directories if they do not exist
    mkdir -p $CERT_DIR
    mkdir -p $KEY_DIR

    # Set proper permissions on directories
    chmod 755 $CERT_DIR
    chmod 700 $KEY_DIR

    # Generate a private key without password
    openssl genpkey -algorithm RSA -out $KEY_FILE

    # Generate Diffie-Hellman parameters (you can choose a different size, e.g., 4096)
    # openssl dhparam -out $DH_PARAM_FILE 2048

    # Generate a self-signed certificate
    openssl req -new -x509 -key $KEY_FILE -out $CERT_FILE -days 365 -subj "/C=US/ST=State/L=City/O=Organization/OU=Unit/CN=localhost"

    # Set proper permissions
    chmod 600 $KEY_FILE
    chmod 644 $CERT_FILE
fi