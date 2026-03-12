##Peticion para obtener la KEY por le id obtenida del KM
import requests

ACCOUNT_ID = "3009"
SAE_ID = 2
KEY_ID="19bb0ebb-f8d8-4586-9aba-b8da621aed36" #Key id obtenida de GETKEY

url = f"https://kme-{SAE_ID}.acct-{ACCOUNT_ID}.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-1/dec_keys?key_ID={KEY_ID}"
cert = (f"../certs/sae-{SAE_ID}.crt", f"../certs/sae-{SAE_ID}.key")
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
