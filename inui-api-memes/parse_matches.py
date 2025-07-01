import requests
import json
import hashlib
import secrets
import time
import os

class INUIMatchParser:
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
    
    def get_match_details(self, match_id, game_version=1):
        """Получает детальную информацию о матче"""
        if not self.auth_token:
            print("❌ Необходима авторизация!")
            return None
            
        match_url = f"{self.base_url}/statistic/match/{match_id}"
        params = {'gameVersion': game_version}
        headers = self.get_headers()
        
        try:
            response = requests.get(match_url, params=params, headers=headers)
            response.raise_for_status()
            
            match_data = response.json()
            print(f"✅ Получена информация о матче {match_id}")
            return match_data
            
        except requests.exceptions.RequestException as e:
            print(f"❌ Ошибка запроса информации о матче {match_id}: {e}")
            if hasattr(e, 'response') and e.response is not None:
                print(f"   Статус код: {e.response.status_code}")
                try:
                    error_data = e.response.json()
                    print(f"   Ответ сервера: {error_data}")
                except:
                    print(f"   Текст ответа: {e.response.text}")
            return None
        except json.JSONDecodeError as e:
            print(f"❌ Ошибка парсинга JSON для матча {match_id}: {e}")
            return None
    
    def extract_team_players(self, match_data):
        """Извлекает данные игроков из команд (nickname, userId, avatarSrc)"""
        red_players = []
        blue_players = []
        
        # Проверяем наличие content в ответе
        if 'content' not in match_data:
            return red_players, blue_players
        
        content = match_data['content']
        
        # Извлекаем игроков красной команды
        red_team = content.get('redTeam', {})
        if 'players' in red_team:
            for player in red_team['players']:
                player_data = {
                    'nickname': player.get('nickname', 'Unknown'),
                    'userId': player.get('userId', ''),
                    'avatarSrc': player.get('avatarSrc', '')
                }
                red_players.append(player_data)
        
        # Извлекаем игроков синей команды
        blue_team = content.get('blueTeam', {})
        if 'players' in blue_team:
            for player in blue_team['players']:
                player_data = {
                    'nickname': player.get('nickname', 'Unknown'),
                    'userId': player.get('userId', ''),
                    'avatarSrc': player.get('avatarSrc', '')
                }
                blue_players.append(player_data)
        
        return red_players, blue_players
    
    def read_match_ids_from_file(self, filename="mirage_matches.txt"):
        """Читает ID матчей из текстового файла"""
        match_ids = []
        
        try:
            if not os.path.exists(filename):
                print(f"❌ Файл {filename} не найден!")
                return match_ids
                
            with open(filename, 'r', encoding='utf-8') as f:
                for line in f:
                    match_id = line.strip()
                    if match_id:  # Пропускаем пустые строки
                        match_ids.append(match_id)
            
            print(f"✅ Прочитано {len(match_ids)} ID матчей из файла {filename}")
            return match_ids
            
        except Exception as e:
            print(f"❌ Ошибка чтения файла {filename}: {e}")
            return match_ids
    
    def parse_all_matches(self, input_filename="mirage_matches.txt", output_filename="match_players.txt"):
        """Парсит все матчи и сохраняет информацию об игроках"""
        match_ids = self.read_match_ids_from_file(input_filename)
        
        if not match_ids:
            print("❌ Нет ID матчей для обработки!")
            return False
        
        all_results = []
        successful_matches = 0
        
        print(f"🔍 Начинаем парсинг {len(match_ids)} матчей...")
        
        for i, match_id in enumerate(match_ids, 1):
            print(f"📊 Обрабатываем матч {i}/{len(match_ids)}: {match_id}")
            
            match_data = self.get_match_details(match_id)
            
            if match_data:
                red_players, blue_players = self.extract_team_players(match_data)
                
                result = {
                    'match_id': match_id,
                    'red_players': red_players,
                    'blue_players': blue_players
                }
                
                all_results.append(result)
                successful_matches += 1
                
                print(f"   ✅ Red: {len(red_players)} игроков, Blue: {len(blue_players)} игроков")
            else:
                print(f"   ❌ Не удалось получить данные для матча {match_id}")
            
            # Небольшая задержка между запросами
            time.sleep(0.5)
        
        # Сохраняем результаты
        if all_results:
            self.save_results_to_file(all_results, output_filename)
            print(f"✅ Успешно обработано {successful_matches}/{len(match_ids)} матчей")
            return True
        else:
            print("❌ Не удалось обработать ни одного матча!")
            return False
    
    def save_results_to_file(self, results, filename="match_players.txt"):
        """Сохраняет результаты в текстовый файл с дополнительной информацией"""
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                for result in results:
                    match_id = result['match_id']
                    red_players = result['red_players']
                    blue_players = result['blue_players']
                    
                    f.write(f"{match_id}:\n")
                    f.write("Red Team players:\n")
                    for player in red_players:
                        f.write(f"Nickname: {player['nickname']}\n")
                        f.write(f"UserID: {player['userId']}\n")
                        f.write(f"Avatar: {player['avatarSrc']}\n")
                        f.write("\n")
                    
                    f.write("Blue Team players:\n")
                    for player in blue_players:
                        f.write(f"Nickname: {player['nickname']}\n")
                        f.write(f"UserID: {player['userId']}\n")
                        f.write(f"Avatar: {player['avatarSrc']}\n")
                        f.write("\n")
                    
                    f.write("\n")  # Пустая строка между матчами
            
            print(f"✅ Результаты сохранены в файл '{filename}'")
            return True
            
        except Exception as e:
            print(f"❌ Ошибка сохранения в файл: {e}")
            return False

def main():
    # Настройки - замените на свои данные
    EMAIL = ""  # Замените на ваш email
    PASSWORD = ""        # Замените на ваш пароль
    INPUT_FILE = "mirage_matches.txt"  # Файл с ID матчей
    OUTPUT_FILE = "match_players.txt"  # Файл для сохранения результатов
    
    print("🚀 Запуск парсера матчей INUI...")
    
    # Создаём парсер
    parser = INUIMatchParser(EMAIL, PASSWORD)
    
    # Авторизуемся
    if not parser.authenticate():
        print("❌ Не удалось авторизоваться!")
        return
    
    # Парсим все матчи
    print(f"📖 Читаем ID матчей из файла '{INPUT_FILE}'...")
    success = parser.parse_all_matches(INPUT_FILE, OUTPUT_FILE)
    
    if success:
        print(f"🎉 Парсинг завершен успешно! Результаты в файле '{OUTPUT_FILE}'")
    else:
        print("❌ Парсинг завершился с ошибками!")

if __name__ == "__main__":
    main()