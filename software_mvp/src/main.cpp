#include <Arduino.h>
#include <Task01.h>
#include <Task02.h> 

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando tasks...");

    // Inicia as tasks. Cada task rodará em um core diferente.
    startTask01(); // cuidado, esse nucleo também cuida das tasks do wifi e bluetooth
    startTask02();
}

void loop() {
    Serial.println("Encerrando o void loop...");
    vTaskDelete(NULL); 
}