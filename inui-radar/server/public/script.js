class CloudRadar {
    constructor() {
        this.radarId = new URLSearchParams(window.location.search).get("auth")?.substring(0, 16);
        this.radarContainer = document.getElementById('radarContainer');
        this.radarMap = document.getElementById('radarMap');
        this.mapNameEl = document.getElementById('mapName');
        this.playerCountEl = document.getElementById('playerCount');
        this.lastUpdateEl = document.getElementById('lastUpdate');
        this.playerListEl = document.getElementById('playerList');
        this.statusDot = document.getElementById('statusDot');
        this.statusText = document.getElementById('statusText');
        this.settingsArea = document.querySelector('.settings-area');

        this.currentMap = '';
        this.playerElements = new Map();
        this.bombElement = null;
        this.lastKnownData = null;

        this.isAdaptiveScale = true;
        this.manualScale = 1.0;
        this.minScale = 0.1;
        this.maxScale = 3.0;

        this.icons = {
            c4: `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path 
            fill="currentColor" 
            stroke="black"
            stroke-width="25"
            stroke-linejoin="round" d="M 68 20.469 C 60.759 22.545, 58.164 23.783, 53.341 27.466 C 43.660 34.858, 37.936 46.035, 36.024 61.278 C 34.701 71.835, 34.711 124.817, 36.040 136.500 C 37.471 149.093, 40.626 158.293, 45.353 163.664 C 50.213 169.185, 53.685 171, 59.385 171 C 61.852 171, 65.024 171.439, 66.435 171.975 L 69 172.950 69 203.023 L 69 233.095 71.646 235.548 L 74.292 238 140.557 238 L 206.822 238 209.411 234.923 L 212 231.847 212 185.923 L 212 140 214.566 140 C 215.977 140, 218.002 139.534, 219.066 138.965 C 220.900 137.983, 221 136.619, 221 112.572 C 221 84.776, 221.280 86, 214.918 86 L 212 86 212 67.455 C 212 49.098, 211.975 48.884, 209.545 46.455 L 207.091 44 167.545 44 L 128 44 128 40.559 C 128 38.666, 127.609 36.876, 127.131 36.581 C 126.654 36.286, 125.980 34.275, 125.634 32.113 C 125.081 28.655, 124.448 27.911, 120.376 25.940 C 108.752 20.313, 79.283 17.234, 68 20.469"/></svg>`,
            defuser: `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path fill="currentColor" d="M 32.583 20.865 C 31.875 22.012, 35.524 27.305, 50.256 46.500 C 61.896 61.665, 62.731 62.516, 66.268 62.810 C 68.321 62.981, 70 63.596, 70 64.176 C 70 66.427, 62.779 75, 60.883 75 C 58.914 75, 58.605 74.523, 57.620 69.973 C 57.140 67.751, 55.112 66.687, 40.787 61.140 C 9.707 49.106, 10.068 49.215, 8.446 51.418 C 7.354 52.901, 7.300 53.657, 8.208 54.750 C 8.853 55.528, 18.703 63.765, 30.097 73.055 C 47.725 87.429, 51.963 90.403, 58.534 93.013 C 70.146 97.626, 71.305 98.398, 76.063 104.690 C 88.284 120.848, 88.731 122.135, 91.446 148.937 C 92.258 156.947, 93.633 165.865, 94.501 168.756 C 99.529 185.484, 109.758 202.913, 121.130 214.128 C 128.984 221.874, 140.323 230.690, 146.592 233.925 C 150.250 235.813, 151.021 235.895, 153.873 234.696 C 157.940 232.985, 161.217 227.080, 160.805 222.205 C 160.539 219.062, 159.614 217.930, 153.500 213.257 C 143.134 205.334, 136.485 198.454, 130.539 189.500 C 121.417 175.762, 117.281 164.355, 114.523 145.324 C 113.632 139.177, 112.475 132.655, 111.951 130.831 C 111.428 129.006, 111 126.653, 111 125.602 C 111 123.765, 111.077 123.760, 112.963 125.466 C 114.847 127.171, 118 127.001, 118 125.195 C 118 124.718, 115.843 122.590, 113.207 120.466 C 109.116 117.170, 98.749 103.700, 97.805 100.453 C 97.637 99.877, 98.709 97.964, 100.187 96.201 L 102.873 92.997 110.187 95.637 C 117.005 98.098, 118.245 98.233, 128.500 97.631 C 134.550 97.275, 143.775 96.316, 149 95.499 C 176.970 91.126, 196.644 95.711, 226.587 113.578 C 236.949 119.761, 241.180 120.470, 245.506 116.748 C 249.320 113.468, 249.992 107.236, 246.842 104.363 C 242.709 100.593, 222.787 87.398, 213.500 82.280 C 199.005 74.293, 191.809 72.835, 167.044 72.867 C 155.645 72.881, 139.816 73.379, 131.868 73.973 C 117.838 75.020, 117.132 74.978, 107.644 72.512 C 92.311 68.528, 92.069 68.370, 84.022 57.100 C 80.056 51.545, 76.410 47, 75.919 47 C 75.428 47, 66.365 40.925, 55.779 33.500 C 37.712 20.827, 33.893 18.747, 32.583 20.865"/></svg>`
        };

        this.mapData = {
            'default': { pos_x: -2476, pos_y: 3239, scale: 4.4 },
            'de_dust2': { pos_x: -2476, pos_y: 3239, scale: 4.4 },
            'de_mirage': { pos_x: -3230, pos_y: 1713, scale: 5.0 },
            'de_inferno': { pos_x: -2087, pos_y: 3870, scale: 4.9 },
            'de_cache': { pos_x: -2000, pos_y: 3250, scale: 5.5 },
            'de_overpass': { pos_x: -4831, pos_y: 1781, scale: 5.2 },
            'de_cbble': { pos_x: -3840, pos_y: 3072, scale: 6.0 },
            'de_train': { pos_x: -2477, pos_y: 2392, scale: 4.7 },
            'de_nuke': { pos_x: -3453, pos_y: 2887, scale: 7.0 },
            'de_vertigo': { pos_x: -3168, pos_y: 1762, scale: 4.0 }
        };

        this.settings = {
            showNames: true,
            showHealth: true,
            smoothMovement: true,
            mouseWheelZoom: true,
            textSize: 10,
            adaptiveScale: true,
            view: {
                style: 'off',
                lineLength: 50,
                lineColor: '#ff00ff',
                fovAngle: 90,
                fovLength: 100,
                fovColor: '#ff00ff'
            },
            colors: {
                bomb: '#ff4444',
                defuser: '#44ff44',
                terroristNormal: '#ff6b35',
                terroristWithBomb: '#ff0000',
                counterTerrorist: '#4169e1'
            }
        };

        this.panX = 0;
        this.panY = 0;
        this.isPanning = false;
        this.lastMouseX = 0;
        this.lastMouseY = 0;
        this.isAdaptiveScale = true;

        this.initializeEventListeners();
        this.loadSettings();
        this.updateScale();

        if (!this.radarId) {
            this.showError('Не указан параметр auth в URL');
            return;
        }

        this.startUpdating();
    }

    initializeEventListeners() {
        // Внутри функции initializeEventListeners()

        document.querySelectorAll('.nav-tab').forEach(tab => {
            tab.addEventListener('click', () => {
                const isAlreadyActive = tab.classList.contains('active');
                const isPanelHidden = this.settingsArea.classList.contains('hidden');

                if (isAlreadyActive) {
                    // КЛИК ПО АКТИВНОЙ ВКЛАДКЕ: Скрываем панель и убираем подсветку
                    this.settingsArea.classList.add('hidden');
                    document.querySelectorAll('.nav-tab').forEach(t => t.classList.remove('active'));

                } else if (isPanelHidden) {
                    // КЛИК ПО ЛЮБОЙ ВКЛАДКЕ, КОГДА ПАНЕЛЬ СКРЫТА: Показываем панель и активируем эту вкладку
                    this.settingsArea.classList.remove('hidden');
                    tab.classList.add('active');

                    // Показываем соответствующий контент
                    const tabId = tab.getAttribute('data-tab');
                    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
                    document.getElementById(tabId + 'Content').classList.add('active');

                } else {
                    // ОБЫЧНОЕ ПЕРЕКЛЮЧЕНИЕ МЕЖДУ ВКЛАДКАМИ (когда панель видима)
                    document.querySelectorAll('.nav-tab').forEach(t => t.classList.remove('active'));
                    tab.classList.add('active');

                    const tabId = tab.getAttribute('data-tab');
                    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
                    document.getElementById(tabId + 'Content').classList.add('active');
                }

                // В любом случае, пересчитываем масштаб радара после анимации
                setTimeout(() => {
                    this.updateScale();
                }, 300);
            });
        });


        document.getElementById("adaptiveScale").addEventListener("change", (e) => {
            this.isAdaptiveScale = e.target.checked;
            if (this.isAdaptiveScale) {
                this.updateScale();
            }
        });

        this.radarContainer.addEventListener('wheel', (e) => {
            if (this.settings.mouseWheelZoom) {
                e.preventDefault();
                this.handleWheelZoom(e);
            }
        });

        this.radarContainer.addEventListener('mousedown', (e) => {
            if (e.button === 1 && this.settings.mousePan) {
                e.preventDefault();
                this.isPanning = true;
                this.lastMouseX = e.clientX;
                this.lastMouseY = e.clientY;
                this.radarContainer.style.cursor = 'grabbing';
            }
        });

        this.radarContainer.addEventListener('mousemove', (e) => {
            if (this.isPanning && this.settings.mousePan) {
                e.preventDefault();
                const deltaX = e.clientX - this.lastMouseX;
                const deltaY = e.clientY - this.lastMouseY;

                this.panX += deltaX;
                this.panY += deltaY;

                this.updatePan();

                this.lastMouseX = e.clientX;
                this.lastMouseY = e.clientY;
            }
        });

        this.radarContainer.addEventListener('mouseup', (e) => {
            if (e.button === 1) {
                this.isPanning = false;
                this.radarContainer.style.cursor = 'default';
            }
        });

        this.radarContainer.addEventListener('mouseleave', () => {
            this.isPanning = false;
            this.radarContainer.style.cursor = 'default';
        });

        this.radarContainer.addEventListener('contextmenu', (e) => {
            if (e.button === 1) {
                e.preventDefault();
            }
        });

        window.addEventListener('resize', () => {
            if (this.isAdaptiveScale) {
                this.updateScale();
            }
        });

        document.getElementById('toggleNames').addEventListener('change', (e) => {
            this.settings.showNames = e.target.checked;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('toggleHealth').addEventListener('change', (e) => {
            this.settings.showHealth = e.target.checked;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('viewStyle').addEventListener('change', (e) => {
            this.settings.view.style = e.target.value;
            this.updateViewSettings();
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('lineLength').addEventListener('input', (e) => {
            this.settings.view.lineLength = parseInt(e.target.value);
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('fovAngle').addEventListener('input', (e) => {
            this.settings.view.fovAngle = parseInt(e.target.value);
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('lineColor').addEventListener('change', (e) => {
            this.settings.view.lineColor = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('fovColor').addEventListener('change', (e) => {
            this.settings.view.fovColor = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('bombColor').addEventListener('change', (e) => {
            this.settings.colors.bomb = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('defuserColor').addEventListener('change', (e) => {
            this.settings.colors.defuser = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('terroristNormalColor').addEventListener('change', (e) => {
            this.settings.colors.terroristNormal = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('terroristWithBombColor').addEventListener('change', (e) => {
            this.settings.colors.terroristWithBomb = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('counterTerroristColor').addEventListener('change', (e) => {
            this.settings.colors.counterTerrorist = e.target.value;
            this.saveSettings();
            this.forceFullRender();
        });

        document.getElementById('textSize').addEventListener('input', (e) => {
            this.settings.textSize = parseInt(e.target.value);
            this.saveSettings();
            this.updateTextSizes();
        });

        document.getElementById('mouseWheelZoom').addEventListener('change', (e) => {
            this.settings.mouseWheelZoom = e.target.checked;
            this.saveSettings();
        });

        document.getElementById('mousePan').addEventListener('change', (e) => {
            this.settings.mousePan = e.target.checked;
            this.saveSettings();
        });

        document.getElementById('smoothMovement').addEventListener('change', (e) => {
            this.settings.smoothMovement = e.target.checked;
            this.saveSettings();
            this.updateSmoothMovement();
        });
    }

    loadSettings() {
        const saved = localStorage.getItem('radarSettings');
        if (saved) {
            try {
                const parsed = JSON.parse(saved);
                // Удаляем debugMode из загруженных настроек, чтобы он не появился снова
                delete parsed.debugMode;
                this.settings = { ...this.settings, ...parsed };
            } catch (e) {
                console.warn('Failed to load settings:', e);
            }
        }

        document.getElementById('toggleNames').checked = this.settings.showNames;
        document.getElementById('toggleHealth').checked = this.settings.showHealth;
        document.getElementById('smoothMovement').checked = this.settings.smoothMovement;
        document.getElementById("mouseWheelZoom").checked = this.settings.mouseWheelZoom;
        document.getElementById("textSize").value = this.settings.textSize;
        document.getElementById('viewStyle').value = this.settings.view.style;
        document.getElementById('lineLength').value = this.settings.view.lineLength;
        document.getElementById('lineColor').value = this.settings.view.lineColor;
        document.getElementById('fovAngle').value = this.settings.view.fovAngle;
        document.getElementById('fovColor').value = this.settings.view.fovColor;
        document.getElementById('bombColor').value = this.settings.colors.bomb;
        document.getElementById('defuserColor').value = this.settings.colors.defuser;
        document.getElementById('terroristNormalColor').value = this.settings.colors.terroristNormal;
        document.getElementById('terroristWithBombColor').value = this.settings.colors.terroristWithBomb;
        document.getElementById('counterTerroristColor').value = this.settings.colors.counterTerrorist;

        this.updateViewSettings();
        this.updateSmoothMovement();
        this.updateTextSizes();
        this.isAdaptiveScale = this.settings.adaptiveScale;
        this.updateScale();
    }

    saveSettings() {
        localStorage.setItem('radarSettings', JSON.stringify(this.settings));
    }

    updateViewSettings() {
        const style = this.settings.view.style;
        document.getElementById('lineSettings').style.display = style === 'line' ? 'flex' : 'none';
        document.getElementById('fovSettings').style.display = style === 'fov' ? 'flex' : 'none';
    }

    updateSmoothMovement() {
        this.playerElements.forEach((elements) => {
            if (this.settings.smoothMovement) {
                elements.container.style.transition = 'transform 0.2s ease-out';
                elements.viewIndicator.style.transition = 'transform 0.2s ease-out, opacity 0.1s ease';
            } else {
                elements.container.style.transition = 'none';
                elements.viewIndicator.style.transition = 'none';
            }
        });
    }

    updateTextSizes() {
        this.playerElements.forEach((elements) => {
            elements.nameEl.style.fontSize = `${this.settings.textSize}px`;
            elements.healthEl.style.fontSize = `${this.settings.textSize - 1}px`;
        });
    }

    handleWheelZoom(e) {
        if (this.isAdaptiveScale) {
            this.isAdaptiveScale = false;
            document.getElementById("adaptiveScale").checked = false;
            this.manualScale = this.getCurrentScale();
        }

        const delta = e.deltaY > 0 ? -0.1 : 0.1;
        this.manualScale = Math.max(this.minScale, Math.min(this.maxScale, this.manualScale + delta));
        this.applyScale(this.manualScale);
    }

    updateScale() {
        if (!this.isAdaptiveScale) return;

        const containerWidth = this.radarContainer.clientWidth;
        const containerHeight = this.radarContainer.clientHeight;
        const radarSize = 1024;

        const scaleX = (containerWidth * 0.8) / radarSize;
        const scaleY = (containerHeight * 0.8) / radarSize;
        const scale = Math.min(scaleX, scaleY, 1);

        this.applyScale(scale);
    }

    getCurrentScale() {
        const transform = getComputedStyle(this.radarMap).transform;
        if (transform === 'none') return 1;

        const matrix = transform.match(/matrix\(([^)]+)\)/);
        if (matrix) {
            const values = matrix[1].split(',');
            return parseFloat(values[0]);
        }
        return 1;
    }

    applyScale(scale) {
        this.radarMap.style.setProperty('--radar-scale', scale);
    }

    updatePan() {
        this.radarMap.style.setProperty('--pan-x', `${this.panX}px`);
        this.radarMap.style.setProperty('--pan-y', `${this.panY}px`);
    }

    resetPan() {
        this.panX = 0;
        this.panY = 0;
        this.updatePan();
    }

    forceFullRender() {
        if (this.lastKnownData) {
            this.renderRadar(this.lastKnownData);
        }
    }

    renderRadar(data) {
        this.lastKnownData = data;

        if (data[64] && this.currentMap !== data[64]) {
            this.currentMap = data[64];
            this.mapNameEl.textContent = this.currentMap;
            this.radarMap.style.backgroundImage = `url('/maps/${this.currentMap}.png')`;
        }

        const seenPlayers = new Set();
        const playerListData = [];

        for (let i = 0; i <= 63; i++) {
            const playerData = data[i];
            if (playerData && Array.isArray(playerData) && playerData.length >= 10) {
                seenPlayers.add(i);
                this.updatePlayer(i, playerData);
                playerListData.push({ id: i, data: playerData });
            }
        }

        for (const entIndex of this.playerElements.keys()) {
            if (!seenPlayers.has(entIndex)) {
                this.playerElements.get(entIndex).container.remove();
                this.playerElements.delete(entIndex);
            }
        }

        this.updateBomb(data[65]);
        this.updatePlayerList(playerListData);
        this.playerCountEl.textContent = seenPlayers.size;
    }

    updatePlayer(entIndex, playerData) {
        const [name, steam, x, y, z, lookX, lookY, dormant, team, alive, health, _p1, _p2, hasBomb] = playerData;

        let p_elements = this.playerElements.get(entIndex);
        if (!p_elements) {
            const container = document.createElement('div');
            container.className = 'player-container';

            const viewIndicator = document.createElement('div');
            viewIndicator.className = 'view-indicator';

            const playerDot = document.createElement('div');
            playerDot.className = 'player-dot';

            const nameEl = document.createElement('div');
            nameEl.className = 'player-name';

            const healthEl = document.createElement('div');
            healthEl.className = 'player-health';

            container.appendChild(viewIndicator);
            container.appendChild(playerDot);
            playerDot.appendChild(nameEl);
            playerDot.appendChild(healthEl);
            this.radarMap.appendChild(container);

            p_elements = { container, viewIndicator, playerDot, nameEl, healthEl };
            this.playerElements.set(entIndex, p_elements);

            if (this.settings.smoothMovement) {
                container.style.transition = 'transform 0.2s ease-out';
                viewIndicator.style.transition = 'transform 0.2s ease-out, opacity 0.1s ease';
            } else {
                container.style.transition = 'none';
                viewIndicator.style.transition = 'none';
            }

            nameEl.style.fontSize = `${this.settings.textSize}px`;
            healthEl.style.fontSize = `${this.settings.textSize - 1}px`;
        }

        const playerPos = this.worldToRadar(x, y);
        p_elements.container.style.transform = `translate(${playerPos.x}px, ${playerPos.y}px)`;

        let playerColor, borderColor;
        if (team === 2) {
            if (hasBomb) {
                playerColor = this.settings.colors.terroristWithBomb;
            } else {
                playerColor = this.settings.colors.terroristNormal;
            }
            borderColor = this.darkenColor(playerColor, 0.3);
        } else if (team === 3) {
            playerColor = this.settings.colors.counterTerrorist;
            borderColor = this.darkenColor(playerColor, 0.3);
        } else {
            playerColor = '#666';
            borderColor = '#999';
        }

        p_elements.playerDot.className = `player-dot ${dormant ? 'dormant' : ''} ${!alive ? 'dead' : ''}`;
        if (alive) {
            p_elements.playerDot.style.backgroundColor = playerColor;
            p_elements.playerDot.style.borderColor = borderColor;
        }

        p_elements.nameEl.textContent = name;
        p_elements.nameEl.style.display = this.settings.showNames ? 'block' : 'none';

        p_elements.healthEl.textContent = health;
        p_elements.healthEl.style.display = this.settings.showHealth && alive ? 'block' : 'none';
        p_elements.healthEl.style.color = health > 75 ? '#4f4' : health > 25 ? '#ff4' : '#f44';

        const viewEl = p_elements.viewIndicator;
        viewEl.style.display = 'none';

        if (alive && lookX !== undefined && lookY !== undefined && this.settings.view.style !== 'off') {
            const lookPos = this.worldToRadar(lookX, lookY);
            const dx = lookPos.x - playerPos.x;
            const dy = lookPos.y - playerPos.y;
            const angleDeg = Math.atan2(dy, dx) * 180 / Math.PI;

            viewEl.style.display = 'block';

            if (this.settings.view.style === 'line') {
                viewEl.className = 'view-indicator';
                viewEl.style.height = '2px';
                viewEl.style.width = `${this.settings.view.lineLength}px`;
                viewEl.style.backgroundColor = this.settings.view.lineColor;
                viewEl.style.clipPath = 'none';
                viewEl.style.transform = `translateY(-50%) rotate(${angleDeg}deg)`;
                viewEl.style.opacity = '0.8';
            } else if (this.settings.view.style === 'fov') {
                viewEl.className = 'view-indicator fov-cone';
                const length = this.settings.view.fovLength;
                const angle = this.settings.view.fovAngle;
                viewEl.style.width = `${length}px`;
                viewEl.style.height = `${length * 2}px`;
                viewEl.style.backgroundColor = this.settings.view.fovColor;
                viewEl.style.clipPath = `polygon(0 50%, 100% ${50 - angle / 2}%, 100% ${50 + angle / 2}%)`;
                viewEl.style.transform = `translateY(-50%) rotate(${angleDeg}deg)`;
                viewEl.style.opacity = '0.3';
            }
        }
    }

    // ЗАМЕНИТЕ ВСЮ ФУНКЦИЮ updateBomb НА ЭТУ:
    // ПОЛНОСТЬЮ ЗАМЕНИТЕ ФУНКЦИЮ updateBomb НА ЭТУ ВЕРСИЮ
    updateBomb(bombData) {
        // Проверяем, есть ли данные о бомбе
        if (bombData && Array.isArray(bombData) && bombData.length >= 3) {
            const [planted, x, y] = bombData;

            // Если координат нет, скрываем все и выходим
            if (x === 0 && y === 0) {
                if (this.bombElement) {
                    this.bombElement.style.display = 'none';
                }
                return;
            }

            // Если иконки еще нет, создаем ее ОДИН РАЗ вместе с текстом
            if (!this.bombElement) {
                this.bombElement = document.createElement('div');
                this.bombElement.className = 'bomb-icon';

                // Создаем SVG иконку
                const svgIcon = document.createElement('div'); // Временный div для парсинга
                svgIcon.innerHTML = this.icons.c4;

                // Создаем элемент для текста
                const textEl = document.createElement('div');
                textEl.className = 'bomb-text';
                textEl.textContent = 'Planted';

                // Добавляем иконку и текст в главный контейнер бомбы
                this.bombElement.appendChild(svgIcon.firstChild);
                this.bombElement.appendChild(textEl);

                this.radarMap.appendChild(this.bombElement);
            }

            // Обновляем позицию и видимость
            const bombPos = this.worldToRadar(x, y);
            this.bombElement.style.transform = `translate(${bombPos.x}px, ${bombPos.y}px) translate(-50%, -50%)`;
            this.bombElement.style.color = this.settings.colors.bomb;
            this.bombElement.style.display = 'block';

            // Находим наш текстовый элемент внутри
            const bombText = this.bombElement.querySelector('.bomb-text');

            // Управляем классом .planted и видимостью текста
            if (planted) {
                this.bombElement.classList.add('planted');
                if (bombText) bombText.style.display = 'block'; // Показываем текст
            } else {
                this.bombElement.classList.remove('planted');
                if (bombText) bombText.style.display = 'none'; // Скрываем текст
            }

        } else {
            // Если данных о бомбе нет вообще, скрываем иконку
            if (this.bombElement) {
                this.bombElement.style.display = 'none';
            }
        }
    }


    checkPlayerHasBomb(entIndex) {
        // Проверяем, есть ли у игрока бомба
        // Это можно определить по данным бомбы или специальному флагу
        if (this.lastKnownData && this.lastKnownData[65]) {
            const bombData = this.lastKnownData[65];
            if (Array.isArray(bombData) && bombData.length >= 4) {
                const [bombX, bombY, bombZ, carrier] = bombData;
                return carrier === entIndex;
            }
        }
        return false;
    }

    darkenColor(color, factor) {
        // Затемняем цвет для границы
        const hex = color.replace('#', '');
        const r = parseInt(hex.substr(0, 2), 16);
        const g = parseInt(hex.substr(2, 2), 16);
        const b = parseInt(hex.substr(4, 2), 16);

        const newR = Math.floor(r * (1 - factor));
        const newG = Math.floor(g * (1 - factor));
        const newB = Math.floor(b * (1 - factor));

        return `#${newR.toString(16).padStart(2, '0')}${newG.toString(16).padStart(2, '0')}${newB.toString(16).padStart(2, '0')}`;
    }

    updatePlayerList(players) {
        this.playerListEl.innerHTML = '';
        const teams = { 2: [], 3: [] };

        players.forEach(player => {
            const team = player.data[8];
            if (teams[team]) teams[team].push(player);
        });

        if (teams[2].length > 0) {
            const tHeader = document.createElement('div');
            tHeader.innerHTML = '<div class="team-header team-t">Terrorists</div>';
            this.playerListEl.appendChild(tHeader);

            teams[2].forEach(player => {
                const item = this.createPlayerListItem(player, 'team-t');
                this.playerListEl.appendChild(item);
            });
        }

        if (teams[3].length > 0) {
            const ctHeader = document.createElement('div');
            ctHeader.innerHTML = '<div class="team-header team-ct">Counter-Terrorists</div>';
            ctHeader.style.marginTop = '10px';
            this.playerListEl.appendChild(ctHeader);

            teams[3].forEach(player => {
                const item = this.createPlayerListItem(player, 'team-ct');
                this.playerListEl.appendChild(item);
            });
        }
    }

    createPlayerListItem(player, teamClass) {
        const item = document.createElement('div');
        const alive = player.data[9];
        const health = player.data[10] || 0;
        const hasBomb = player.data[13];
        const hasDefuser = player.data[14];

        let extras = '';
        if (hasBomb) extras += `<span class="icon" style="color: ${this.settings.colors.bomb}">${this.icons.c4}</span>`;
        if (hasDefuser) extras += `<span class="icon" style="color: ${this.settings.colors.defuser}">${this.icons.defuser}</span>`;

        item.innerHTML = `<span>${player.data[0]}${extras}</span><span>${alive ? health + 'HP' : 'DEAD'}</span>`;
        item.className = `player-item ${alive ? teamClass : 'team-dead'}`;
        return item;
    }

    worldToRadar(worldX, worldY) {
        const mapInfo = this.mapData[this.currentMap] || this.mapData['default'];
        if (worldX === undefined || worldY === undefined) return { x: 0, y: 0 };

        const translatedX = parseFloat(worldX) - mapInfo.pos_x;
        const translatedY = parseFloat(worldY) - mapInfo.pos_y;

        let mapX = translatedX / mapInfo.scale;
        let mapY = -translatedY / mapInfo.scale;

        mapX = Math.max(0, Math.min(1024, mapX));
        mapY = Math.max(0, Math.min(1024, mapY));

        return { x: mapX, y: mapY };
    }

    async startUpdating() {
        setInterval(() => this.fetchData(), 200);
    }

    async fetchData() {
        try {
            const response = await fetch(`/api/radar/${this.radarId}`);
            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            const radarData = await response.json();
            if (!radarData.active) {
                this.setStatus(false, 'Неактивен');
                return;
            }

            this.setStatus(true, 'Активен');
            if (radarData.data) {
                this.renderRadar(radarData.data);
            }

            this.updateLastUpdateTime(radarData.lastUpdate);
        } catch (error) {
            this.setStatus(false, 'Ошибка подключения');
        }
    }

    updateLastUpdateTime(timestamp) {
        const now = Date.now();
        const diff = now - timestamp;

        if (diff < 1000) this.lastUpdateEl.textContent = 'только что';
        else if (diff < 60000) this.lastUpdateEl.textContent = `${Math.floor(diff / 1000)}s назад`;
        else this.lastUpdateEl.textContent = `${Math.floor(diff / 60000)}m назад`;
    }

    setStatus(active, text) {
        this.statusDot.classList.toggle('active', active);
        this.statusText.textContent = text;
    }

    showError(message) {
        console.error(message);
        this.setStatus(false, 'Ошибка');
    }
}

document.addEventListener('DOMContentLoaded', () => {
    new CloudRadar();
});