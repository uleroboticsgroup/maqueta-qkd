# QKD ETSI API Demo con Rust/C y Docker
Este proyecto es una maqueta experimental de quantum key distribution o QKD, utilizando [qukaydee.com](https://qukaydee.com). Asi mismo utiliza Docker compose y como lenguaje se han implementado en dos lenguajes (C y Rust). La implementación principal en C encriptara el mensaje con el algoritmo de clave simetrica XOR y firmara el mensaje con el algoritmo pos-cuantico CRYSTALS-Dilithium, utilizando Dilithium5 junto a la libreria [pq-crystals/dilithium](https://github.com/pq-crystals/dilithium). Por otra parte la implementacion en Rust, no firmara el mensaje, solo encriptara el mensaje.

## Descripción
El proyecto se compone de dos contenedores los cuales se comunicaran entre si:

- **Alice**: Este contenedor leera un archivo `lorem.txt`, encriptara su contenido, firmara el contenido con Dilithium5 y sera enviado a Bob.
- **Bob**: Este contenedor escuchara el mensaje de Alice, desencriptara el mensaje y verificara la contenido.

> El cifrado se realiza mediante una operación **XOR** con una clave única (One-Time Pad), obtenida de manera "segura" a través de la red QKD. No se reutilizan claves, cumpliendo con los principios del cifrado perfecto. Dado que utiliza el simulador [qukaydee.com](https://qukaydee.com) no es realmente segura, pero lo sería en caso de utilizar equipos QKD reales.

Ambos servicios se ejecutan en contenedores separados orquestados por `docker-compose`.

## Configuración Rust
En el archivo [sae.rs](rust/sae/src/sae.rs) se encuentran estas constantes que se pueden modificar para probar con distintas configuraciones. Cuánto mayor sea el tamaño de clave más rápida será la comunicación, tanto porque se piden menos claves como porque se tienen que enviar menos mensajes. El número de claves por petición solo acelera el lado que encripta los mensajes. La velocidad aumentará hasta que llegemos a la limitación de bits por segundo del equipo QKD que estemos utilizando, en el caso de este simulador está por defecto en 10000 bits/segundo. Con estos parámetros he conseguido una ganancia de velocidad considerable con respecto a los que utilizaba originalmente, pero está llegando al límite de lo que admite el simulador.

```rust
const KEYS_PER_REQUEST: usize = 4;
const KEY_SIZE_BYTES: usize = 2048;
const KEY_SIZE_BITS: usize = KEY_SIZE_BYTES * 8;
```

## Cómo ejecutarlo
### 1. Pre-requisitos
- Docker version v28.4.0
- Docker Compose version v2.39.1
- Cuenta en [qukaydee.com](https://qukaydee.com) con claves configuradas y guardadas en el directorio [certs](certs/). Es necesario crear previamente el directorio mencionado.
- Realizar el [tutorial](https://qukaydee.com/pages/getting_started) que guía por el proceso de conseguir estas claves.
- Es necesario modificar la variable de entorno $ACCOUNT_ID en el archivo [docker-compse.yml](./docker-compose.yml) por el identificar de la cuenta correspondiente. El archivo "Client CA Certificate" (se descarga desde qukaydee) contiene el ID de la cuenta.

```
account-ACCOUNT_ID-server-ca-qukaydee-com.crt
```
- Dependiendo la version que se desee probar se debera cambiar en el [docker-compse.yml](./docker-compose.yml), el campo  "dockerfile:"
    - En C: c/DockerfileBob (en Bob) y c/DockerfileAlice (en Alice)
    - En Rust: rust/Dockerfile

### 2. Construir y levantar

```bash
docker compose up --build
```
