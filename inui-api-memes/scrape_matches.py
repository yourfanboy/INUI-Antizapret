import requests
import json
import hashlib
import secrets
import time

class INUIClient:
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
            'User-Agent': 'Mozilla/5.0 (Windows XP; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) INUIClient/0.107.5 Chrome/130.0.6723.118 Electron/33.2.0 Safari/537.36',
            'Content-Type': 'application/json',
            'origin': 'inui://inui',
            'sec-fetch-site': 'cross-site',
            'sec-fetch-mode': 'cors',
            'sec-fetch-dest': 'empty',
            'accept-encoding': 'gzip, deflate, br, zstd',
            'accept-language': 'en-US',
            'priority': 'u=1, i',
            'sec-ch-ua-platform': '"t.me/inuicheat"',
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
    
    def get_user_matches(self, query="9EMVXKSKMSPZWAIJ", page=0, size=10, only_favorite=False, sort="DESC", game_version=1):
        """Получает матчи пользователя"""
        if not self.auth_token:
            print("❌ Необходима авторизация!")
            return None
            
        params = {
            'query': query,
            'page': page,
            'onlyFavorite': str(only_favorite).lower(),
            'size': size,
            'sort': sort,
            'gameVersion': game_version
        }
        
        matches_url = f"{self.base_url}/statistic/getUserMatches"
        headers = self.get_headers()
        
        try:
            response = requests.get(matches_url, params=params, headers=headers)
            response.raise_for_status()
            
            matches_data = response.json()
            print(f"✅ Получены матчи - страница {page}, элементов: {len(matches_data.get('content', []))}")
            return matches_data
            
        except requests.exceptions.RequestException as e:
            print(f"❌ Ошибка запроса матчей: {e}")
            return None
        except json.JSONDecodeError as e:
            print(f"❌ Ошибка парсинга JSON: {e}")
            return None
    
    def get_all_mirage_matches(self, query="9EMVXKSKMSPZWAIJ", size=50):
        """Получает все матчи на карте de_mirage"""
        mirage_matches = []
        page = 0
        
        while True:
            matches_data = self.get_user_matches(query=query, page=page, size=size)
            
            if not matches_data or not matches_data.get('content'):
                break
                
            # Фильтруем матчи на карте de_mirage
            for match in matches_data['content']:
                if match.get('map') == 'de_mirage':
                    mirage_matches.append({
                        'matchId': match['matchId'],
                        'map': match['map']
                    })
            
            # Проверяем, есть ли ещё страницы
            page_info = matches_data.get('pageInfo', {})
            if page_info.get('isLast', True):
                break
                
            page += 1
            time.sleep(0.5)  # Небольшая задержка между запросами
        
        return mirage_matches
    
    def save_matches_to_file(self, matches, filename="mirage_matches.txt"):
        """Сохраняет ID матчей в текстовый файл"""
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                for match in matches:
                    f.write(f"{match['matchId']}\n")
            
            print(f"✅ Сохранено {len(matches)} матчей в файл '{filename}'")
            return True
            
        except Exception as e:
            print(f"❌ Ошибка сохранения в файл: {e}")
            return False

def main():
    # Настройки - замените на свои данные
    EMAIL = ""  # Замените на ваш email
    PASSWORD = ""        # Замените на ваш пароль
    QUERY = "T4HAUYWTUZFGJQNK"       # ID пользователя для поиска  9EMVXKSKMSPZWAIJ
    
    print("🚀 Запуск INUI API клиента...")
    
    # Создаём клиент
    client = INUIClient(EMAIL, PASSWORD)
    
    # Авторизуемся
    if not client.authenticate():
        print("❌ Не удалось авторизоваться!")
        return
    
    # Получаем все матчи на de_mirage
    print(f"🔍 Ищем матчи на карте de_mirage для пользователя {QUERY}...")
    mirage_matches = client.get_all_mirage_matches(query=QUERY, size=100)
    
    if mirage_matches:
        print(f"✅ Найдено {len(mirage_matches)} матчей на de_mirage:")
        for i, match in enumerate(mirage_matches[:5], 1):  # Показываем первые 5
            print(f"  {i}. {match['matchId']} - {match['map']}")
        
        if len(mirage_matches) > 5:
            print(f"  ... и ещё {len(mirage_matches) - 5} матчей")
        
        # Сохраняем в файл
        client.save_matches_to_file(mirage_matches)
    else:
        print("❌ Матчи на de_mirage не найдены!")

if __name__ == "__main__":
    main()