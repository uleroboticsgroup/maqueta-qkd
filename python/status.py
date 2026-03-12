import requests

ACCOUNT_ID = "3009"

url = f"https://kme-1.acct-{ACCOUNT_ID}.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-2/status"
cert = ("../certs/sae-1.crt", "../certs/sae-1.key")
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
