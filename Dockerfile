FROM ubuntu:24.04

WORKDIR /app
RUN apt-get update && apt-get install -y curl gcc openssl pkg-config libssl-dev build-essential
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y 

COPY ./sae /app/
COPY ./certs /app/
COPY ./start.sh /app/
COPY ./lorem.txt /app/
RUN chmod +x /app/start.sh

RUN export PATH="$HOME/.cargo/bin:$PATH" && cargo build --release
CMD ["/app/start.sh"]
