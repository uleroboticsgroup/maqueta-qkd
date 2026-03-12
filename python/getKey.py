import requests
import json

ACCOUNT_ID = "3009"
SAE_MAESTRO=1
SAE_ESCLAVO=2

url = f"https://kme-{SAE_MAESTRO}.acct-{ACCOUNT_ID}.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-{SAE_ESCLAVO}/enc_keys?number=2&size=1024"
cert = (f"../certs/sae-{SAE_MAESTRO}.crt", f"../certs/sae-{SAE_MAESTRO}.key")
ca_cert = f"../certs/account-{ACCOUNT_ID}-server-ca-qukaydee-com.crt"

headers = {
    "Accept": "application/json"
}

try:
    respuesta = requests.get(
        url,
        headers=headers,
        cert=cert,
        verify=ca_cert
    )

    if respuesta.status_code == 200:
        print(respuesta.json())
    else:
        print("Error:", respuesta.status_code)
        print(respuesta.text)

except requests.exceptions.RequestException as e:
    print("Error en la conexión:", e)
    
    

