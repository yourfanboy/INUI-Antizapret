const express = require('express');
const path = require('path');
const crypto = require('crypto');

const app = express();
const PORT = 1337;

// Middleware
app.use(express.urlencoded({ extended: true }));
app.use(express.json());
app.use(express.static('public'));
app.use('/public/maps', express.static(path.join(__dirname, 'maps')));
app.use('/public/svg', express.static(path.join(__dirname, 'icons')));

// Хранилище радаров
const radars = new Map();
const INACTIVE_RADAR_CLEANUP_TIME = 1 * 60 * 1000; // 1 минута

// --- Функции ---
function generateRadarId() {
    return crypto.randomBytes(8).toString('hex');
}

function generateAuthToken() {
    return crypto.randomBytes(8).toString('hex');
}

function cleanupOldRadars() {
    const now = Date.now();
    for (const [id, radar] of radars.entries()) {
        if (now - radar.lastUpdate > INACTIVE_RADAR_CLEANUP_TIME) {
            radars.delete(id);
            // ЛОГ 3: С каким ID удалился радар
            console.log(`[CLEANUP] Radar ${id} was removed due to inactivity.`);
        }
    }
}

// Запускаем очистку каждые 2 минуты
setInterval(cleanupOldRadars, 2 * 60 * 1000);

// --- Маршруты API ---

// Создание нового радара
app.get('/create', (req, res) => {
    const userAgent = req.get('User-Agent') || '';
    const userAgentInfo = req.get('User-Agent-Info') || '';

    if (!userAgent.includes('Steam') && !userAgentInfo.includes('gs_cloud_radar')) {
        return res.status(403).send('Forbidden');
    }
    
    const radarId = generateRadarId();
    const authToken = generateAuthToken();
    
    radars.set(radarId, {
        id: radarId,
        auth: authToken,
        created: Date.now(),
        lastUpdate: Date.now(),
        data: null,
        active: true
    });
    
    // ЛОГ 2: С каким ID создался радар
    console.log(`[CREATE] New radar created with ID: ${radarId}`);
    
    res.send(radarId + authToken);
});

// Обновление данных радара
app.post('/update', (req, res) => {
    const userAgentInfo = req.get('User-Agent-Info') || req.body.auth || req.query.auth || '';
    const data = req.body.data;

    if (!userAgentInfo || !data) {
        return res.status(400).send('Missing auth token or data');
    }
    
    let targetRadar = null;
    for (const radar of radars.values()) {
        if (radar.auth === userAgentInfo && radar.active) {
            targetRadar = radar;
            break;
        }
    }
    
    if (!targetRadar) {
        return res.status(404).send('Radar not found');
    }
    
    try {
        targetRadar.data = JSON.parse(data);
        targetRadar.lastUpdate = Date.now();
        res.send('OK');
    } catch (error) {
        res.status(400).send('Invalid data format');
    }
});

// Уничтожение радара (когда игрок выходит)
app.get('/destroy', (req, res) => {
    const authToken = req.get('User-Agent-Info') || req.query.auth || '';
    
    if (!authToken) return res.status(400).send('No auth token');

    for (const radar of radars.values()) {
        if (radar.auth === authToken) {
            radar.active = false;
            // ЛОГ 3 (альтернативный): Когда игрок сам выходит
            console.log(`[DESTROY] Radar ${radar.id} was deactivated by the client.`);
            break;
        }
    }
    
    res.send('Radar destroyed');
});

// Главная страница радара
app.get('/', (req, res) => {
    const auth = req.query.auth;
    if (!auth) {
        return res.send('<p>Please provide an auth parameter in the URL.</p>');
    }
    
    const radarId = auth.substring(0, 16);
    const radar = radars.get(radarId);
    
    if (!radar) {
        return res.send('<h1>Radar not found</h1><p>The radar with the specified ID does not exist or has expired.</p>');
    }
    
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// API для фронтенда
app.get('/api/radar/:id', (req, res) => {
    const radar = radars.get(req.params.id);

    // 1. Проверяем, существует ли радар вообще
    if (!radar) {
        return res.status(404).json({ error: 'Radar not found', active: false });
    }

    // --- НАЧАЛО ИСПРАВЛЕНИЯ ---

    // 2. Проверяем, был ли радар уничтожен клиентом (`/destroy`)
    // Если да, он НЕАКТИВЕН. Точка.
    if (!radar.active) {
        return res.json({
            active: false,
            lastUpdate: radar.lastUpdate,
            data: radar.data || {}
        });
    }

    // 3. Если он не был уничтожен, проверяем его по времени бездействия
    const isStillTicking = (Date.now() - radar.lastUpdate) < 60000;

    res.json({
        active: isStillTicking, // Отправляем результат проверки по времени
        lastUpdate: radar.lastUpdate,
        data: radar.data || {}
    });
});

// --- Запуск сервера ---
app.listen(PORT, '0.0.0.0', () => {
    // ЛОГ 1: Запуск сервера
    console.log(`CloudRadar server running on http://0.0.0.0:${PORT}` );
    console.log('Ready to accept radar connections...');
});

module.exports = app;
