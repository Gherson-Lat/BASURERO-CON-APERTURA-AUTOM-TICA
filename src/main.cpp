/*
 * PROYECTO ACADÉMICO: BOTE DE BASURA INTELIGENTE
 * Placa: ESP32 DevKit v1 (USB-C - 30 pines)
 * Sensor IR: Módulo MH-B (Lógica activa en BAJO: LOW = Detectado, HIGH = Libre)
 * Sensor Ultrasónico: HY-SRF05 (5 pines)
 * 
 * --- CONFIGURA TU MONITOR SERIE EN ARDUINO IDE A 115200 BAUDIOS ---
 * 
 * Conexiones físicas:
 * - Sensor IR (MH-B): VCC -> VIN (5V), GND -> GND, OUT -> GPIO 19 (D19)
 * - Servomotor MG90S: Señal (Naranja) -> GPIO 13 (D13), Rojo -> +5V Fuente Ext., Marrón -> GND común
 * - Sensor HY-SRF05: VCC -> +5V, TRIG -> GPIO 5 (D5), ECHO -> GPIO 18 (D18, divisor 1k/2k), OUT -> libre (NC), GND -> GND
 * - Pantalla LCD 16x2 I2C: VCC -> 3.3V (o 5V), GND -> GND, SDA -> GPIO 21 (D21), SCL -> GPIO 22 (D22)
 */


#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// --- PINES DE CONEXIÓN ---
const int PIN_IR = 19;        // Sensor IR de Proximidad (GPIO 19)
const int PIN_SERVO = 13;     // Servomotor MG90S (GPIO 13)
const int PIN_TRIG = 5;       // HY-SRF05 Trigger (GPIO 5)
const int PIN_ECHO = 18;      // HY-SRF05 Echo (GPIO 18)
const int LED_INTERNO = 2;    // LED azul integrado en la placa ESP32 (D2)

// --- LÓGICA DEL SENSOR IR ---
// LOW = Detecta cuando recibe 0V (comportamiento actual de tu sensor)
const int IR_DETECTADO = LOW; 

// --- CONFIGURACIÓN DEL SERVOMOTOR ---
Servo servoTapa;
const int ANGULO_CERRADO = 10;           // Ángulo tapa cerrada
const int ANGULO_ABIERTO = 210;         // Ángulo tapa abierta (ajustable)
const int TIEMPO_ESPERA_ABIERTO = 4000; // Tiempo abierta tras retirar la mano (4s)

// --- CONFIGURACIÓN DEL SENSOR HY-SRF05 (MEDIDAS DEL BOTE) ---
const float DIST_VACIO = 18.0;   // Distancia al fondo en cm (bote vacío)
const float DIST_LLENO = 4.0;    // Distancia mínima en cm (bote lleno)

// --- PANTALLA LCD 16x2 I2C ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección I2C típica: 0x27 (o 0x3F)

// --- VARIABLES DE ESTADO ---
bool tapaAbierta = false;
unsigned long tiempoApertura = 0;
unsigned long ultimoMuestreoNivel = 0;
const unsigned long INTERVALO_NIVEL = 1000; // Medir nivel cada 1 segundo

bool ultimoEstadoDeteccion = false; // Guarda si en el ciclo anterior había objeto
int ultimoPorcentaje = -1;

// =========================================================================
//                   DECLARACIÓN DE FUNCIONES AUXILIARES
// =========================================================================

// Función auxiliar para leer el IR de forma clara (true = hay objeto, false = libre)
bool hayObjetoCerca() {
  return (digitalRead(PIN_IR) == IR_DETECTADO);
}

// Medición de distancia con HY-SRF05
float medirDistanciaHYSRF05() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 25000);
  if (duracion == 0) return DIST_VACIO;

  return (duracion * 0.0343) / 2.0;
}

// Calcular % de llenado
int calcularPorcentajeLlenado(float distancia) {
  if (distancia > DIST_VACIO) distancia = DIST_VACIO;
  if (distancia < DIST_LLENO) distancia = DIST_LLENO;

  float porcentajeFloat = ((DIST_VACIO - distancia) / (DIST_VACIO - DIST_LLENO)) * 100.0;
  int porcentaje = round(porcentajeFloat);
  return constrain(porcentaje, 0, 100);
}

// Control del Servo
void abrirTapa() {
  Serial.println("[ACCION]: Moviendo servomotor a ABIERTO");
  servoTapa.write(ANGULO_ABIERTO);
  tapaAbierta = true;
}

void cerrarTapa() {
  Serial.println("[ACCION]: Moviendo servomotor a CERRADO");
  servoTapa.write(ANGULO_CERRADO);
  tapaAbierta = false;
}

// Actualización de pantalla LCD
void actualizarPantalla(int porcentaje) {
  lcd.setCursor(0, 0);
  lcd.print("BOTE INTELIGENTE");
  lcd.setCursor(0, 1);
  lcd.print("Llenado: ");
  lcd.print(porcentaje);
  lcd.print("%     ");
  
  if (porcentaje >= 90) {
    lcd.setCursor(13, 1);
    lcd.print("[!]");
  } else {
    lcd.setCursor(13, 1);
    lcd.print("   ");
  }
  Serial.print("[LCD]: Actualizado -> 'Llenado: ");
  Serial.print(porcentaje);
  Serial.println("%'");
}

// Auto-diagnóstico inicial
void ejecutarAutodiagnostico() {
  Serial.println("\n--- INICIANDO AUTO-TEST DE COMPONENTES ---");
  
  // Test 1: LCD
  Serial.println("[PRUEBA 1/4] Pantalla LCD 16x2 I2C...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEST LCD 16x2");
  lcd.setCursor(0, 1);
  lcd.print("CONEXION I2C OK!");
  delay(1200);
  
  // Test 2: Servomotor
  Serial.println("[PRUEBA 2/4] Servomotor MG90S...");
  lcd.clear();
  lcd.print("TEST: SERVO");
  lcd.setCursor(0, 1);
  lcd.print("Abriendo...");
  servoTapa.write(ANGULO_ABIERTO);
  Serial.println("  -> Movimiento: Abierto");
  delay(1200);
  
  lcd.setCursor(0, 1);
  lcd.print("Cerrando...  ");
  servoTapa.write(ANGULO_CERRADO);
  Serial.println("  -> Movimiento: Cerrado");
  delay(1200);

  // Test 3: HY-SRF05
  Serial.println("[PRUEBA 3/4] Sensor Ultrasónico HY-SRF05...");
  float distTest = medirDistanciaHYSRF05();
  Serial.print("  -> Distancia detectada: ");
  Serial.print(distTest, 1);
  Serial.println(" cm");
  lcd.clear();
  lcd.print("TEST: HY-SRF05");
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distTest, 1);
  lcd.print(" cm");
  delay(1200);

  // Test 4: Sensor IR
  Serial.println("[PRUEBA 4/4] Sensor Infrarrojo de Proximidad...");
  bool detectadoTest = hayObjetoCerca();
  Serial.print("  -> Estado actual del pin 19: ");
  Serial.println(detectadoTest ? "DETECTADO (Objeto presente)" : "LIBRE (Sin objeto)");
  lcd.clear();
  lcd.print("TEST: SENSOR IR");
  lcd.setCursor(0, 1);
  lcd.print(detectadoTest ? "DETECTA OBJETO" : "SIN OBSTACULO");
  delay(1200);

  lcd.clear();
  lcd.print("BOTE INTELIGENTE");
  lcd.setCursor(0, 1);
  lcd.print("SISTEMA ACTIVO");
  Serial.println("--- FIN DEL AUTO-TEST: TODOS LOS COMPONENTES PROBADOS --- \n");
  delay(1000);
  lcd.clear();
}

// Menú de ayuda por puerto serie
void mostrarMenuAyuda() {
  Serial.println("=================================================================");
  Serial.println(" COMANDOS MANUALES (Escribe el número y presiona ENTER):");
  Serial.println(" [1] -> Forzar Apertura de Tapa");
  Serial.println(" [2] -> Forzar Cierre de Tapa");
  Serial.println(" [3] -> Leer estado instantáneo del Sensor IR (Pin 19)");
  Serial.println(" [4] -> Medir distancia actual con Sensor HY-SRF05");
  Serial.println(" [5] -> Enviar mensaje de prueba manual al LCD");
  Serial.println(" [h] -> Mostrar este menú de ayuda");
  Serial.println("=================================================================\n");
}

// Procesar comandos manuales
void procesarComandoManual(char c) {
  if (c == '\n' || c == '\r') return;

  Serial.print("\n[COMANDO MANUAL]: ");
  Serial.println(c);

  switch (c) {
    case '1':
      Serial.println("Ejecutando: Forzar Apertura.");
      abrirTapa();
      break;
    case '2':
      Serial.println("Ejecutando: Forzar Cierre.");
      cerrarTapa();
      break;
    case '3': {
      bool detectado = hayObjetoCerca();
      Serial.print("Estado del Sensor IR (GPIO 19): ");
      Serial.println(detectado ? "¡DETECTADO! (Objeto cerca)" : "LIBRE (Sin objeto)");
      break;
    }
    case '4': {
      float d = medirDistanciaHYSRF05();
      int p = calcularPorcentajeLlenado(d);
      Serial.print("Lectura HY-SRF05: ");
      Serial.print(d, 1);
      Serial.print(" cm (Llenado: ");
      Serial.print(p);
      Serial.println("%)");
      break;
    }
    case '5':
      Serial.println("Enviando mensaje de prueba al LCD...");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PRUEBA MANUAL");
      lcd.setCursor(0, 1);
      lcd.print("ESP32 USB-C OK");
      delay(2000);
      ultimoPorcentaje = -1;
      break;
    case 'h':
    case 'H':
      mostrarMenuAyuda();
      break;
    default:
      Serial.println("Comando no reconocido. Presiona 'h' para ver ayuda.");
      break;
  }
}

// =========================================================================
//                          SETUP Y LOOP PRINCIPALES
// =========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=================================================================");
  Serial.println("  SISTEMA DE DIAGNÓSTICO EN VIVO - BOTE DE BASURA INTELIGENTE   ");
  Serial.println("  Controlador: ESP32 DevKit v1 USB-C | Sensor: HY-SRF05         ");
  Serial.println("=================================================================");
  Serial.println("[ESP32] Inicializando pines...");

  pinMode(PIN_IR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(LED_INTERNO, OUTPUT);

  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(LED_INTERNO, LOW);

  ESP32PWM::allocateTimer(0);
  servoTapa.setPeriodHertz(50);
  servoTapa.attach(PIN_SERVO, 500, 2400);
  servoTapa.write(ANGULO_CERRADO);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  ejecutarAutodiagnostico();
  mostrarMenuAyuda();
}

void loop() {
  unsigned long tiempoActual = millis();

  // 1. Monitoreo del Sensor IR (Proximidad)
  bool objetoPresente = hayObjetoCerca();

  if (objetoPresente != ultimoEstadoDeteccion) {
    ultimoEstadoDeteccion = objetoPresente;
    if (objetoPresente) {
      Serial.println("\n>>> [EVENTO SENSOR IR]: ¡DETECTADO! Objeto frente al basurero <<<");
      digitalWrite(LED_INTERNO, HIGH);
      if (!tapaAbierta) abrirTapa();
      tiempoApertura = tiempoActual;
    } else {
      Serial.println("\n>>> [EVENTO SENSOR IR]: LIBRE. Se retiró el objeto.");
      digitalWrite(LED_INTERNO, LOW);
    }
  }

  // Mantiene renovado el temporizador mientras la persona siga frente al sensor
  if (objetoPresente) {
    tiempoApertura = tiempoActual;
  }

  // Cierra la tapa tras 4 segundos sin detección
  if (tapaAbierta && !objetoPresente && (tiempoActual - tiempoApertura >= TIEMPO_ESPERA_ABIERTO)) {
    cerrarTapa();
  }

  // 2. Monitoreo del Sensor HY-SRF05 (Nivel de Llenado)
  if (tiempoActual - ultimoMuestreoNivel >= INTERVALO_NIVEL) {
    ultimoMuestreoNivel = tiempoActual;
    
    float distancia = medirDistanciaHYSRF05();
    int porcentaje = calcularPorcentajeLlenado(distancia);
    
    Serial.print("[REGISTRO HY-SRF05]: Distancia = ");
    Serial.print(distancia, 1);
    Serial.print(" cm | Llenado = ");
    Serial.print(porcentaje);
    Serial.println("%");

    if (porcentaje != ultimoPorcentaje) {
      ultimoPorcentaje = porcentaje;
      actualizarPantalla(porcentaje);
    }
  }

  // 3. Comandos manuales por teclado
  if (Serial.available() > 0) {
    char comando = Serial.read();
    procesarComandoManual(comando);
  }
}