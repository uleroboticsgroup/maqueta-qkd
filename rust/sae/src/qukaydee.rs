/*
* Este archivo contiene las funciones necesarias para comunicarse con
* https://qukaydee.com y obtener claves simuladas
*/
use base64::{Engine as _, engine::general_purpose::STANDARD as b64};
use reqwest::{
    Certificate, Client, Identity, Method, Request, Url,
    header::{ACCEPT, HeaderMap},
};
use serde_json::Value;
use std::{
    collections::{HashMap, VecDeque},
    env,
};
use tokio::fs;

#[inline]
pub async fn get_client_and_url_from_env() -> Result<(Client, Url), Box<dyn std::error::Error>> {
    let account_id: i32 = env::var("ACCOUNT_ID")
        .expect("Variable de entorno no encontrada")
        .parse()
        .expect("Not a Number");
    let sae_id: i32 = env::var("SAE_ID")
        .expect("Variable de entorno no encontrada")
        .parse()
        .expect("Not a Number"); // 1 o 2 en esta demo
    let kme_id = sae_id; // En entornos reales no, pero en esta demo coinciden
    let cert_dir = env::var("CERT_DIR").expect("Variable de entorno no encontrada");
    let other_sae: i32 = if sae_id == 1 { 2 } else { 1 };

    let ca_cert = format!(
        "{}/account-{}-server-ca-qukaydee-com.crt",
        cert_dir, account_id
    );
    let sae_cert = format!("{cert_dir}/sae-{sae_id}.crt");
    let sae_key = format!("{cert_dir}/sae-{sae_id}.key");

    let ca_cert = fs::read(ca_cert);
    let sae_cert = fs::read(sae_cert);
    let sae_key = fs::read(sae_key);

    let ca_cert = ca_cert
        .await
        .expect("Fallo al leer del sistema de archivos");
    let sae_cert = sae_cert
        .await
        .expect("Fallo al leer del sistema de archivos");
    let sae_key = sae_key
        .await
        .expect("Fallo al leer del sistema de archivos");

    let ca_cert = Certificate::from_pem(&ca_cert).expect("Fallo al crear el Certificado");
    let sae_identity =
        Identity::from_pkcs8_pem(&sae_cert, &sae_key).expect("Fallo al crear la Identidad");

    let mut cabecera = HeaderMap::new();
    cabecera.insert(ACCEPT, "application/json".parse()?);

    let client = Client::builder()
        .add_root_certificate(ca_cert)
        .identity(sae_identity)
        .default_headers(cabecera)
        .build()
        .expect("Fallo al crear el cliente");

    let base_url = format!(
        "https://kme-{kme_id}.acct-{account_id}.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-{other_sae}/"
    );

    let base_url = Url::parse(&base_url)?;

    return Ok((client, base_url));
}

pub async fn get_status(
    client: &Client,
    base_url: &Url,
) -> Result<HashMap<String, String>, reqwest::Error> {
    let url = base_url.join("status").expect("Fallo al configurar la url");
    let r = Request::new(Method::GET, url);

    let response = client.execute(r).await?.error_for_status()?;

    let response: Value = serde_json::from_str(
        &response
            .text()
            .await
            .expect("Fallo al acceder al contenido del mensaje"),
    )
    .expect("Fallo al leer el json");

    let mut map = HashMap::new();

    if let Some(object) = response.as_object() {
        for (clave, valor) in object {
            let valor_str = match valor {
                Value::String(s) => s.clone(),
                Value::Number(n) => n.to_string(),
                _ => continue, // Ignorar arrays, objetos, bools, nulls
            };
            map.insert(clave.clone(), valor_str);
        }
    }

    return Ok(map);
}

pub async fn get_keys_and_id(
    client: &Client,
    base_url: &Url,
    num_keys: usize,
    key_size_bits: usize,
) -> Result<VecDeque<(String, Vec<u8>)>, Box<dyn std::error::Error>> {
    let fin_url = format!("enc_keys?number={num_keys}&size={key_size_bits}");
    let url = base_url.join(&fin_url)?;

    let request = Request::new(Method::GET, url);
    let response = client.execute(request).await?;
    let body = response.text().await?;

    let json: Value = serde_json::from_str(&body)?;

    let mut deque = VecDeque::new();

    if let Some(keys) = json.get("keys").and_then(|v| v.as_array()) {
        for item in keys {
            if let (Some(key), Some(key_id)) = (
                item.get("key").and_then(|v| v.as_str()),
                item.get("key_ID").and_then(|v| v.as_str()),
            ) {
                let decoded = b64.decode(key)?; // base64 decoding
                deque.push_back((key_id.to_string(), decoded));
            }
        }
    }

    Ok(deque)
}

pub async fn get_key_from_id(
    client: &Client,
    base_url: &Url,
    key_id: &str,
) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
    let fin_url = format!("dec_keys?key_ID={key_id}");
    let url = base_url.join(&fin_url)?;

    let request = reqwest::Request::new(Method::GET, url);
    let response = client.execute(request).await?;
    let body = response.text().await?;

    let json: Value = serde_json::from_str(&body)?;

    let key_str = json
        .get("keys")
        .and_then(|keys| keys.as_array())
        .and_then(|array| array.get(0))
        .and_then(|item| item.get("key"))
        .and_then(|v| v.as_str())
        .ok_or("No se encontró ninguna clave válida")?;

    let decoded = b64.decode(key_str)?; // base64 decoding

    Ok(decoded)
}
