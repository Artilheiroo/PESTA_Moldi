import socket
import json
import os
import mysql.connector

# Configurações via Variáveis de Ambiente com valores padrão
PORT = int(os.getenv("BRIDGE_PORT", 5000))
MYSQL_HOST = os.getenv("MYSQL_HOST", "host.docker.internal") # Liga ao host a partir do container

def iniciar_ponte():
    print(f"[*] A iniciar ponte TCP -> MySQL na porta {PORT}...")
    print(f"[*] Configurado para ligar ao MySQL em: {MYSQL_HOST}")
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind(('0.0.0.0', PORT))
        server.listen(5)
        print("[+] Ponte TCP à escuta de conexões do ESP32.")
    except Exception as e:
        print(f"[-] Erro ao iniciar o servidor: {e}")
        return

    while True:
        client_sock, client_addr = server.accept()
        print(f"[+] Conexão recebida de {client_addr}")
        
        try:
            # Recebe o JSON do ESP32
            data = client_sock.recv(1024).decode('utf-8').strip()
            if not data:
                client_sock.sendall(b"ERROR")
                continue
                
            print(f"[i] Dados recebidos: {data}")
            payload = json.loads(data)
            
            # Conecta ao MySQL usando as credenciais enviadas pelo ESP32
            db = mysql.connector.connect(
                host=MYSQL_HOST,
                user=payload['user'],
                password=payload['pass'],
                database=payload['db'],
                port=3306
            )
            
            cursor = db.cursor()
            
            # Converter "--" para None (NULL no MySQL) — o ESP32 envia "--" quando o sensor falha
            temp_val = payload.get('temperatura')
            hum_val = payload.get('humidade')
            if temp_val == '--': temp_val = None
            if hum_val == '--': hum_val = None

            # Insere os dados usando backticks para evitar conflitos de sintaxe com 'Data'
            query = "INSERT INTO `Data` (`date`, `hora`, `valor_sensor`, `estado`, `temperatura`, `humidade`) VALUES (%s, %s, %s, %s, %s, %s)"
            values = (payload['data'], payload['hora'], payload['corrente'], payload.get('estado'), temp_val, hum_val)
            
            cursor.execute(query, values)
            db.commit()
            
            print(f"[+] Inserido no MySQL: Data={payload['data']} Hora={payload['hora']} Corrente={payload['corrente']}A Estado={payload.get('estado')} Temp={payload.get('temperatura')} Hum={payload.get('humidade')}")
            client_sock.sendall(b"OK")
            
            cursor.close()
            db.close()
            
        except json.JSONDecodeError:
            print("[-] Erro: Payload não é um JSON válido.")
            client_sock.sendall(b"ERROR_JSON")
        except mysql.connector.Error as err:
            print(f"[-] Erro de Base de Dados: {err}")
            client_sock.sendall(b"ERROR_DB")
        except Exception as e:
            print(f"[-] Erro inesperado: {e}")
            client_sock.sendall(b"ERROR")
        finally:
            client_sock.close()

if __name__ == "__main__":
    iniciar_ponte()
