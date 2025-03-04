

document.addEventListener('DOMContentLoaded', () => {
    // Elementos del DOM
    const menuItems = document.querySelectorAll('.menu-item');
    const sections = document.querySelectorAll('.content-card');
    const passwordInput = document.getElementById('networkPassword');
    const togglePassword = document.querySelector('.toggle-password');
    const firmwareFile = document.getElementById('firmwareFile');
    const flashBtn = document.getElementById('flashBtn');
    const fwProgress = document.getElementById('fwProgress');

    // Estado de la aplicación
    let selectedFile = null;
    let scheduleEntries = [];

    // Función para mostrar secciones
    const showSection = (sectionId) => {
        sections.forEach(section => {
            section.classList.remove('active', 'hidden');
            section.classList.add('hidden');
        });
        
        const activeSection = document.querySelector(`.${sectionId}-section`);
        if(activeSection) {
            activeSection.classList.remove('hidden');
            activeSection.classList.add('active');
        }
    };

    // Navegación del menú
    menuItems.forEach(item => {
        item.addEventListener('click', () => {
            menuItems.forEach(i => i.classList.remove('active'));
            item.classList.add('active');
            showSection(item.dataset.section);
        });
    });

    // Toggle contraseña
    togglePassword.addEventListener('click', () => {
        const type = passwordInput.type === 'password' ? 'text' : 'password';
        passwordInput.type = type;
        togglePassword.querySelector('.eye-icon').style.opacity = type === 'text' ? '0.7' : '1';
    });

    // Formulario de red
    document.getElementById('networkForm').addEventListener('submit', async (e) => {
        e.preventDefault();
        const btn = e.target.querySelector('button');
        btn.disabled = true;
        btn.textContent = 'Actualizando...';
    
        try {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('networkPassword').value;
    
            const response = await fetch('/connect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, pass: password })
            });
    
            if (!response.ok) throw new Error('Error en la respuesta');
            
            const result = await response.text();
            btn.textContent = '¡Red Actualizada!';
            alert(result);
        } catch (error) {
            console.error('Error:', error);
            btn.textContent = 'Error de conexión';
            alert('Error al actualizar la configuración');
        } finally {
            setTimeout(() => {
                btn.textContent = 'Actualizar Red';
                btn.disabled = false;
            }, 2000);
        }
    });

    // Gestión de firmware
let otaTimerVar = null;
let seconds = null;

// Modificar el event listener del input de archivo
document.getElementById('firmwareFile').addEventListener('change', function(e) {
    const file = e.target.files[0];
    document.getElementById('fileName').textContent = file ? `${file.name} (${(file.size/1024).toFixed(1)} KB)` : 'Ningún archivo seleccionado';
    document.getElementById('flashBtn').disabled = !file;
});

// Nueva función de actualización de firmware
function handleFirmwareUpdate() {
    const fileInput = document.getElementById('firmwareFile');
    const file = fileInput.files[0];
    
    if (!file) {
        alert('Por favor selecciona un archivo .bin primero');
        return;
    }

    const formData = new FormData();
    formData.append('file', file);
    
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/OTAupdate');
    xhr.responseType = 'blob';

    // Actualizar progreso
    xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable) {
            const percent = (e.loaded / e.total) * 100;
            document.getElementById('fwProgress').style.width = `${percent}%`;
        }
    });

    // Manejar respuesta
    xhr.onload = () => {
        if (xhr.status === 200) {
            checkOTAStatus();
        } else {
            document.getElementById('fileName').textContent = 'Error en la subida del firmware';
            document.getElementById('flashBtn').disabled = false;
        }
    };

    xhr.onerror = () => {
        document.getElementById('fileName').textContent = 'Error de conexión';
        document.getElementById('flashBtn').disabled = false;
    };

    document.getElementById('flashBtn').disabled = true;
    document.getElementById('fileName').textContent = `Subiendo ${file.name}...`;
    xhr.send(formData);
}

// Función para verificar estado OTA
function checkOTAStatus() {
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/OTAstatus');
    xhr.onload = () => {
        if (xhr.status === 200) {
            const response = JSON.parse(xhr.responseText);
            
            if (response.ota_update_status === 1) {
                seconds = 10;
                startRebootTimer();
            } else if (response.ota_update_status === -1) {
                document.getElementById('fileName').textContent = 'Error en el firmware (verifica el archivo .bin)';
            }
        }
    };
    xhr.send();
}

// Temporizador de reinicio
function startRebootTimer() {
    document.getElementById('fileName').textContent = `Actualización exitosa! Reiniciando en ${seconds} segundos...`;
    
    if (--seconds <= 0) {
        clearTimeout(otaTimerVar);
        window.location.href = '/';
    } else {
        otaTimerVar = setTimeout(startRebootTimer, 1000);
    }
}

// Actualizar el event listener del botón Flash
document.getElementById('flashBtn').addEventListener('click', handleFirmwareUpdate);

    // Programación de registros
    document.getElementById('addSchedule').addEventListener('click', (e) => {
        e.preventDefault();
        
        const dateTime = document.getElementById('scheduleTime').value;
        const days = Array.from(document.querySelectorAll('input[name="days"]:checked'))
                         .map(checkbox => checkbox.value);
        
        if (!dateTime || scheduleEntries.length >= 10) return;
        
        scheduleEntries.push({
            id: Date.now(),
            date: new Date(dateTime).toLocaleString(),
            days: days.length > 0 ? days.join(', ') : 'Una vez'
        });
        
        updateScheduleList();
        document.getElementById('scheduleTime').value = '';
        document.querySelectorAll('input[name="days"]:checked').forEach(cb => cb.checked = false);
    });

    const updateScheduleList = () => {
        const entriesList = document.getElementById('scheduleEntries');
        entriesList.innerHTML = scheduleEntries.map(entry => `
            <div class="schedule-entry">
                <div class="entry-info">
                    <div class="entry-date">${entry.date}</div>
                    <div class="entry-days">${entry.days}</div>
                </div>
                <button class="btn delete-btn" data-id="${entry.id}">🗑️</button>
            </div>
        `).join('');
        
        document.querySelectorAll('.delete-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const id = parseInt(btn.dataset.id);
                scheduleEntries = scheduleEntries.filter(entry => entry.id !== id);
                updateScheduleList();
            });
        });
    };

    // Inicialización
    showSection('network');
    document.querySelector('.menu-item[data-section="network"]').classList.add('active');
});

class InternetClock {
    constructor() {
        this.clockElement = document.querySelector('.internet-clock');
        this.loader = document.querySelector('.clock-loader');
        this.hoursElement = document.querySelector('.hours');
        this.minutesElement = document.querySelector('.minutes');
        this.secondsElement = document.querySelector('.seconds');
        this.dateElement = document.querySelector('.date-display');
        this.syncStatus = document.querySelector('.sync-status');
        this.syncBtn = document.querySelector('.sync-btn');
        
        this.timeAPI = 'https://worldtimeapi.org/api/ip';
        this.lastSync = null;
        this.timeOffset = 0;
        
        this.init();
    }

    async init() {
        this.syncBtn.addEventListener('click', () => this.syncTime());
        await this.syncTime();
        this.startClock();
    }

    async syncTime() {
        try {
            this.showLoader();
            const response = await fetch(this.timeAPI);
            const data = await response.json();
            
            const serverTime = new Date(data.utc_datetime);
            const localTime = new Date();
            this.timeOffset = serverTime - localTime;
            
            this.lastSync = new Date();
            this.updateDisplay(serverTime);
            this.syncStatus.textContent = `🟢 Sincronizado: ${data.timezone}`;
        } catch (error) {
            console.error('Error sincronizando:', error);
            this.syncStatus.textContent = '🔴 Usando hora local';
            this.timeOffset = 0;
        } finally {
            this.hideLoader();
        }
    }

    showLoader() {
        this.clockElement.classList.add('syncing');
        this.loader.style.display = 'block';
    }

    hideLoader() {
        this.clockElement.classList.remove('syncing');
        this.loader.style.display = 'none';
    }

    startClock() {
        setInterval(() => {
            const adjustedTime = new Date(Date.now() + this.timeOffset);
            this.updateDisplay(adjustedTime);
        }, 1000);
    }

    updateDisplay(time) {
        this.hoursElement.textContent = time.getHours().toString().padStart(2, '0');
        this.minutesElement.textContent = time.getMinutes().toString().padStart(2, '0');
        this.secondsElement.textContent = time.getSeconds().toString().padStart(2, '0');
        
        this.dateElement.textContent = time.toLocaleDateString('es-ES', {
            weekday: 'long',
            year: 'numeric',
            month: 'long',
            day: 'numeric'
        }).replace(/^\w/, c => c.toUpperCase());
    }
}

// Inicializar al cargar
document.addEventListener('DOMContentLoaded', () => {
    new InternetClock();
});

document.addEventListener('DOMContentLoaded', () => {
    const slider = document.getElementById('servoSlider');
    const percentageInput = document.getElementById('percentageInput');
    const percentageDisplay = document.querySelector('.percentage-display');
    const setServoBtn = document.getElementById('setServo');
    const manualBtn = document.getElementById('manualMode');
    const autoBtn = document.getElementById('autoMode');
    let currentMode = 'manual';

    // Función para mapear 0-100% a 0-180 grados
    function mapPercentageToAngle(percentage) {
        return Math.round((percentage / 100) * 180);
    }

    function updateServo(value) {
        const percentage = Math.min(100, Math.max(0, value));
        const angle = (percentage / 100) * 90;
        
        // Actualizar visualización
        document.querySelector('.shutter.left').style.transform = `rotateY(-${angle}deg)`;
        document.querySelector('.shutter.right').style.transform = `rotateY(${angle}deg)`;
        percentageDisplay.textContent = `${percentage}%`;
        
        // Enviar comando al ESP (mapeado a 0-180)
        if(currentMode === 'manual') {
            const servoAngle = mapPercentageToAngle(percentage);
            
            fetch("/set_time", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ min: servoAngle })
            })
            .then(response => response.text())
            .then(data => console.log(`Respuesta ESP: ${data}`))
            .catch(error => console.error("Error:", error));
        }
    }

    // Event listeners
    slider.addEventListener('input', (e) => {
        const value = e.target.value;
        percentageInput.value = value;
        updateServo(value);
    });

    percentageInput.addEventListener('input', (e) => {
        let value = Math.min(100, Math.max(0, e.target.value));
        slider.value = value;
        updateServo(value);
    });

    setServoBtn.addEventListener('click', () => {
        updateServo(percentageInput.value);
        alert('Posición actualizada');
    });

    // Handler de modos
    function setMode(mode) {
        currentMode = mode;
        document.querySelector('.servo-control').classList.toggle('auto-mode', mode === 'auto');
        manualBtn.classList.toggle('active', mode === 'manual');
        autoBtn.classList.toggle('active', mode === 'auto');
        
        // Enviar modo al ESP
        fetch("/set_time", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ mode: mode.toUpperCase() })
        })
        .then(response => response.text())
        .then(data => console.log(`Modo cambiado: ${data}`))
        .catch(error => console.error("Error:", error));
    }

    // Event listeners para modos
    manualBtn.addEventListener('click', () => setMode('manual'));
    autoBtn.addEventListener('click', () => setMode('auto'));

    // Inicialización
    setMode('manual');
});