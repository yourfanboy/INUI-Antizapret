import re, binascii, pathlib

# 1. берём строку с hex
hex_str = pathlib.Path('test.txt').read_text()

# 2. оставляем только символы 0-9 a-f A-F
hex_str = ''.join(re.findall(r'[0-9a-fA-F]+', hex_str))

data = binascii.unhexlify(hex_str)

# 3. ищем JPEG-маркеры
start = data.find(b'\xff\xd8')
end   = data.find(b'\xff\xd9', start)
if start == -1 or end == -1:
    raise RuntimeError('FF D8-FF D9 не найдено — файл усечён')

jpeg = data[start:end+2]

# 4. сохраняем
with open('recovered.jpg', 'wb') as f:
    f.write(jpeg)