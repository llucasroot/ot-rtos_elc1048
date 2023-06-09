#include <atmel_start.h>
#include <stdio.h>
#include "Test_Io1_Plained/bno055.h"

#define VCC_TARGET 3.3 // Tens�o VCC da placa de R21 usada como refer�ncia

volatile bool conversion_done = false;
struct io_descriptor* USART_debug_io;
struct bno055_dev bno055; // Corrigido para usar o tipo 'bno055_dev'

/**
 * Faz a convers�o anal�gico digital do sensor de luz e avisa quando terminar
 */
static void convert_cb_Light_sensor_ADC(const struct adc_async_descriptor *const descr, const uint8_t channel)
{
    conversion_done = true;
}

/**
 * Inicializa o conversor AD para o sensor de luz
 */
void Light_sensor_init(void)
{
    adc_async_register_callback(&Light_sensor_ADC, 0, ADC_ASYNC_CONVERT_CB, convert_cb_Light_sensor_ADC);
    adc_async_enable_channel(&Light_sensor_ADC, 0);
    adc_async_start_conversion(&Light_sensor_ADC);
}

/**
 * Inicializa a UART usada como debug do programa
 * Pode ser usada para printf
 */
void USART_dbg_init(void){
    usart_sync_get_io_descriptor(&USART_debug, &USART_debug_io);
}

/**
 * Escreve 1 byte na UART de debug
 */
void UART_write_byte(uint8_t byte_to_send){
    io_write(USART_debug_io, &byte_to_send, 1);
}

/**
 * L� o valor digital do sensor de luz ap�s passar pelo ADC e calcula e entrega a ilumin�ncia com base nas caracter�sticas el�tricas do sensor
 */
float GET_light_sensor(void){
    uint8_t lightSensorValue;
    float voltageSensor;
    float iluminance;

    /* Faz a convers�o AD do sensor de luz*/
    adc_async_start_conversion(&Light_sensor_ADC);
    while(!conversion_done){}
    adc_async_read_channel(&Light_sensor_ADC, 0, &lightSensorValue, 1);

    /* Faz a defini��o dos valores de tens�o lidos do sensor a partir dos dados quantizados do ADC*/
    voltageSensor = lightSensorValue * VCC_TARGET / 255;

    /* Calcula a ilumin�ncia com base na corrente do resistor de coletor do fototransistor e na rela��o entre lux e corrente*/
    iluminance = (VCC_TARGET - voltageSensor) * 200;

    return iluminance;
}

/**
 * Liga o LED da placa de expans�o IO1X
 */
void SET_IO1X_LED_ON(void){
    gpio_set_pin_level(LED, true);
}

/**
 * Desliga o LED da placa de expans�o IO1X
 */
void SET_IO1X_LED_OFF(void){
    gpio_set_pin_level(LED, false);
}

/**
 * Converte um n�mero float em string com a precis�o informada
 * � usada para poder imprimir um valor float na tela usando o sprintf
 * O compilador usado n�o aceita float no sprintf
 */
void floatToString(float num, char* str, int precision) {
    int i = 0;

    // Extract the integral part
    int integralPart = (int)num;

    // Convert the integral part to string
    do {
        str[i++] = integralPart % 10 + '0';
        integralPart /= 10;
    } while (integralPart > 0);

    // Reverse the integral part string
    int j;
    int len = i;
    for (j = 0; j < len / 2; j++) {
        char temp = str[j];
        str[j] = str[len - j - 1];
        str[len - j - 1] = temp;
    }

    // Add decimal point
    str[i++] = '.';

    // Extract the fractional part
    float fractionalPart = num - (int)num;

    // Convert the fractional part to string
    int k;
    for (k = 0; k < precision; k++) {
        fractionalPart *= 10;
        int digit = (int)fractionalPart;
        str[i++] = digit + '0';
        fractionalPart -= digit;
    }

    // Add null-terminating character
    str[i] = '\0';
}

float acceleration_x; // Acelera��o no eixo X
float acceleration_y; // Acelera��o no eixo Y
float acceleration_z; // Acelera��o no eixo Z

float temperature; // Temperatura lida do sensor

int main(void)
{
    /* Initializes MCU, drivers and middleware */
    atmel_start_init();
    Light_sensor_init();
    USART_dbg_init();

    char message[50];
    char acceleration_str[10];
    char temperature_str[10];

    /* Inicializa o aceler�metro */
    bno055_init(&bno055); // Corrigido para usar 'bno055_init' em vez de 'struct bno055_init'

    while (1) {
        /* L� os dados de acelera��o do aceler�metro */
        bno055_read_acceleration(&bno055, &acceleration_x, &acceleration_y, &acceleration_z); // Corrigido para usar 'bno055_read_acceleration' em vez de 'struct bno055_read_acceleration'

        /* L� a temperatura do sensor */
        temperature = bno055_get_temperature(&bno055); // Corrigido para usar 'bno055_get_temperature' em vez de 'struct bno055_get_temperature'

        /* Formata as acelera��es como strings */
        floatToString(acceleration_x, acceleration_str, 4);
        floatToString(temperature, temperature_str, 2);

        /* Constr�i a mensagem a ser enviada pela UART */
        sprintf(message, "Aceleracao X: %s\r\nTemperatura: %s\r\n", acceleration_str, temperature_str);
        printf(message);

        /* Liga e desliga o LED da placa de expans�o */
        delay_ms(20);
        SET_IO1X_LED_OFF();
        delay_ms(20);
    }
}
