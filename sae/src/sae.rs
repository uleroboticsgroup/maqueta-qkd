/*
* Este archivo contiene el código para la comunicación entre los dos dockers
* */
use std::collections::VecDeque;
use std::time::Duration;

use reqwest::{Client, Url};
use serde::{Deserialize, Serialize};
use thiserror::Error;
use tokio::fs::File;
use tokio::io::{AsyncReadExt, AsyncWriteExt, BufReader};
use tokio::net::{TcpListener, TcpStream};
use tokio::time::Instant;

use crate::qukaydee::{get_key_from_id, get_keys_and_id, get_status};

// Aqui se puede configurar el funcionamiento de las peticiones al QKD
const KEYS_PER_REQUEST: usize = 5;
const KEY_SIZE_BYTES: usize = 1024;
const KEY_SIZE_BITS: usize = KEY_SIZE_BYTES * 8;

const SERVER_IP: &str = "192.168.8.2";
const SERVER_PORT: usize = 8080;

const TEXT_FILE: &str = "lorem.txt";

#[derive(Debug, Error)]
pub enum EncryptedMessageError {
    #[error("Key length ({0}) is shorter than plaintext length ({1})")]
    KeyTooShort(usize, usize),
    #[error("Fallo al desencriptar")]
    InvalidUtf8,
}

#[derive(Debug, Serialize, Deserialize)]
struct EncryptedMessage {
    pub key_id: String,
    pub ciphertext: Vec<u8>,
}

impl EncryptedMessage {
    pub fn new(
        key_id: String,
        key: Vec<u8>,
        plaintext: Vec<u8>,
    ) -> Result<Self, EncryptedMessageError> {
        if key.len() < plaintext.len() {
            return Err(EncryptedMessageError::KeyTooShort(
                key.len(),
                plaintext.len(),
            ));
        }
        // XOR encrypt byte-by-byte without cycling key
        // Si no reutilizamos claves XOR es seguro (One time pad)
        let ciphertext = plaintext
            .iter()
            .zip(key.iter())
            .map(|(&p, &k)| p ^ k)
            .collect();

        Ok(EncryptedMessage { key_id, ciphertext })
    }

    pub fn decrypt(&self, key: &[u8]) -> Result<String, EncryptedMessageError> {
        if key.len() < self.ciphertext.len() {
            return Err(EncryptedMessageError::KeyTooShort(
                key.len(),
                self.ciphertext.len(),
            ));
        }

        // XOR ciphertext with key to get plaintext bytes
        let plaintext_bytes: Vec<u8> = self
            .ciphertext
            .iter()
            .zip(key.iter())
            .map(|(&c, &k)| c ^ k)
            .collect();

        // Convert plaintext bytes to UTF-8 string
        match String::from_utf8(plaintext_bytes) {
            Ok(plaintext) => Ok(plaintext),
            Err(_) => Err(EncryptedMessageError::InvalidUtf8),
        }
    }
}

pub async fn sae_1(client: &Client, base_url: &Url) -> Result<(), Box<dyn std::error::Error>> {
    let _status_map = get_status(&client, &base_url);

    let ip_and_port = format!("0.0.0.0:{SERVER_PORT}");
    let listener = TcpListener::bind(ip_and_port).await?;
    println!("Server listening on port {SERVER_PORT}");

    let (socket, addr) = listener.accept().await?;
    println!("New connection: {}", addr);

    // Spawn a new task to handle the connection
    handle_client(socket, client, base_url).await;

    return Ok(());
}

pub async fn sae_2(client: &Client, base_url: &Url) -> Result<(), Box<dyn std::error::Error>> {
    // Abriendo conexión
    let ip_and_port = format!("{SERVER_IP}:{SERVER_PORT}");
    let stream = conectar_a_servidor(&ip_and_port);
    println!("Conectando al servidor {ip_and_port}...");

    let file = File::open(TEXT_FILE)
        .await
        .expect("Fallo al leer el archivo");
    let mut reader = BufReader::new(file);

    let mut buffer = vec![0u8; KEY_SIZE_BYTES];
    let mut id_and_key_queue: VecDeque<(String, Vec<u8>)> = VecDeque::new();
    let mut stream = stream.await?;

    println!("Comenzando envio...");
    let start = Instant::now();
    while reader.read(&mut buffer).await? > 0 {
        if id_and_key_queue.is_empty() {
            id_and_key_queue =
                get_keys_and_id(&client, &base_url, KEYS_PER_REQUEST, KEY_SIZE_BITS).await?;
            if id_and_key_queue.is_empty() {
                return Err("No se recibió ninguna clave del servidor".into());
            }
        }

        let (key_id, key) = id_and_key_queue
            .pop_front()
            .ok_or("No hay claves disponibles en la cola")?;

        let plaintext = buffer.clone();
        let ciphertext = EncryptedMessage::new(key_id, key, plaintext)?;
        let serialized = serde_json::to_vec(&ciphertext)?;
        let len = (serialized.len() as u32).to_be_bytes(); // 4 bytes
        stream.write_all(&len).await?;
        stream.write_all(&serialized).await?;
    }
    let end = Instant::now();
    let tiempo = end.duration_since(start);
    println!(
        "Comunicación terminada sin errores en el cliente\nTiempo transcurrido en el cliente: {:?}",
        tiempo
    );

    return Ok(());
}

async fn handle_client(mut socket: TcpStream, client: &Client, base_url: &Url) {
    let start = Instant::now();
    loop {
        // Leer los 4 bytes del tamaño
        let mut len = [0u8; 4];
        if let Err(e) = socket.read_exact(&mut len).await {
            println!("Client disconnected or error: {}", e);
            break; // Fin de la conexión
        }

        let len = u32::from_be_bytes(len) as usize;
        let mut buf = vec![0u8; len];

        // Leer el mensaje completo
        socket
            .read_exact(&mut buf)
            .await
            .expect("Error en el recieve");

        let msg: EncryptedMessage = serde_json::from_slice(&buf).expect("Error en el recieve");
        let key: Vec<u8> = get_key_from_id(client, base_url, &msg.key_id)
            .await
            .expect("Error en el recieve");
        let decripted = msg.decrypt(&key).expect("Error al desencriptar");

        // println!("{:?}", msg.ciphertext);
        println!("{decripted}");
    }

    let end = Instant::now();
    let tiempo = end.duration_since(start);
    println!(
        "Comunicación terminada sin errores en el servidor\nTiempo transcurrido en el servidor: {:?}",
        tiempo
    );
}

async fn conectar_a_servidor(addr: &String) -> Result<TcpStream, Box<dyn std::error::Error>> {
    return loop {
        match TcpStream::connect(addr).await {
            Ok(s) => break Ok(s),
            Err(e) => {
                println!("Reintentando al conectar al servidor...: {e}");
                tokio::time::sleep(Duration::from_millis(100)).await;
                continue;
            }
        };
    };
}
