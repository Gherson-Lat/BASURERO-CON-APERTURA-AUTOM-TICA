# BASURERO-CON-APERTURA-AUTOM-TICA
**PROTOTIPO DE UN BASURERO CON APERTURA AUTOMÁTICA Y MONITOREO DEL NIVEL DE LLENADO**

## Introducción

Este proyecto académico presenta el diseño, implementación y validación de un **prototipo de basurero inteligente** controlado por un microcontrolador **ESP32 DevKit v1 (USB-C, 30 pines)**. El sistema combina la apertura automática de la tapa sin contacto con el monitoreo periódico del nivel interno de residuos, e incorpora una rutina de autodiagnóstico inicial y comandos por puerto serie para facilitar las pruebas y la demostración del prototipo.

### Funcionalidades principales

* **Apertura y cierre automático:** Un sensor infrarrojo de proximidad (**MH-B**, lógica activa en `LOW`) detecta cuando hay un objeto frente al recipiente y acciona un servomotor **MG90S** para abrir la tapa. El temporizador se mantiene activo mientras continúe la detección y cierra la tapa automáticamente tras **4 segundos** (`4000 ms`) sin presencia.
* **Monitoreo de nivel de llenado:** Un sensor ultrasónico (**HY-SRF05**) mide la distancia hacia el interior cada **1 segundo** (`1000 ms`), calcula el porcentaje de llenado (`0 %` a `100 %` según `DIST_VACIO` y `DIST_LLENO`) y muestra la alerta `[!]` en la pantalla cuando el nivel alcanza o supera el **90 %**.
* **Diagnóstico y pruebas por consola:** Ejecuta un auto-test de la pantalla LCD, el servomotor, el sensor ultrasónico y el sensor IR al iniciar, además de aceptar comandos manuales por el Monitor Serie (`1`–`5` y `h`) a **115200 baudios**.

### Resumen de hardware y pines

| Componente | Función | Pines / Conexión |
| :--- | :--- | :--- |
| **ESP32 DevKit v1** | Controlador principal (USB-C, 30 pines) | `LED_INTERNO` en `GPIO 2` |
| **Sensor IR MH-B** | Detección de proximidad (activo en `LOW`) | `OUT` → `GPIO 19`, `VCC` → `VIN (5V)`, `GND` → `GND` |
| **Servomotor MG90S** | Apertura y cierre de la tapa | Señal → `GPIO 13`, `5V` fuente ext., `GND` común |
| **Sensor HY-SRF05** | Medición de distancia / nivel de llenado | `TRIG` → `GPIO 5`, `ECHO` → `GPIO 18` (divisor `1k/2k`), `VCC` → `5V`, `GND` → `GND` |
| **LCD 16x2 I2C** | Visualización de porcentaje y alertas | `SDA` → `GPIO 21`, `SCL` → `GPIO 22` (dir. `0x27` o `0x3F`) |