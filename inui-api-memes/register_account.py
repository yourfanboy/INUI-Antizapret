import requests
import hashlib
import secrets
import json
from time import sleep
from datetime import datetime

class INUIAccountManager:
    def __init__(self):
        self.base_url = "https://api.inuisoftware.com/api/v1"
        self.auth_token = None
        self.user_id = None
        self.account_data = {
            'email': '',
            'username': '',
            'password': '',
            'user_id': '',
            'auth_token': '',
            'steam_id': '',
            'registration_date': ''
        }
    
    def generate_random_sha256(self):
        """Генерирует случайный SHA-256 хеш"""
        random_string = secrets.token_hex(32)
        return hashlib.sha256(random_string.encode()).hexdigest()
    
    def get_headers(self, include_auth=False):
        """Формирует заголовки для запросов"""
        headers = {
            'x-inui-id': self.generate_random_sha256(),
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) INUIClient/0.107.5 Chrome/130.0.6723.118 Electron/33.2.0 Safari/537.36',
            'Content-Type': 'application/json',
            'origin': 'inui://inui',
            'sec-fetch-site': 'cross-site',
            'sec-fetch-mode': 'cors',
            'sec-fetch-dest': 'empty',
            'accept-encoding': 'gzip, deflate, br, zstd',
            'accept-language': 'en-US',
            'priority': 'u=1, i',
            'sec-ch-ua-platform': '"Windows"',
            'sec-ch-ua': '"Not?A_Brand";v="99", "Chromium";v="130"',
            'sec-ch-ua-mobile': '?0',
            'x-app-version': '0.107.5',
            'accept': 'application/json, text/plain, */*'
        }
        
        if include_auth and self.auth_token:
            headers['authorization'] = f'Bearer {self.auth_token}'
            
        return headers
    
    def save_account_to_file(self):
        """Сохраняет данные аккаунта в файл"""
        try:
            with open('accounts.txt', 'a', encoding='utf-8') as f:
                f.write(f"=== АККАУНТ {self.account_data['email']} ===\n")
                f.write(f"Дата регистрации: {self.account_data['registration_date']}\n")
                f.write(f"Email: {self.account_data['email']}\n")
                f.write(f"Username: {self.account_data['username']}\n")
                f.write(f"Password: {self.account_data['password']}\n")
                f.write(f"User ID: {self.account_data['user_id']}\n")
                f.write(f"Steam ID: {self.account_data['steam_id']}\n")
                f.write(f"Access Token: {self.account_data['auth_token']}\n")
                f.write("="*40 + "\n\n")
            
            print(f"✅ Данные аккаунта сохранены в файл accounts.txt")
            return True
            
        except Exception as e:
            print(f"❌ Ошибка при сохранении в файл: {e}")
            return False
    
    def register_account(self, email, username, password):
        """Регистрирует новый аккаунт"""
        self.account_data['email'] = email
        self.account_data['username'] = username
        self.account_data['password'] = password
        self.account_data['registration_date'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        url = f"{self.base_url}/auth/register"
        data = {
            "email": email,
            "username": username,
            "password": password
        }
        
        try:
            response = requests.post(
                url,
                json=data,
                headers=self.get_headers()
            )
            response.raise_for_status()
            
            result = response.json()
            if result.get('status') == 'SUCCESS':
                print(f"✅ Успешная регистрация для {email}")
                return True
            else:
                print(f"❌ Ошибка регистрации: {result}")
                return False
                
        except Exception as e:
            print(f"❌ Ошибка при регистрации: {e}")
            return False
    
    def confirm_email(self, email, confirmation_code):
        """Подтверждает email с полученным кодом"""
        url = f"{self.base_url}/auth/new/check-confirm"
        data = {
            "email": email,
            "code": confirmation_code
        }
        
        try:
            response = requests.post(
                url,
                json=data,
                headers=self.get_headers()
            )
            response.raise_for_status()
            
            result = response.json()
            if result.get('status') == 'SUCCESS':
                self.auth_token = result['payload']['accessToken']
                self.user_id = result['payload']['userId']
                self.account_data['auth_token'] = self.auth_token
                self.account_data['user_id'] = self.user_id
                
                print(f"✅ Email {email} успешно подтверждён!")
                print(f"🔑 Access Token: {self.auth_token[:15]}...")
                print(f"🆔 User ID: {self.user_id}")
                return True
            else:
                print(f"❌ Ошибка подтверждения email: {result}")
                return False
                
        except Exception as e:
            print(f"❌ Ошибка при подтверждении email: {e}")
            return False
    
    def connect_steam_account(self, steam_id):
        """Привязывает Steam аккаунт"""
        if not self.auth_token:
            print("❌ Необходимо сначала подтвердить email!")
            return False
            
        self.account_data['steam_id'] = steam_id
        
        url = f"{self.base_url}/socials/connect"
        data = {
            "authorizationCode": steam_id,
            "appName": "STEAM"
        }
        
        try:
            response = requests.post(
                url,
                json=data,
                headers=self.get_headers(include_auth=True)
            )
            response.raise_for_status()
            
            result = response.json()
            if result.get('event') == 'connect_socials':
                print(f"✅ Steam аккаунт {steam_id} успешно привязан!")
                return True
            else:
                print(f"❌ Ошибка привязки Steam: {result}")
                return False
                
        except Exception as e:
            print(f"❌ Ошибка при привязке Steam: {e}")
            return False

def main():
    print("🚀 INUI Account Manager")
    print("-----------------------\n")
    
    manager = INUIAccountManager()
    
    # Ввод данных
    email = input("Enter email: ").strip()
    username = input("Enter username: ").strip()
    password = input("Enter password: ").strip()
    
    # Регистрация
    print("\n🔄 Регистрируем аккаунт...")
    if not manager.register_account(email, username, password):
        return
    
    # Подтверждение email
    print("\n📧 Введите код подтверждения из email")
    confirmation_code = input("Код подтверждения: ").strip()
    
    if not manager.confirm_email(email, confirmation_code):
        return
    
    # Привязка Steam
    print("\n🎮 Привязка Steam аккаунта")
    steam_id = input("Введите Steam ID: ").strip()
    
    if not manager.connect_steam_account(steam_id):
        return
    
    # Сохранение данных в файл
    manager.save_account_to_file()
    
    print("\n🎉 Все операции успешно завершены!")

if __name__ == "__main__":
    main()