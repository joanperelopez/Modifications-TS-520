
#include <Arduino.h>
#include <Wire.h>
//#include <Adafruit_SI5351.h>

#include "si5351.h"

//Adafruit_SI5351 clockgen = Adafruit_SI5351();

Si5351 si5351;

#define LOG_OUT 1 // use the log output function
#define FHT_N 256 // set to 256 point fht
#include <FHT.h> // include the library


#define offsetX  50
#define offsetY  200
#define SS_SLAVE 3        //Chip select PD2
int FFT_Number = 0;
char buffer[128]; 

int32_t cal_factor = 118800;                          //Buscado experimentalmente

void setup() {
unsigned long long freq = 338550000ULL;               //Frecuencia de salida
unsigned long long pll_freq = 60261900000ULL;         //Frecuencia del PLL
//unsigned long long pll_freq = 56017500000ULL;         //Forzado el PLL a bajar de frecuencia para mejorar el ajuste de fase
bool i2c_found;


  // Start serial and initialize the Si5351
  Serial.begin(115200);

  i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if(!i2c_found)
  {
    Serial.println("Device not found on I2C bus!");
  }

  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);           //Origen de cada salida
  si5351.set_ms_source(SI5351_CLK1, SI5351_PLLA);

  si5351.set_freq_manual(freq, pll_freq, SI5351_CLK0);      //Pone las frecuencias de salida y la frecuencia del PLL
  si5351.set_freq_manual(freq, pll_freq, SI5351_CLK1);

  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);   //Factor de correccion de los errores

  si5351.set_phase(SI5351_CLK0, 0);                         // Fases en cuadratura
  si5351.set_phase(SI5351_CLK1, 127);

  si5351.pll_reset(SI5351_PLLA);                            //Resetear para que inicie correctamente las fases

  si5351.update_status();

  TIMSK0 = 0;                     // turn off timer0 for lower jitter
  ADCSRA = 0xe5;                  // set the adc to free running mode
  ADMUX = 0x40;                   // use adc0
  DIDR0 = 0x01;                   // turn off the digital input for adc0

}



void loop() {

while (digitalRead(SS_SLAVE) == HIGH);    // Espera hasta que el maestro active SS
  
  creaFFT();                              //Si. Crea la FFT y la manda al maestro


  
}

//  CREA LA FFT Y MANDA LOS DATOS AL MAESTRO
//--------------------------------------------
void creaFFT() {
//  while(1) {                                // reduces jitter
    cli();                                    // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < FHT_N ; i++) {       // save 256 samples
      while(!(ADCSRA & 0x10));                // wait for adc to be ready
      ADCSRA = 0xf5;                          // restart adc
      byte m = ADCL;                          // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m;                   // form into an int
      k -= 0x0200;                            // form into a signed int
      k <<= 6;                                // form into a 16b signed int
      fht_input[i] = k;                       // put real data into bins
    }
  fht_window();                               // window the data for better frequency response
  fht_reorder();                              // reorder the data before doing the fht
  fht_run();                                  // process the data in the fht
  fht_mag_log();                              // take the output of the fht
  sei();
  //Serial.println("Pasada 1");
  for(int i = 0; i<=127; i++){                //126 bins
      FFT_Number = fht_log_out[i]-15;            //Filtro digital. Creo que regula la amplitud
      if  (FFT_Number <= 0){FFT_Number = 0;}  //Evita números negativos
      buffer[i] = FFT_Number;                 //Llena el buffer para el maestro
    }
  compensar();                                //Iguala los valores del array
  Serial.write(buffer, sizeof(buffer));       // Envía el buffer completo al maestro
  while (digitalRead(SS_SLAVE) == LOW);       // Espera hasta que el maestro desactive SS
  }


void compensar() {
// Compensar el array
  // Compensación proporcional para ajustar valores entre 0 y 60
  for (int i = 0; i <= 127; i++) {
    buffer[i] = map(buffer[i], 0, 127, 0, 110);
    if  (buffer[i] < 0){buffer[i] = 0;}
  }

  // Compensación proporcional para ajustar valores en función de la posición
  for (int i = 0; i <= 127; i++) {
    float factor = 0.07 + 0.5 * (float)i / 100; // Ajusta el factor de 0.4 a 1 linealmente
    buffer[i] = buffer[i] * factor;
  }

}