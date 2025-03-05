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

    document.getElementById('firmwareFile').addEventListener('change', function(e) {
        const file = e.target.files[0];
        document.getElementById('fileName').textContent = file ? `${file.name} (${(file.size/1024).toFixed(1)} KB)` : 'Ningún archivo seleccionado';
        document.getElementById('flashBtn').disabled = !file;
    });

//MARK: Handle Firmeware 
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
    
        // Variables para el cálculo de tiempo
        let uploadStartTime = Date.now();
        let previousLoaded = 0;
    
        xhr.upload.addEventListener('progress', (e) => {
            if (e.lengthComputable) {
                const percent = (e.loaded / e.total) * 100;
                document.getElementById('fwProgress').style.width = `${percent}%`;
                document.getElementById('progressText').textContent = `${Math.round(percent)}%`;
                
                // Calcular tiempo restante
                const currentTime = Date.now();
                const elapsed = (currentTime - uploadStartTime) / 1000; // segundos
                const speed = e.loaded / elapsed; // bytes/segundo
                const remaining = (e.total - e.loaded) / speed;
                
                document.getElementById('fileName').innerHTML = `
                    Subiendo ${file.name}...<br>
                    <small>Tiempo restante: ${Math.ceil(remaining)} segundos</small>
                `;
            }
        });
    
        xhr.onload = () => {
            if (xhr.status === 200) {
                checkOTAStatus();
            } else {
                document.getElementById('fileName').textContent = 'Error en la subida del firmware';
                document.getElementById('flashBtn').disabled = false;
            }
            // Resetear el formulario después de la carga
            fileInput.value = '';
            document.getElementById('fileName').textContent = 'Ningún archivo seleccionado';
        };
    
        xhr.onerror = () => {
            document.getElementById('fileName').innerHTML = `
        <span class="error">Error de conexión</span>
        <small>Intente nuevamente</small>`;
            document.getElementById('flashBtn').disabled = false;
        };
    
        document.getElementById('flashBtn').disabled = true;
        xhr.send(formData);
    }

    function checkOTAStatus() {
        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/OTAstatus');
        xhr.onload = () => {
            if (xhr.status === 200) {
                const response = JSON.parse(xhr.responseText);
                
                if (response.ota_update_status === 1) {
                    seconds = 15;  // Aumentamos el tiempo para dar margen al reinicio
                    startRebootTimer();
                    
                } else if (response.ota_update_status === -1) {
                    document.getElementById('fileName').textContent = 'Error en el firmware';
                }
            } else {
                // Si hay error de conexión (servidor reiniciando), forzar recarga
                setTimeout(() => {
                    window.location.href = '/';
                }, 5000);
            }
        };
        
        xhr.onerror = () => {
            // Si falla la conexión, intentar recargar después de 5 seg
            setTimeout(() => {
                window.location.href = '/';
            }, 5000);
        };
        
        xhr.send();
    }

    function startRebootTimer() {
        const countdownElement = document.getElementById('fileName');
        
        const updateTimer = () => {
            countdownElement.innerHTML = `
                Actualización exitosa!<br>
                <small>Reiniciando en ${seconds} segundos... (Recargue si se demora)</small>
            `;
            
            if (percent == 0) {
                clearInterval(otaTimerVar);
                window.location.href = window.location.href; // Forzar recarga completa
            }
        };
        
        updateTimer(); // Actualización inmediata
        otaTimerVar = setInterval(updateTimer, 1000);
    }
    document.getElementById('flashBtn').addEventListener('click', handleFirmwareUpdate);

    // Inicialización
    showSection('network');
    document.querySelector('.menu-item[data-section="network"]').classList.add('active');
});


//MARK: SCHEDULE MANAGER
class ScheduleManager {
    constructor() {
        this.scheduleEntries = [];
        this.registroCounter = 1;
        this.init();
        this.daysOrder = ["Lun", "Mar", "Mié", "Jue", "Vie", "Sáb", "Dom"];
        this.loadRegisters();
        this.registerElements = {};
        this.registerElements = Array.from({length: 10}, (_, i) => 
            document.getElementById(`reg_${i + 1}`));
        this.loadButton = document.getElementById('loadRegistersButton');
        if (this.loadButton) {
            this.loadButton.addEventListener('click', () => this.loadRegisters());
        }
    }

    init() {
        this.bindEvents();
        this.updateScheduleList();
        this.loadButton?.addEventListener('click', () => this.loadRegisters());
    }

    bindEvents() {
        document.getElementById('addSchedule')?.addEventListener('click', (e) => this.addSchedule(e));
        
    }

    async addSchedule(e) {
        e.preventDefault();
        
        const dateTime = document.getElementById('scheduleTime').value;
        const angle = document.getElementById('angleInput').value;
        
        if (!dateTime) return;
        
        const date = new Date(dateTime);
        const hours = date.getHours().toString().padStart(2, '0');
        const minutes = date.getMinutes().toString().padStart(2, '0');
        
        const selectedDays = this.daysOrder.map(day => 
            document.querySelector(`input[name="days"][value="${day}"]`).checked ? "1" : "0"
        );

        const numeroRegistro = this.findAvailableRegistroNumber();
        if (numeroRegistro === null) {
            alert("No se pueden agregar más de 10 registros.");
            return;
        }

        try {
            await this.sendRegister(numeroRegistro, hours, minutes, selectedDays, angle);
            
            this.scheduleEntries.push({
                id: Date.now(),
                numeroRegistro: numeroRegistro,
                date: date.toLocaleString('es-ES'),
                days: selectedDays.includes("1") ? this.getSelectedDaysLabels(selectedDays) : 'Una vez',
                angle: angle
            });
            
            this.updateScheduleList();
            this.resetForm();
            this.loadRegisters();
        } catch (error) {
            console.error('Error al guardar registro:', error);
            alert('Error al guardar el registro en el dispositivo');
        }
    }

    async sendRegister(regNumber, hours, minutes, days, angle) {
        const requestData = {
            selectedNumber: regNumber.toString(),
            hours: hours,
            minutes: minutes,
            selectedDays: days,
            angle: angle
        };

        const response = await fetch('/regchange.json', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(requestData)
        });

        if (!response.ok) throw new Error('Error en la respuesta del servidor');
    }

    getSelectedDaysLabels(daysArray) {
        return daysArray
            .map((val, index) => val === "1" ? this.daysOrder[index] : null)
            .filter(val => val !== null)
            .join(', ');
    }

    async loadRegisters() {
        try {
            const response = await fetch('/read_regs.json', {
                method: 'GET'  // Ahora usa GET
            });
            
            if (!response.ok) throw new Error('Error en la respuesta');
            
            const data = await response.json();
            
            this.registerElements.forEach((element, index) => {
                const regNumber = index + 1;
                const regValue = data[`reg${regNumber}`] || 'Vacío';
                element.textContent = regValue;
                element.classList.toggle('active-register', regValue !== 'Vacío');
            });
            
        } catch (error) {
            console.error('Error cargando registros:', error);
            alert('Error al cargar registros');
        }
    }

    init() {
        this.bindEvents();
        this.updateScheduleList();
        this.createRegisterDisplay(); // Nueva función
    }

    createRegisterDisplay() {
        const container = document.getElementById('currentRegisters');
        
        this.registerElements = Array.from({length: 10}, (_, i) => {
            const regNumber = i + 1;
            const regElement = document.createElement('div');
            regElement.className = 'register-item';
            regElement.innerHTML = `
                <span class="reg-number">Registro ${regNumber}</span>
                <span class="reg-value" id="reg_${regNumber}">Cargando...</span>
            `;
            container.appendChild(regElement);
            return document.getElementById(`reg_${regNumber}`);
        });
    }

    async deleteSchedule(id) {
        const entry = this.scheduleEntries.find(e => e.id === parseInt(id));
        if (!entry) return;

        try {
            await this.eraseRegister(entry.numeroRegistro);
            this.scheduleEntries = this.scheduleEntries.filter(e => e.id !== parseInt(id));
            this.updateScheduleList();
            this.loadRegisters();
        } catch (error) {
            console.error('Error eliminando registro:', error);
            alert('Error al eliminar el registro del dispositivo');
        }
    }

    async eraseRegister(regNumber) {
        const response = await fetch('/regerase.json', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ selectedNumber: regNumber.toString() })
        });

        if (!response.ok) throw new Error('Error en la respuesta del servidor');
    }

    findAvailableRegistroNumber() {
        const usedNumbers = this.scheduleEntries.map(entry => entry.numeroRegistro);
        for (let i = 1; i <= 10; i++) {
            if (!usedNumbers.includes(i)) return i;
        }
        return null;
    }

    updateScheduleList() {
        const entriesList = document.getElementById('scheduleEntries');
        if (!entriesList) return;

        entriesList.innerHTML = this.scheduleEntries.map(entry => `
            <div class="schedule-entry">
                <div class="entry-info">
                    <div class="entry-number">Registro #${entry.numeroRegistro}</div>
                    <div class="entry-date">${entry.date}</div>
                    <div class="entry-days">${entry.days}</div>
                    <div class="entry-angle">Ángulo: ${entry.angle}°</div>
                </div>
                <button class="btn delete-btn" data-id="${entry.id}">🗑️</button>
            </div>
        `).join('');

        document.querySelectorAll('.delete-btn').forEach(btn => {
            btn.addEventListener('click', () => this.deleteSchedule(btn.dataset.id));
        });
    }

    resetForm() {
        document.getElementById('scheduleTime').value = '';
        document.querySelectorAll('input[name="days"]:checked').forEach(cb => cb.checked = false);
        document.getElementById('angleInput').value = 0;
    }
}

// Inicialización del ScheduleManager
document.addEventListener('DOMContentLoaded', () => {
    new ScheduleManager();
});

//MARK: INTERNET CLOCK
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

document.addEventListener('DOMContentLoaded', () => {
    new InternetClock();
});

//MARK:SERVO CONTROL

document.addEventListener('DOMContentLoaded', () => {
    const slider = document.getElementById('servoSlider');
    const percentageInput = document.getElementById('percentageInput');
    const percentageDisplay = document.querySelector('.percentage-display');
    const setServoBtn = document.getElementById('setServo');
    const manualBtn = document.getElementById('manualMode');
    const autoBtn = document.getElementById('autoMode');
    let currentMode = 'manual';
    let pendingValue = 0;

    // Función solo para actualizar la visualización
    function updateVisualization(value) {
        const percentage = Math.min(100, Math.max(0, value));
        const angle = (percentage / 100) * 90;
        
        document.querySelector('.shutter.left').style.transform = `rotateY(-${angle}deg)`;
        document.querySelector('.shutter.right').style.transform = `rotateY(${angle}deg)`;
        percentageDisplay.textContent = `${percentage}%`;
        pendingValue = percentage; // Guardar el valor pendiente
    }

    // Función para enviar la posición al servidor
    function updateServo() {
        if(currentMode !== 'manual') return;
        
        const servoAngle = Math.round((pendingValue / 100) * 180);
        
        fetch("/set_mode_manual", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ min: servoAngle })
        })
        .then(response => response.text())
        .then(data => {
            console.log(`Respuesta ESP: ${data}`);
            setServoBtn.textContent = '✓ Aplicado!';
            setTimeout(() => setServoBtn.textContent = 'Aplicar', 2000);
        })
        .catch(error => {
            console.error("Error:", error);
            setServoBtn.textContent = '✗ Error!';
            setTimeout(() => setServoBtn.textContent = 'Aplicar', 2000);
        });
    }

    // Event listeners
    slider.addEventListener('input', (e) => {
        const value = e.target.value;
        percentageInput.value = value;
        updateVisualization(value);
    });

    percentageInput.addEventListener('input', (e) => {
        let value = Math.min(100, Math.max(0, e.target.value));
        slider.value = value;
        updateVisualization(value);
    });

    setServoBtn.addEventListener('click', () => {
        if(currentMode === 'manual') {
            updateServo();
        }
    });

    // Handler de modos
    function setMode(mode) {
        currentMode = mode;
        const isManual = mode === 'manual';
        
        slider.disabled = !isManual;
        percentageInput.disabled = !isManual;
        setServoBtn.disabled = !isManual;
        
        document.querySelector('.servo-control').classList.toggle('auto-mode', !isManual);
        manualBtn.classList.toggle('active', isManual);
        autoBtn.classList.toggle('active', !isManual);
    
        if (!isManual) {
            updateVisualization(0);
            slider.value = 0;
            percentageInput.value = 0;


            // Enviar a la URI correspondiente
            
            fetch("/set_mode_auto", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ mode: 0 })
            })
            .then(response => response.text())
            .then(data => console.log(`Modo cambiado: ${data}`))
            .catch(error => console.error("Error:", error));


            
        }

        
    
        /* // Enviar a la URI correspondiente
        const url = mode === 'manual' ? '/set_mode_manual' : '/set_mode_auto';
        fetch(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ mode: 1 })
        })
        .then(response => response.text())
        .then(data => console.log(`Modo cambiado: ${data}`))
        .catch(error => console.error("Error:", error));  */
    }

    manualBtn.addEventListener('click', () => setMode('manual'));
    autoBtn.addEventListener('click', () => setMode('auto'));

    // Inicialización
    setMode('manual');
    updateVisualization(0);
});

//MARK: REGISTROS

function send_register() {
    // Obtener valores del formulario
    const selectedNumber = document.getElementById('selectNumber').value;
    const hours = document.getElementById('hours').value;
    const minutes = document.getElementById('minutes').value;
    const angle = document.getElementById('angleInput').value;

    // Mapear días de la semana (versión optimizada)
    const days = ['mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'];
    const selectedDays = days.map(day => 
        document.getElementById(`day_${day}`).checked ? "1" : "0"
    );

    // Crear objeto de datos
    const requestData = {
        selectedNumber,
        hours,
        minutes,
        selectedDays,
        angle,
        timestamp: Date.now()
    };

    console.log("Datos enviados:", requestData);

    // Configuración de fetch (corregido)
    fetch("/regchange.json", {
        method: "POST",
        headers: { 
            "Content-Type": "application/json",
            "Cache-Control": "no-cache" // Alternativa válida
        },
        body: JSON.stringify(requestData)
    })
    .then(response => {
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        return response.text();
    })
    .then(data => alert(`Respuesta del servidor: ${data}`))
    .catch(error => {
        console.error("Error en la solicitud:", error);
        alert(`Error: ${error.message}`);
    });
}

/**
 * toogle led function.
 */
function read_reg()
{

	
	$.ajax({
		url: '/readreg.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		//headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		//data: {'timestamp': Date.now()}
	});
//	var xhr = new XMLHttpRequest();
//	xhr.open("POST", "/toogle_led.json");
//	xhr.setRequestHeader("Content-Type", "application/json");
//	xhr.send(JSON.stringify({data: "mi información"}));
}


function erase_register()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();



    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regchange.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });

    // Print the resulting JSON to the console (for testing)
    //console.log(requestDataJSON);
}
    

