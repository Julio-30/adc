// Importação das bibliotecas necessárias
#include "hardware/pio.h"  
#include "hardware/i2c.h" 
#include "pico/stdlib.h" 
#include "ssd1306.h"   
#include <stdlib.h> 
#include <stdio.h>
#include "font.h"  
#include "hardware/adc.h" 
#include "hardware/pwm.h" 

// Definição dos pinos utilizados 
#define I2C_INTERFACE i2c1    // Interface I2C
#define SDA_PIN 14      
#define SCL_PIN 15     
#define DISPLAY_ADDR 0x3C   // Endereço do display SSD1306

#define JOYSTICK_X 26  // Pino do eixo X
#define JOYSTICK_Y 27  // Pino do eixo Y
#define JOYSTICK_BUTTON 22 // Botão do Joystick
#define BUTTON_A 5      
#define LED_BLUE 12              // LED RGB Azul
#define LED_GREEN 11            // LED RGB Verde
#define LED_RED 13        // LED RGB Vermelho

volatile uint32_t ultimo_tempo = 0; 
bool estado_led_verde = false, estado_led_rgb = true, cor_display = true, quadrado = true;
ssd1306_t display;  
uint16_t valor_x, valor_y, brilho_r, brilho_b;
int posicao_centro = 2047, x_display, y_display;    
uint slice_vermelho, slice_azul;

// Configuração do PWM
void configurar_pwm(uint pino, uint *slice, uint16_t brilho) {
    gpio_set_function(pino, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(pino);
    pwm_set_clkdiv(*slice, 16); 
    pwm_set_wrap(*slice, 4095);
    pwm_set_gpio_level(pino, brilho);
    pwm_set_enabled(*slice, true);
}

// Inicialização do LED RGB
void configurar_led_rgb() {
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);

    configurar_pwm(LED_RED, &slice_vermelho, brilho_r);
    configurar_pwm(LED_BLUE, &slice_azul, brilho_b);
}

// Configuração do display OLED
void configurar_display() {
    i2c_init(I2C_INTERFACE, 400000); 
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);  
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);   
    gpio_pull_up(SCL_PIN); 

    ssd1306_init(&display, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_INTERFACE);
    ssd1306_config(&display);
    ssd1306_fill(&display, false);

    ssd1306_rect(&display, 1, 1, 122, 58, cor_display, !cor_display);
    ssd1306_send_data(&display);
}

// Alternar estado dos LEDs RGB
void alternar_led_rgb(bool *estado) {
    *estado = !(*estado);
    pwm_set_enabled(slice_vermelho, *estado);
    pwm_set_enabled(slice_azul, *estado);
}

// Alternar estado do LED verde
void alternar_led_verde(bool *estado) {
    *estado = !(*estado);
    gpio_put(LED_GREEN, *estado);
}

// Interrupção dos botões
void tratar_interrupcao_botao(uint pino, uint32_t eventos) {
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    if (tempo_atual - ultimo_tempo > 200000) {
        printf("LED Verde = %d\n", estado_led_verde);
        ultimo_tempo = tempo_atual;
        if (pino == BUTTON_A) {
            printf("LEDs RGB = %d\n", estado_led_rgb);
            alternar_led_rgb(&estado_led_rgb);
        }
        else if (pino == JOYSTICK_BUTTON) {
            alternar_led_verde(&estado_led_verde);
            cor_display = !cor_display;
        }
    }
}

// Configuração inicial dos botões e joystick
void configurar_hardware() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN); 
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, tratar_interrupcao_botao);
    
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y); 

    gpio_init(JOYSTICK_BUTTON);
    gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true, tratar_interrupcao_botao); 
}

// Atualização do display
void atualizar_display() {
    ssd1306_fill(&display, !cor_display);
    ssd1306_rect(&display, 2, 2, 125, 61, cor_display, !cor_display);
    
    x_display = (valor_x == posicao_centro) ? 60 : (valor_x < posicao_centro) ? 120 - valor_x / 32 : valor_x / 32 - 124;
    y_display = (valor_y == posicao_centro) ? 28 : (valor_y < posicao_centro) ? 54 - valor_y / 64 : valor_y / 64 - 60;
    
    ssd1306_rect(&display, y_display, x_display, 8, 8, quadrado, !quadrado);
    ssd1306_send_data(&display);
}

// Leitura dos valores do joystick
void ler_joystick(uint16_t *eixo_x, uint16_t *eixo_y) {
    adc_select_input(0);
    sleep_us(20);
    *eixo_x = adc_read();
    adc_select_input(1);
    sleep_us(20);
    *eixo_y = adc_read();
}

int main() {
    stdio_init_all();
    configurar_hardware();   
    configurar_led_rgb();   
    configurar_display();      

    while (true) {
        ler_joystick(&valor_x, &valor_y);
        printf("x = %d, y = %d\n", x_display, y_display); 
        atualizar_display();

        pwm_set_gpio_level(LED_RED, (valor_x == posicao_centro) ? 0 : (valor_x < posicao_centro) ? 4096 - valor_x : valor_x);
        pwm_set_gpio_level(LED_BLUE, (valor_y == posicao_centro) ? 0 : (valor_y < posicao_centro) ? 4096 - valor_y : valor_y);

        sleep_ms(50);
    }
    return 0;
}
