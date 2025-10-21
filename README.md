# QKD ETSI API Demo con Rust y Docker

Este es un proyecto de demostración diseñado para familiarizarme con la [API REST de la ETSI para redes QKD](https://www.etsi.org/deliver/etsi_gs/QKD/001_099/014/01.01.01_60/gs_qkd014v010101p.pdf), usando [qukaydee.com](https://qukaydee.com), contenedores Docker y Rust como lenguaje de programación principal.

## Descripción
El proyecto se compone de dos servicios:

- **Servidor (SAE-1)**: expone un endpoint para recibir mensajes cifrados y el ID de clave que firmó esos datos.
- **Cliente (SAE-2)**: lee el archivo `lorem.txt`, divide su contenido, cifra cada fragmento usando una clave obtenida de la API de qukaydee.com y lo envía al servidor junto con el ID de la clave usada.

> El cifrado se realiza mediante una operación **XOR** con una clave única (One-Time Pad), obtenida de manera "segura" a través de la red QKD. No se reutilizan claves, cumpliendo con los principios del cifrado perfecto. Dado que utiliza el simulador [qukaydee.com](https://qukaydee.com) no es realmente segura, pero lo sería en caso de utilizar equipos QKD reales.

Ambos servicios se ejecutan en contenedores separados orquestados por `docker-compose`.

## Configuración
En el archivo [sae.rs](sae/src/sae.rs) se encuentran estas constantes que se pueden modificar para probar con distintas configuraciones. Cuánto mayor sea el tamaño de clave más rápida será la comunicación, tanto porque se piden menos claves como porque se tienen que enviar menos mensajes. El número de claves por petición solo acelera el lado que encripta los mensajes. La velocidad aumentará hasta que llegemos a la limitación de bits por segundo del equipo QKD que estemos utilizando, en el caso de este simulador está por defecto en 10000 bits/segundo. Con estos parámetros he conseguido una ganancia de velocidad considerable con respecto a los que utilizaba originalmente, pero está llegando al límite de lo que admite el simulador.

```rust
const KEYS_PER_REQUEST: usize = 4;
const KEY_SIZE_BYTES: usize = 2048;
const KEY_SIZE_BITS: usize = KEY_SIZE_BYTES * 8;
```

## Cómo ejecutarlo
### 1. Pre-requisitos
- Docker
- Docker Compose
- Una cuenta en [qukaydee.com](https://qukaydee.com) con claves configuradas y guardadas en el directorio [certs](certs/).
- Recomiendo realizar el [tutorial](https://qukaydee.com/pages/getting_started) que guía por el proceso de conseguir estas claves. Las claves y certificados proporcionadas son las asociadas a mi cuenta y podrían dejar de ser válidas.
- Rust (si deseas compilar o probar localmente)

### 2. Construir y levantar

```bash
docker-compose build
docker-compose up
```

# TODO:
- [X] Mover la decodificación de las claves al [qukaydee.rs](sae/src/qukaydee.rs).
- [X] Cambiar el struct en el que se devuelven las claves a un FIFO de tuplas (id,clave).
