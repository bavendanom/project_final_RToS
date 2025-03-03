var seconds 	= null;
var otaTimerVar =  null;

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() 
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) 
	{		
        var response = JSON.parse(xhr.responseText);
						
	 	document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time

		// If flashing was complete it will return a 1, else -1
		// A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) 
		{
    		// Set the countdown timer time
            seconds = 10;
            // Start the countdown timer
            otaRebootTimer();
        } 
        else if (response.ota_update_status == -1)
		{
            document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}


function connectWiFi() {
    let ssid = document.getElementById('wifi-ssid').value;
    let pass = document.getElementById('wifi-pass').value;


    if (ssid === "" || pass === "") {
        alert("Please enter both SSID and Password.");
        return;
    }
    
    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        ssid: ssid,
        pass: pass
    };

    fetch('/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => alert(data))
    .catch(error => console.error('Error:', error));
}

// 🔹 Configurar Hotspot
function setHotspot() {
    let ssid = document.getElementById("hotspot-ssid").value;
    let password = document.getElementById("hotspot-pass").value;

    fetch("/set_hotspot", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ ssid, password }),
    })
    .then(response => response.json())
    .then(data => alert(data.message))
    .catch(error => console.error("Error:", error));
}


//MARK: SET LED RGB RED MAX - MIN
function setRed() {
    let min = document.getElementById('red-min').value;
    let max = document.getElementById('red-max').value;

    if (min === "" || max === "") {
        alert("Enter Min and Max values for RED");
        return;
    }

    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        color: 'red',
        min: parseFloat(min),
        max: parseFloat(max)
    };

    // Enviar los datos al servidor mediante una solicitud POST
    fetch("/set_rgb", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => alert(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
}

//MARK: SET LED RGB GREEN MAX - MIN
function setGreen() {
    let min = document.getElementById('green-min').value;
    let max = document.getElementById('green-max').value;

    if (min === "" || max === "") {
        alert("Enter Min and Max values for GREEN");
        return;
    }

    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        color: 'green',
        min: parseFloat(min),
        max: parseFloat(max)
    };

    // Enviar los datos al servidor mediante una solicitud POST
    fetch("/set_rgb", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => alert(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
}

//MARK: SET LED RGB BLUE MAX - MIN
function setBlue() {
    let min = document.getElementById('blue-min').value;
    let max = document.getElementById('blue-max').value;

    if (min === "" || max === "") {
        alert("Enter Min and Max values for BLUE");
        return;
    }

    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        color: 'blue',
        min: parseFloat(min),
        max: parseFloat(max)
    };

    // Enviar los datos al servidor mediante una solicitud POST
    fetch("/set_rgb", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => alert(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
}


//MARK: GET TEMPERATURE last reading
function getTemperature() {
    fetch("/turn_on_temp")
    //.then(response => response.json())
    .then(data => {
        document.getElementById("temperature-display").innerText = `Temperature: ${data.Temperature} °C`;  // Usar la clave correcta del JSON
    })
    .catch(error => console.error("Error:", error))
    .then(data => console.log(`Response: ${data}`));
}

//MARK: TOOGLE LED D2
function Toogle() {
    fetch("/toogle_led", {
        method: "POST",  // Cambiar a POST
        headers: { "Content-Type": "application/json" }
    })
    .then(response => response.text())
    .then(data => console.log(data)) // Muestra una alerta con el mensaje de respuesta
    .catch(error => console.error("Error:", error));
}

//MARK: CHANGE COLOR ADC
function changeColor() {
    fetch("/change_color", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "change" })
    })
    .then(response => response.text())
    .then(data => console.log(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
}


//MARK: PRINT VALUE TEMPERATURE 
let tempInterval;
const maxEntries = 3;

function turnOnTemp() {
    console.log("turnOnTemp called");  // Log para verificar si la función se llama
    tempInterval = setInterval(() => {
        fetch("/turn_on_temp")
            //.then(response => response.json())
            .then(data => {
                console.log("Temperature received:", data.Temperature);  // Log para verificar el valor recibido
                const tempOutput = document.getElementById("temperature-output");

                // Redondear a 1 decimal
                const roundedTemp = parseFloat(data.Temperature).toFixed(1);

                // Crear un nuevo elemento para la nueva temperatura
                const newTemp = document.createElement("div");
                newTemp.innerText = `Temperature: ${roundedTemp} °C`;

                // Añadir la nueva temperatura al final
                tempOutput.appendChild(newTemp);

                // Si hay más de 3 entradas, eliminar la más antigua
                while (tempOutput.childElementCount > maxEntries) {
                    tempOutput.removeChild(tempOutput.firstChild);
                }
            })
            .catch(error => console.error("Error:", error));
    }, 1000);
}

function turnOffTemp() {
    console.log("turnOffTemp called");  // Log para verificar si la función se llama
    clearInterval(tempInterval);
}


//MARK: PRINT VALUE ADC 
let adcInterval;

function turnOnADC() {
    adcInterval = setInterval(() => {
        fetch("/get_adc")
            .then(response => response.json())
            .then(data => {
                console.log("ADC Value received:", data.adc_value);  // Log para verificar el valor recibido
                document.getElementById("adc-value").innerText = `ADC Value: ${data.adc_value}`;
            })
            .catch(error => console.error("Error:", error));
    }, 1000);
}

function turnOffADC() {
    
    console.log("turnOffADC called");  // Log para verificar si la función se llama
    clearInterval(adcInterval);
}

//MARK: RGB CRHOMATIC CIRCLE
function rgb_crhomatic_circle(rgb) {

    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        red: rgb[0],
        green: rgb[1],
        blue: rgb[2]
    };
    fetch("/rgb_crhomatic_circle", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => alert(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
} 


//MARK: DRAW CRHOMATIC CIRCLE
function drawChromaticCircle() {
    const canvas = document.getElementById('chromatic-circle');
    const ctx = canvas.getContext('2d');

    const centerX = (canvas.width / 2) ;
    const centerY = (canvas.height / 2);
    const radius = Math.min(centerX, centerY);

    console.log("Coordenadas del radio:", radius);
    console.log("Coordenadas del centro (x, y):", centerX, centerY); // Depuración de coordenadas

    // Dibujar el círculo cromático
    for (let angle = 0; angle < 360; angle += 1) {
        const startAngle = (angle -2 ) * Math.PI / 180;
        const endAngle = angle * Math.PI / 180;

        // Usar el ángulo para calcular el color HSL
        const hue = angle;
        const saturation = 100;
        const lightness = 50;

        // Convertir HSL a un color CSS
        const color = `hsl(${hue}, ${saturation}%, ${lightness}%)`;

        // Dibujar un segmento del círculo
        ctx.beginPath();
        ctx.moveTo(centerX, centerY);
        ctx.arc(centerX, centerY, radius-49, startAngle, endAngle);
        ctx.closePath();

        // Rellenar el segmento con el color correspondiente
        ctx.fillStyle = color;
        ctx.fill();
    }

    // Evento para obtener el color al hacer clic
    canvas.addEventListener("click", (event) => {
        const rect = canvas.getBoundingClientRect();
        const x = event.clientX - rect.left-75;
        const y = event.clientY - rect.top-75;
        
        console.log("Coordenadas del clic (x, y):", x, y); // Depuración de coordenadas

    
        const dx = x - centerX;
        const dy = y - centerY;
        const distance = Math.sqrt(dx * dx + dy * dy);

        console.log("Distancia desde el centro (distance):", distance); // Depuración de distancia
        // Verificar si el clic está dentro del círculo
        if (distance <= radius) {
            // Calcular el ángulo correctamente (en grados)
            let angle = Math.atan2(dy, dx) * (180/ Math.PI);
            
            // Asegurar que el ángulo sea positivo en el rango [0, 360]
            if (angle < 0) {
                angle += 360;
            }

            console.log("Ángulo calculado (angle):", angle); // Depuración del ángulo
    
            // Convertir HSL a RGB
            const h = angle / 360; // Asegurar que h esté en el rango [0, 1]
            const rgb = hslToRgb(angle / 360, 0.9, 0.6);

            console.log("Valores HSL (h, s, l):", h, 1, 0.5); // Depuración de HSL
            console.log("Valores RGB (r, g, b):", rgb); // Depuración de RGB
    
            // Mostrar los valores RGB en los inputs
            document.getElementById("red-value").value = rgb[0];
            document.getElementById("green-value").value = rgb[1];
            document.getElementById("blue-value").value = rgb[2];
            rgb_crhomatic_circle(rgb)
        } else {
            // Si el clic está fuera del círculo, limpiar los valores RGB
            document.getElementById("red-value").value = "";
            document.getElementById("green-value").value = "";
            document.getElementById("blue-value").value = "";
            console.log("Clic fuera del círculo"); // Depuración adicional
        }
    });
    
}

// Función para convertir HSL a RGB
function hslToRgb(h, s, l) {
    let r, g, b;

    if (s === 0) {
        r = g = b = l; // Escala de grises
    } else {
        const hue2rgb = (p, q, t) => {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1 / 6) return p + (q - p) * 6 * t;
            if (t < 1 / 2) return q;
            if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
            return p;
        };

        const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        const p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }

    return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

// Llamar a la función para dibujar el círculo cromático
drawChromaticCircle();

//MARK: SLIDER
function slider_cromatic_circle(currentValue) {
    const numericValue = Number(currentValue);

    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        value: numericValue,
    };
    fetch("/slider_crhomatic_circle", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => console.log(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
} 

// Obtener el slider y el elemento para mostrar el valor
const brightnessSlider = document.getElementById('brightness-slider');
const sliderValue = document.getElementById('slider-value');
// Actualizar el valor cuando el slider cambie
brightnessSlider.addEventListener("input", function () {
    const currentValue = this.value; // Obtener el valor actual del slider
    sliderValue.textContent = currentValue; // Actualizar el texto en la interfaz
    slider_cromatic_circle(currentValue); // Enviar el valor al servidor
});

//MARK: TIME ON OFF
function setTime(){
    let min = document.getElementById('time_on').value;
    //let max = document.getElementById('time_off').value;

    if (min === "") {
        alert("Enter Min and Max values for RED");
        return;
    }

    // Crear el cuerpo de la solicitud con los datos configurados
    const data = {
        min: parseFloat(min),
        //max: parseFloat(max)
    };

    // Enviar los datos al servidor mediante una solicitud POST
    fetch("/set_time", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data)
    })
    .then(response => response.text())
    .then(data => alert(`Response: ${data}`))
    .catch(error => console.error("Error:", error));
}



