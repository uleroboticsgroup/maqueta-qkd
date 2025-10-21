use std::env;
use tokio;

mod qukaydee;
mod sae;

use crate::qukaydee::get_client_and_url_from_env;
use crate::sae::{sae_1, sae_2};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // dotenv::dotenv().ok(); // En caso de que se use un .env
    let (client, base_url) = match get_client_and_url_from_env().await {
        Ok(r) => r,
        Err(e) => {
            println!("Error al inicializar: {e}");
            std::process::exit(1);
        }
    };

    let sae_id: i32 = env::var("SAE_ID")
        .expect("Variable de entorno no encontrada")
        .parse()
        .expect("Not a Number");

    if sae_id == 1 {
        sae_1(&client, &base_url).await?;
    } else if sae_id == 2 {
        sae_2(&client, &base_url).await?;
    } else {
        println!("SAE no válido: sae_{sae_id}");
    }

    return Ok(());
}
