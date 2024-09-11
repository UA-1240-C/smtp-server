FROM ubuntu:24.04

# Set up environment for building C++ apps
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    libssl-dev\
    libboost-all-dev\
    libpqxx-dev\
    libgtest-dev\
    libgmock-dev\
    libsodium-dev\
    git

WORKDIR /smtp-server

COPY . .

RUN chmod +x scripts/build.sh

RUN ./scripts/build.sh



EXPOSE 2525

ENTRYPOINT ["./build/SMTP_server"]
# CMD ["ls"]