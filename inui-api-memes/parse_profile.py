import requests
import json
import hashlib
import secrets

class INUIUserProfileParser:
    def __init__(self, email, password):
        self.email = email
        self.password = password
        self.auth_token = None
        self.base_url = "https://api.inuisoftware.com/api/v1"
        
    def generate_random_sha256(self):
        """Генерирует случайный SHA-256 хеш"""
        random_string = secrets.token_hex(32)
        return hashlib.sha256(random_string.encode()).hexdigest()
    
    def get_headers(self, include_auth=True):
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
    
    def authenticate(self):
        """Получает токен авторизации"""
        auth_url = f"{self.base_url}/auth"
        auth_data = {
            "email": self.email,
            "password": self.password
        }
        
        headers = self.get_headers(include_auth=False)
        
        try:
            response = requests.post(auth_url, json=auth_data, headers=headers)
            response.raise_for_status()
            
            auth_response = response.json()
            
            if auth_response.get('status') == 'SUCCESS':
                self.auth_token = auth_response['payload']['accessToken']
                print(f"✅ Авторизация успешна! Токен получен.")
                return True
            else:
                print(f"❌ Ошибка авторизации: {auth_response}")
                return False
                
        except requests.exceptions.RequestException as e:
            print(f"❌ Ошибка запроса авторизации: {e}")
            return False
        except json.JSONDecodeError as e:
            print(f"❌ Ошибка парсинга JSON: {e}")
            return False
    
    def get_user_profile(self, user_id):
        """Получает профиль пользователя по ID"""
        if not self.auth_token:
            print("❌ Необходима авторизация!")
            return None
            
        profile_url = f"{self.base_url}/user?userId={user_id}"
        headers = self.get_headers()
        
        try:
            response = requests.get(profile_url, headers=headers)
            response.raise_for_status()
            
            profile_data = response.json()
            
            if profile_data.get('event') != 'get_user_profile_by_user_id':
                print(f"❌ Неожиданный ответ сервера: {profile_data.get('event')}")
                return None
                
            if 'content' not in profile_data:
                print("❌ Отсутствует content в ответе")
                return None
                
            print(f"✅ Профиль пользователя {user_id} успешно получен")
            return profile_data['content']
            
        except requests.exceptions.RequestException as e:
            print(f"❌ Ошибка запроса профиля пользователя: {e}")
            if hasattr(e, 'response') and e.response is not None:
                print(f"   Статус код: {e.response.status_code}")
                try:
                    error_data = e.response.json()
                    print(f"   Ответ сервера: {error_data}")
                except:
                    print(f"   Текст ответа: {e.response.text}")
            return None
        except json.JSONDecodeError as e:
            print(f"❌ Ошибка парсинга JSON: {e}")
            return None
    
    def save_profile_to_file(self, profile_data, filename="user_profile.txt"):
        """Сохраняет профиль пользователя в файл"""
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                f.write("=== Профиль пользователя ===\n\n")
                
                # Основная информация
                f.write(f"User ID: {profile_data.get('userId', 'N/A')}\n")
                f.write(f"Никнейм: {profile_data.get('nickname', 'N/A')}\n")
                f.write(f"Public Name: {profile_data.get('publicName', 'N/A')}\n")
                f.write(f"Username: {profile_data.get('username', 'N/A')}\n")
                f.write(f"Статус: {profile_data.get('status', 'N/A')}\n")
                f.write(f"Последний визит: {profile_data.get('lastVisit', 'N/A')}\n")
                f.write(f"Дата регистрации: {profile_data.get('memberSince', 'N/A')}\n")
                f.write(f"Страна: {profile_data.get('country', 'N/A')}\n")
                f.write(f"Город: {profile_data.get('city', 'N/A')}\n")
                f.write(f"Аватар: {profile_data.get('avatarSrc', 'N/A')}\n")
                f.write(f"Фон профиля: {profile_data.get('backgroundSrc', 'N/A')}\n")
                f.write(f"Pro статус: {'Да' if profile_data.get('pro', False) else 'Нет'}\n")
                f.write(f"Верифицирован: {'Да' if profile_data.get('verified', False) else 'Нет'}\n\n")
                
                # Социальные данные
                f.write("=== Социальные данные ===\n")
                f.write(f"Друзей: {profile_data.get('friendsCount', 0)}\n")
                f.write(f"Подписчиков: {profile_data.get('followersCount', 0)}\n")
                f.write(f"Подписок: {profile_data.get('followedCount', 0)}\n")
                f.write(f"Заблокированных: {profile_data.get('blockedUsersCount', 0)}\n")
                f.write(f"Запросов в друзья: {profile_data.get('friendRequestsCount', 0)}\n\n")
                
                # Социальные ссылки
                if 'socialLinks' in profile_data and profile_data['socialLinks']:
                    f.write("=== Социальные сети ===\n")
                    for link in profile_data['socialLinks']:
                        f.write(f"{link.get('appName', 'N/A')}: {link.get('link', 'N/A')}\n")
                        if 'title' in link:
                            f.write(f"   Название: {link['title']}\n")
                    f.write("\n")
                
                # Дополнительная информация
                f.write("=== Дополнительная информация ===\n")
                f.write(f"Статус аутентификации: {profile_data.get('authStatus', 'N/A')}\n")
                f.write(f"Уровень: {profile_data.get('level', 0)}\n")
                f.write(f"Опыт: {profile_data.get('experience', 0)}/{profile_data.get('finishExpOfLevel', 100)}\n")
                f.write(f"Заблокировал вас: {'Да' if profile_data.get('blockedMe', False) else 'Нет'}\n")
                f.write(f"Отправлен запрос в друзья: {'Да' if profile_data.get('friendRequestSent', False) else 'Нет'}\n")
                f.write(f"Друг: {'Да' if profile_data.get('isFriend', False) else 'Нет'}\n")
                f.write(f"Подписан на вас: {'Да' if profile_data.get('isFollowed', False) else 'Нет'}\n")
                f.write(f"Заблокирован вами: {'Да' if profile_data.get('isBlockedByMe', False) else 'Нет'}\n")

            print(f"✅ Профиль сохранён в файл '{filename}'")
            return True
            
        except Exception as e:
            print(f"❌ Ошибка сохранения профиля: {e}")
            return False

def main():
    # Настройки
    EMAIL = ""  # Замените на ваш email
    PASSWORD = ""        # Замените на ваш пароль
    USER_ID = "BNHYOYZKVKI42GXS"      # Замените на нужный user_id
    OUTPUT_FILE = "user_profile.txt"  # Файл для сохранения
    
    print("🚀 Запуск парсера профиля пользователя INUI...")
    
    # Создаём парсер
    parser = INUIUserProfileParser(EMAIL, PASSWORD)
    
    # Авторизуемся
    if not parser.authenticate():
        print("❌ Не удалось авторизоваться!")
        return
    
    # Получаем профиль
    print(f"🔍 Получаем профиль пользователя {USER_ID}...")
    profile_data = parser.get_user_profile(USER_ID)
    
    if profile_data:
        # Сохраняем профиль
        parser.save_profile_to_file(profile_data, OUTPUT_FILE)
        print("🎉 Профиль успешно получен и сохранён!")
    else:
        print("❌ Не удалось получить профиль пользователя!")

if __name__ == "__main__":
    main()