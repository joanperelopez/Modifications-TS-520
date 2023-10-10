/***************************************************
This program was written by Pere Lopez EA3AGK in order
to create an electronic module to update de Kenwood
TS-520 transceiver. This program control the ILI9341 TFT
and represent the frequency, band, mode, s-meter and the spectrum 
of the IF signal.
****************************************************/

//#include "SPI.h"
#include <Arduino.h>
#include <ILI9341_Fast.h>
#include "Adafruit_GFX.h"
#include <FreqCount.h>
#include <ColorsPere.h>
#include <Play_Regular8pt7b.h>        //(7 x 11 pixels)



#define CODE_VER              "v1.20"
#define BUILD_TYPE            "dev"              // 'dev' = development, 'beta' = bata 'PROD' = production
#define BUILD_DATE            "4-9-2023"          
#define SPLASH_SCREEN_TIME    (3000)             //Splash screen hold time in milliseconds


boolean DEBUG = false;      
boolean DEBUG1 = false;      

//   VARIABLES DEL FFT Y DEL MICRO ESCLAVO
//------------------------------------------

#define   SS_SLAVE      7            //Chip select del micro esclavo. Conectado por SPI. PD7
int   START  = 0xA5;          //Clave que le indica al esclavo mandar el buffer del FFT
//Escribe los valores de la linea de frecuencia
#define offsetX  55
#define offsetY  200                  // Linea de frecuencias
//Escribe las barritas de amplitud del FFT
#define setX    10                    //Se convierte en offset Y al estar girado el display para imprimir en vertical
#define setY    35                    //Se convierte en offset X al estar girado el display para imprimir en vertical

char buffer[128];                      //Este es el buffer que llena el esclavo y que se puede meter directamente en el display
char FFT_Number = 0;
unsigned long last_FFT = 0;


//       VARIABLES DEL S-METER
//------------------------------------------
uint16_t  iTick_color;              //Color de cada componente
int       iTick_pos;                //Posición de los ticks
#define   S_UNITS     A3            //Entrada analógica de lectura del s-meter A3
int test_a = 0;


//       VARIABLES DEL DISPLAY
//-------------------------------———----
#define TFT_CS    10                  //Pines de control
#define TFT_DC    9
#define TFT_RST   8                   //Not used

char cifraNew[9];                     //Donde se almacenan las cifras de la frecuencia
char cifraOld[9];                     //Cifras de la frecuencia anterior
unsigned long lastFrame = millis();

ILI9341 tft = ILI9341(TFT_DC, TFT_RST, TFT_CS);  


//       VARIABLES DEL CONTADOR DE FRECUENCIA
//-------------------------------------------------—
// Averaging variables for smoothing display jitter
const int numReadings = 4;                // number of readings to average over
unsigned long readings[numReadings];      // the readings from the frequency county
//int readIndex = 0;                        // the index of the current reading
//unsigned long total = 0;                  // the running total
//unsigned long average = 0;                // the average
unsigned int  band = 80;                  // Band
// timer varables
//volatile unsigned long timerCounts;
//volatile boolean counterReady;

char mode = 'U';                          // current mode computed from the BFO frequency 
char OldMode = 'S';                       // used in test to see if mode has changed

// counting routine variables
unsigned long overflowCount;
unsigned int timerTicks;
unsigned int timerPeriod;

//Frequency measurement variables
unsigned long frq = 0;                    // From counter
unsigned long vfo = 0;                    // holds measured vfo frequency
unsigned long bfo = 0;                    // holds measured bfo frequency
unsigned long hfo = 0;                    // holds measured hfo frequency (which has been divided by 8 in hardware to get frequency into range that the Arduino can measure)
unsigned long freq = 0;                   // calculated frequency
unsigned long old_freq = 0;               // Para ver si ha cambiado la frecuencia
unsigned long lastbfo = 0;                // holds last bfo frequency to see if it has changed
signed long freqchange = 0;               // holds the hfo/bfo frequency delta from the last measurement
unsigned long fix_hfo = 0;












/////////////////////////////////////////
//    S E T U P
/////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(300); // Establece un tiempo de espera para recibir datos
  tft.begin();
  tft.setRotation(1);                   //Rota la pantalla
  tft.fillScreen(BLACK);                //Borra la pantalla
  splashScreen();
  writeFreqFixed();                     // Hace el layout de la pantalla: Caracteres fijos de la frecuencia
  freqCounter ();                       // llama al contador de frecuencia
  creaCifrasNew();                      // La frecuencia que será escrita en el display lo metemos en array en formato caracter
  writeFrequency();                     // Escribe la frecuencia
//Inicio del contador de frecuencia
  FreqCount.begin(100);
 //initilize averaging array
for (int thisReading = 0; thisReading < numReadings; thisReading++) { readings[thisReading] = 0;}
  pinMode(S_UNITS, INPUT);              //Analog input for S-Meter
  pinMode(4, OUTPUT);                   //setup pins used to select vfo/bfo/hfo using 74LS153
  pinMode(6, OUTPUT);                   //setup pins used to select vfo/bfo/hfo using 74LS153
//  Micro esclavo
//  SPI.begin();
//  pinMode(MISO, INPUT);
  pinMode(SS_SLAVE, OUTPUT);
  digitalWrite(SS_SLAVE, HIGH);         // Desactiva el esclavo al principio

  last_FFT = millis();
  lastFrame = millis();

  VFO_ON ();
  draw_bar_meter_s();                   //Dibuja el contenedor del s-meter
  init_SpectrumFFT();                   //Dibuja el contenedor del FFT

}




/////////////////////////////////////////////////
//           PROGRAMA PRINCIPAL
/////////////////////////////////////////////////
void loop(void) {

  if ((millis()-lastFrame)>70) {                  //Espera 50 milisegundos
  lastFrame = millis();  
  freqCounter ();                                 //Llama al contador de frecuencia 
  poneModo();                                     //Pone el MODO
  poneBanda();                                    //Pone la banda
  update_s_meter();                               //Actualiza el smeter
  if (old_freq != freq){   
      creaCifrasNew();                                //Crea el array de cifras
      writeFrequency();                           //Nueva frecuencia
        if (freq < 130000) borraPrimerDigito();     // Es 3,5 o 7 MHz. Borra primer dígito 
      } 
  }

if ((millis()-last_FFT)>50) {                  //Espera 20 milisegundos
  last_FFT = millis();
  digitalWrite(SS_SLAVE, LOW);                   //Llama al esclavo
  poneFFTen_Pantalla();
  digitalWrite(SS_SLAVE, HIGH);   
  delay(2); 
  }           
}


///////////////////////////////////////////////////////
//.     RUTINAS FFT
///////////////////////////////////////////////////////


//.   PONE LAS GRAFICAS DE LA FFT EN PANTALLA
void poneFFTen_Pantalla() { 

while (Serial.available() > 0) {
    char c = Serial.read();                       // Leer y descartar cada byte del buffer
    }

int separacion = 2;     
      while (Serial.available() == 0);            //Mira si le ha llegado el buffer desde el esclavo
      Serial.readBytes(buffer, sizeof(buffer));
      
      for(int i = 2; i<=127; i++){                //126 bins

      tft.drawLine(offsetX+separacion*i, offsetY, offsetX+separacion*i, offsetY-75, BLACK); //Se borra el anterior valor
      tft.drawLine(offsetX+separacion*i, offsetY, offsetX+separacion*i, offsetY-buffer[i], WHITE);  //Se muestra el nuevo valor
      }

  }
  




//.  PONE EL CONTENEDOR Y EL EJE DE FRECUENCIAS DE LA FFT
//-------------------------------------------------------------
void init_SpectrumFFT(){            //Pone el contenedor del FFT
    tft.setRotation(0);             //Para que imprima las frecuencias en vertical
    tft.setTextSize(1);             // Definimos tamaño del texto. (Probado tamaños del 1 al 10) 
    tft.setTextColor(WHITE);        // Definimos el color del texto  

    float o_freq = -10.5;           //Para imprimr las frecuencias relativas del FFT

    for (int i=1; i<=15; i++) {
      if ((i>=2&&i<=7)||(i==15)) tft.setCursor( setX-6 , (18*i) + setY );   //Esta y las sigueintes dos lineas compensan la altura de los carteles
      else if  (i==1)            tft.setCursor( setX-12 , (18*i) + setY );  
      else                       tft.setCursor( setX , (18*i) + setY );         
      tft.println((o_freq));        //Imprime las frecuencias
      o_freq +=1.5;
    }
  tft.setRotation(1);               //Vuelve a la rotacion normal

  }





///////////////////////////////////////////////////////
//    BLOQUE DE RUTINAS DEL S-METER
///////////////////////////////////////////////////////

//Define colors for bar meter elements using RGB565 format
//see: http://www.barth-dev.de/online/rgb565-color-picker/
const uint16_t BARMETER_GRAY = 0x8430;    //Gray color in RGB565 format.
const uint16_t BARMETER_MGRAY = RGBto565(50,50,50);    //Medium Gray color in RGB565 format.
const uint16_t BARMETER_BLUE = 0x02D9;    //Blue color
const uint16_t BARMETER_RED  = 0xD8E5;    //Tercera parte de la barra
const uint16_t BARMETER_YELLOW = 0xFF80;  //Segunda parte de la barra
const uint16_t BARMETER_DARK  = 0x2124;   //bar pointer when off
const uint16_t BARMETER_GREEN  = 0x07E0;  //Primera parte de la barra

// Dibujo del contenedor y las marcas del s-meter
#define INI_SX      70              //Posición X donde empieza el s-meter
#define INI_SM      INI_SX-3        // Inicio de las marcas
#define INI_SY      100             // Posición Y
#define S_WG         177            // anchura Linea gris superior
#define S_WR         83             // anchura Linea roja superior
#define S_WGI        235            // anchura Linea gris inferior
#define S_END       INI_SX+230      // Cierre final del contenedor
#define S_H         16              // Altura dwel contenedor
#define S_GRUESO     2             // Grososr del contenedor
#define S_DIST       17             // Distancia entre marcas
// Dibujo de las barritas dentro del s-meter
#define INI_XBAR    INI_SX      // Donde se emplieza a dibujar las barras X 20
#define INI_YBAR    INI_SY+4        // 72
      // Donde se emplieza a dibujar las barras Y
#define S_ALT       S_H-6           // Altura de las barra
#define S_COL1      34              //Dónde se empiezan a poner barritas amarillas
#define S_COL2      62              //Dónde se empiezan a poner barritas rojas

#define   FROM_LOW    350             //Límite bajo de lectura del ADC
#define   FROM_HIGH   640           //Limite de lectura alto del ADC
#define   TO_LOW      95            //Mínimo del mapeado. Antes 3
#define   TO_HIGH     2           //Máximo del mapeado. Antes 100

// Crea la parte fija del s-meter
//--------------------------------
void draw_bar_meter_s(void){

// Dibuja las líneas del s-meter
  tft.drawRect(INI_SX, INI_SY, S_WG,S_GRUESO, BARMETER_GRAY);    //Linea superior izquierda
  tft.drawRect(150+INI_SX, INI_SY, S_WR,S_GRUESO, BARMETER_RED);  //Linea superior derecha roja
  tft.fillRect(INI_SX, INI_SY, S_GRUESO+1,S_H, BARMETER_GRAY);        //Línea vertical izquierda
  tft.fillRect(S_END, INI_SY, S_GRUESO+1,S_H, BARMETER_RED);        //Línea vertical derecha
  tft.drawRect(INI_SX, INI_SY+S_H, S_WG,S_GRUESO, BARMETER_GRAY);    //Linea inferior izquierda
  tft.drawRect(150+INI_SX, INI_SY+S_H, S_WR,S_GRUESO, BARMETER_RED);  //Linea inferior derecha roja 

  //Set text font and size
  tft.setFont(&Play_Regular8pt7b); //font
  tft.setTextSize(1);

  //Add "S" indicator 
  tft.setCursor(INI_SX-15 ,INI_SY+12); //(x,y)
  tft.setTextColor(BARMETER_GRAY);
  tft.println(F("S"));
 
  for (int i=1; i<14; i++) {
    if (i>9) iTick_color=BARMETER_RED; else iTick_color=BARMETER_GRAY;
    tft.fillRect(INI_SM+(S_DIST*i), INI_SY-5, 3,7, iTick_color);        // Pone las marcas
    if (i%2)  {

      if (i<=9) {
        tft.setCursor(INI_SM+(S_DIST*i)-4,INI_SY-10);                       //Dibuja números del 1 al 9      
        tft.print(i);
        } // Final números

      if (i==11) {
        tft.setTextColor(BARMETER_RED);
        tft.setCursor(INI_SM+(S_DIST*i)-14,INI_SY-10);                       //Dibuja +20dB     
        tft.print("+20");
        }

        if (i==13) {
        tft.setTextColor(BARMETER_RED);
        tft.setCursor(INI_SM+(S_DIST*i)-14,INI_SY-10);                       //Dibuja +40db      
        tft.print("+40");
        }
    } // (i%2)
  }  // for...
tft.setFont(); // Standard font
}


//-------------------------------------------------------------------------------
void update_s_meter() {
  int iAnalog_value=0;                      //Temp variable to hold analog value from analogRead() so analog-in pin only needs to be scanned once
  int val;

  for (int i=0; i<10; i++) {
    val = analogRead(S_UNITS);
    iAnalog_value += val;
    }
  iAnalog_value /= 10;

 //Read analog input and convert to TO_LOW to TO_HIGH
  iAnalog_value = map(iAnalog_value, FROM_LOW, FROM_HIGH, TO_LOW, TO_HIGH);    //Mapea la lectura del ADC para darle el rango correcto 
  if (iAnalog_value<2) iAnalog_value = 2;
  deflect_bar_meter(INI_XBAR,INI_YBAR,iAnalog_value,S_COL1,S_COL2);           //Llama a actualizar las barritas del s-meter

/*
// For testing pourposes
  test_a = (analogRead(S_UNITS));
  Serial.print ("ADC mapeado: ");
  Serial.print (iAnalog_value);
  Serial.print ("   ADC:  ");
  Serial.print (test_a);
  Serial.print (" iAnalog_value:   "); 
  Serial.println (iAnalog_value);
*/
} //<end> function update_s_meter()



//         CREA LAS BARRITAS DEL S-METER CON SUS DISTINTOS COLORES
//-------------------------------------------------------------------------------
// iXpos, and iYpos is meter position.
// iValue is level 0-100 of deflection.
// iCaution is level at which bar turns yellow.
// iWarning is level at which bar turns red.
// if level is greater than 100, entire bar turns red
void deflect_bar_meter(int iXpos, int iYpos, int iValue, int iCaution, int iWarning) {

  uint16_t iBarSeg_color = BARMETER_DARK;

  //Calcula las barritas que hay que pintar en función de lo que ha leido desde el ADC
  int iBarSeg_y = INI_YBAR;                       //Absolute Y screen position for all bar segments
  int iBarSeg_value = INI_XBAR+(iValue/2*5);      //Absolute X screen position for value segment
  int iBarSeg_warning = INI_XBAR+(iWarning/2*5);  //Absolute X screen position for warning segment
  int iBarSeg_caution = INI_XBAR+(iCaution/2*5);  //Absolute X screen position for caution segment
  int iBarSeg_zero = INI_XBAR;                    //Absolute position of bar segment zero value
  int iBarSeg_pos = INI_XBAR+220;                 //Absolute current bar-meter segment position, set to full scale initially +220

// Pinta las barritas de los diferentes segmentos del s-meter
  for( ; iBarSeg_pos>iBarSeg_zero; iBarSeg_pos-=5) {               //Start at the full scale position and work back to zero

    if(iBarSeg_pos <= iBarSeg_value){

      if(iBarSeg_pos >= iBarSeg_warning){iBarSeg_color = BARMETER_RED;}           //Tercera parte del s-meter
      else if(iBarSeg_pos >= iBarSeg_caution){iBarSeg_color = BARMETER_YELLOW;}   //Segunda parte del s-meter
      else iBarSeg_color = BARMETER_GREEN;                                        //Primera parte del s-meter

    }
    else iBarSeg_color = BARMETER_MGRAY;                                          //Fondo del s-meter
    tft.fillRect(iBarSeg_pos, iBarSeg_y, 3,S_ALT, iBarSeg_color);

  }

}








///////////////////////////////////////////////////////
//    BLOQUE DE RUTINAS DEL DISPLAY TFT ILI9341
///////////////////////////////////////////////////////
//
//  FUNCIONES:
//  ILI9341.fillScreen(ILI9341_GREEN);              //Rellena toda la pantalla
//  ILI9341.setTextColor(ILI9341_YELLOW);           //Cambiar color de texto
//  ILI9341.setTextSize(2);                         //Cambiar tamaño del texto
//  ILI9341.println("my foonting turlingdromes.");  //Poner un texto en pantalla
//  ILI9341.setCursor(0, 0);                        //Situar el cursor
//  ILI9341.fillRect(X, Y, W, H, ILI9341_BLACK);    //Rellenar en negro una parte del display
//  ILI9341.setRotation(1);                         //Rota la pantalla
//  ILI9341.fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
//  ILI9341.drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
//  ILI9341.drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
//  drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
//  drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
//  fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
//  fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
//  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
//  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
//  void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
//   void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
//  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
//  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg);
//  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
//   void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
//  void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], const uint8_t mask[], int16_t w, int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h);
//  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
//  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
//  void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
//  void getTextBounds(const __FlashStringHelper *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
//  void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
//  void setTextSize(uint8_t s);
//  void setTextSize(uint8_t sx, uint8_t sy);
//  void setFont(const GFXfont *f = NULL);
//  virtual void invertDisplay(bool i);
//  virtual void endWrite(void);
//  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
//   virtual void startWrite(void);
//   virtual void endWrite(void);

/*
// Color definitions
#define P_BLACK 0x0000                          ///<   0,   0,   0
#define P_NAVY 0x000F                           ///<   0,   0, 123
#define P_DARKGREEN 0x03E0                      ///<   0, 125,   0
#define P_DARKCYAN 0x03EF                       ///<   0, 125, 123
#define P_MAROON 0x7800                         ///< 123,   0,   0
#define P_PURPLE 0x780F                         ///< 123,   0, 123
#define P_OLIVE 0x7BE0                          ///< 123, 125,   0
#define P_LIGHTGREY 0xC618                      ///< 198, 195, 198
#define P_DARKGREY 0x7BEF                       ///< 123, 125, 123
#define p_BLUE 0x001F                           ///<   0,   0, 255
#define p_GREEN 0x07E0                          ///<   0, 255,   0
#define p_CYAN 0x07FF                           ///<   0, 255, 255
#define p_RED 0xF800                            ///< 255,   0,   0
#define p_MAGENTA 0xF81F                        ///< 255,   0, 255
#define p_YELLOW 0xFFE0                         ///< 255, 255,   0
#define P_WHITE 0xFFFF                          ///< 255, 255, 255
#define P_ORANGE 0xFD20                         ///< 255, 165,   0
#define P_GREENYELLOW 0xAFE5                    ///< 173, 255,  41
#define P_PINK 0xFC18                           ///< 255, 130, 198
#define P_GREY  RGBto565(128,128,128)
#define P_LGREY RGBto565(160,160,160)
#define P_DGREY RGBto565( 80, 80, 80)
#define P_LBLUE RGBto565(100,100,255)
#define P_DBLUE RGBto565(  0,  0,128)
*/

//
//
//  unsigned long start = micros();                 //Cálculo de tioempo
//  return micros() - start;                        //Cálculo de tiempo

// Display de la frecuencia
#define INI_FX      50    //Posición X donde empieza la frecuencia
#define INI_FY      25    //Posición Y
#define W_F         235   //Anchura
#define H_F         30    //Altura
#define anchoChar_F 25    //anchura de cada caracter de la frecuencia
#define MHZ_TEXT    180   //Lugar donde se escribe "Mhz"
#define COLOR_F     WHITE
#define COLOR_MHZ   P_YELLOW
#define COLOR_FBG   P_DBLUE

// Marco de la frecuencia
#define INI_MX      0           //Posición X donde empieza el marco de la frecuencia
#define INI_MY      10          //Posición Y
#define W_M         320         //Anchura
#define H_M         60          //Altura
#define COLOR_MAR   P_DBLUE

// Linea bajo la frecuencia
#define INI_LX      45          //Posición X
#define INI_LY      70          //Posición Y
#define W_L         198         //Anchura
#define COLOR_LIN   P_WHITE     //Altura

// Modo de operación
#define INI_MODX      0             //Posición X
#define INI_MODY      61            //Posición Y
#define W_MOD         40            //Anchura de la banda
#define H_MOD         23            //Altura de la banda
#define COLOR_MOD     P_BLACK       //Color texto no seleccionado
#define COLOR_MODBG   P_DGREY       //Background no seleccionado
#define DESP_USB      27            //Lugar donde está el indicador LSB antes 0
#define DESP_LSB      54            //Lugar donde está el indicador LSB antes 27
#define DESP_CW       0             //Lugar donde está el indicador CW antes 54
#define HIGH_BG       P_DARKGREEN   //Background resaltado
#define HIGH_TEXT     P_WHITE       //Texto resaltado

// Leds de VFO, RIT y FIX
#define INI_LEDX      0             //Posición X
#define INI_LEDY      INI_MODY+90   //Posición Y
#define W_LED         W_MOD         //Anchura de la banda
#define H_LED         H_MOD         //Altura de la banda
#define COLOR_LED     COLOR_MOD     //Color texto no seleccionado
#define COLOR_LEDBG   COLOR_MODBG   //Background no seleccionado
#define DESP_FIX      27            //Lugar donde está el indicador LSB antes 0
#define DESP_RIT      54            //Lugar donde está el indicador LSB antes 27
#define DESP_VFO       0            //Lugar donde está el indicador CW antes 54
#define HIGH_LEDBG    P_RED         //Background resaltado
#define HIGH_LED      P_WHITE       //Texto resaltado


// BANDA
#define INI_BX      250           //Posición X
#define INI_BY      46            //Posición Y
#define W_B         65            //Anchura
#define H_B         25            //Altura
#define COLOR_B     P_WHITE       //Color texto
#define COLOR_BBG   P_DGREY


//      SPLASH SCREEN
//----------------------------------------———
void splashScreen(void) {
tft.setCursor(30, 20);    //X,Y
tft.setTextColor(WHITE); tft.setTextSize(3);
tft.println("Kenwood TS-520");   

tft.setCursor(30, 60);    //X,Y
tft.setTextColor(GREEN); tft.setTextSize(2);
tft.println("Modifications by EA3AGK");   

tft.setFont(&Play_Regular8pt7b); 
tft.setCursor(30, 110);    //X,Y
tft.setTextColor(YELLOW); tft.setTextSize(1);
tft.print("Code version: ");   
tft.println(CODE_VER);   

tft.setCursor(30, 130);    //X,Y
tft.print("Build type: ");   
tft.println(BUILD_TYPE);   

tft.setCursor(30, 150);    //X,Y
tft.print("Build date: ");   
tft.println(BUILD_DATE);   

tft.setFont(); 
delay(SPLASH_SCREEN_TIME);
tft.fillScreen(BLACK);  
}


///Hace el layout de la pantalla: nombres, lineas, franjas y elemento que no cambian
//----------------------------------------------------------------------------------
void writeFreqFixed() {
  tft.setCursor(10, 0);    //X,Y
  tft.setTextColor(WHITE); tft.setTextSize(1);
  tft.println("Kenwood TS-520 EA3AGK Software version 1.0");          //Name & call sign

// Marcos y lineas
  tft.fillRect(INI_MX, INI_MY, W_M, H_M, COLOR_MAR);                  // Marco de la frecuencia              
  tft.drawFastHLine(INI_LX, INI_LY, W_L, COLOR_LIN);                  // Linea bajo la frecuencia
  tft.drawFastHLine(INI_LX, INI_LY-1, W_L, COLOR_LIN);                // Linea bajo la frecuencia

// Imprime MHz  
  tft.setCursor(INI_FX+MHZ_TEXT, INI_FY); 
  tft.setFont(&Play_Regular8pt7b);    
  tft.setTextColor(COLOR_MHZ); tft.setTextSize(1);              
  tft.println("MHz"); 
  tft.setFont();  

// Rctangulo del MODO en gris
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_CW, W_MOD, H_MOD, 3, COLOR_MODBG);          // reactangulo de modo CW
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_LSB, W_MOD, H_MOD, 3, COLOR_MODBG);         // Rectangulo de modo LSB
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_USB, W_MOD, H_MOD, 3, COLOR_MODBG);         // Rectangulo de modo USB

// Rctangulo de los LEDs en gris
  tft.fillRoundRect(INI_LEDX, INI_LEDY+DESP_RIT, W_LED, H_LED, 3, COLOR_LEDBG);          // reactangulo de modo CW
  tft.fillRoundRect(INI_LEDX, INI_LEDY+DESP_FIX, W_LED, H_LED, 3, COLOR_LEDBG);         // Rectangulo de modo LSB
  tft.fillRoundRect(INI_LEDX, INI_LEDY+DESP_VFO, W_LED, H_LED, 3, COLOR_LEDBG);         // Rectangulo de modo USB

// Imprime el MODO en pantalla
  tft.setTextColor(COLOR_MOD,  COLOR_MODBG); tft.setTextSize(2);
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_USB); 
  tft.println("USB");           //Modo USB
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_LSB ); 
  tft.println("LSB");           //Modo LSB
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_CW ); 
  tft.println("CW ");           //Modo CW

// Imprime los LEDs en pantalla
  tft.setTextColor(COLOR_LED,  COLOR_LEDBG); tft.setTextSize(2);
  tft.setCursor(INI_LEDX+2 , INI_LEDY+5+DESP_RIT); 
  tft.println("RIT");           //LED RIT
  tft.setCursor(INI_LEDX+2 , INI_LEDY+5+DESP_FIX ); 
  tft.println("FIX");           //LED FIX
  tft.setCursor(INI_LEDX+2 , INI_LEDY+5+DESP_VFO ); 
  tft.println("VFO");           //LED VFO

// Rectangulo de la banda
  tft.fillRoundRect(INI_BX, INI_BY, W_B, H_B, 3, COLOR_BBG);                  // Rectangulo de banda
  tft.drawRoundRect(INI_BX, INI_BY, W_B, H_B, 3, COLOR_B);

  }


//Escribe todos los dígitos de la frecuencia en el display
//----------------------------------------------------------
void writeFrequency()
{
//  tft.setFont(&fixednums8x16);    
tft.setTextColor(COLOR_F,  COLOR_FBG); tft.setTextSize(4);          //Color de la frecuencia
//tft.setTextColor(COLOR_F,  COLOR_FBG); tft.setTextSize(1);          //Color de la frecuencia
for (int i=0; i<=6; i++){
  if (i==0) {
        tft.setCursor(INI_FX, INI_FY); 
        if (cifraNew[0]!=('0')) tft.println(cifraNew[0]);           //Primera cifra. No pone el primer cero
        }
  else  {
        if (i==6) tft.setTextSize(3);
        tft.setCursor(INI_FX+(anchoChar_F*i), INI_FY);              //Pone el resto de cifras
        if (i==6) tft.setCursor(INI_FX+(anchoChar_F*i)+3, INI_FY+8);
        tft.println(cifraNew[i]);  
        }
  }
tft.setFont();  
old_freq = freq;
}


//Pone un espacio en blanco como primer caracter cuando estamos en 3,5 y 7 MHz 
void borraPrimerDigito() {
  tft.setCursor(INI_FX, INI_FY); 
  tft.setTextColor(COLOR_F,  COLOR_FBG); tft.setTextSize(4);  
  tft.println(" "); 
  }

//Activa el LED de VFO
void VFO_ON () {
  tft.setTextColor(HIGH_LED,  HIGH_LEDBG); tft.setTextSize(2);
  tft.setCursor(INI_LEDX , INI_LEDY+DESP_VFO); 
  tft.fillRoundRect(INI_LEDX, INI_LEDY+DESP_VFO, W_LED, H_LED, 3, HIGH_LEDBG);  
  tft.setCursor(INI_LEDX+2 , INI_LEDY+5+DESP_VFO);  
  tft.println("VFO");           //LED VFO

}

void USB_ON () {
  tft.setTextColor(HIGH_TEXT,  HIGH_BG); tft.setTextSize(2);
  tft.setCursor(INI_MODX , INI_MODY+DESP_USB); 
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_USB, W_MOD, H_MOD, 3, HIGH_BG);  
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_USB);  
  tft.println("USB");           //Modo USB

}

void USB_OFF () {
  tft.setTextColor(COLOR_MOD ,  COLOR_MODBG); tft.setTextSize(2);
  tft.setCursor(INI_MODX , INI_MODY+DESP_USB); 
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_USB, W_MOD, H_MOD, 3, COLOR_MODBG);  
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_USB);  
  tft.println("USB");           //Modo USB

}

void LSB_OFF () {
  tft.setTextColor  (COLOR_MOD ,  COLOR_MODBG); tft.setTextSize(2);
  tft.setCursor(INI_MODX , INI_MODY+DESP_LSB); 
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_LSB, W_MOD, H_MOD, 3, COLOR_MODBG);
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_LSB);    
  tft.println("LSB");           //Modo USB

}

void LSB_ON () {
   tft.setTextColor(HIGH_TEXT,  HIGH_BG); tft.setTextSize(2);
  tft.setCursor(INI_MODX , INI_MODY+DESP_LSB); 
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_LSB, W_MOD, H_MOD, 3, HIGH_BG);
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_LSB);    
  tft.println("LSB");           //Modo USB

}

void CW_ON () {
   tft.setTextColor(HIGH_TEXT,  HIGH_BG); tft.setTextSize(2);
  tft.setCursor(INI_MODX , INI_MODY+DESP_CW); 
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_CW, W_MOD, H_MOD, 3, HIGH_BG);
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_CW);    
  tft.println("CW ");           //Modo USB

}

void CW_OFF () {
  tft.setTextColor  (COLOR_MOD ,  COLOR_MODBG); tft.setTextSize(2);
  tft.setCursor(INI_MODX , INI_MODY+DESP_CW); 
  tft.fillRoundRect(INI_MODX, INI_MODY+DESP_CW, W_MOD, H_MOD, 3, COLOR_MODBG);
  tft.setCursor(INI_MODX+2 , INI_MODY+5+DESP_CW);    
  tft.println("CW ");           //Modo USB

}



// Indica si estamos en USB, LSB o CW
void poneModo(){
if (mode != OldMode) {
  switch (mode) {
        case 'U':
        USB_ON();
        LSB_OFF();
        CW_OFF();
        break;
        case 'L':
        LSB_ON();
        USB_OFF();
        CW_OFF();
        break;
        case 'C':
        CW_ON();
        USB_OFF();
        LSB_OFF();
        break;
        }
  }
OldMode = mode;
}

//Pone en el display la banda de frecuencias
void poneBanda() {
    tft.setCursor(INI_BX+7 , INI_BY+5); 
    tft.setTextColor(COLOR_B,  COLOR_BBG); tft.setTextSize(2);

    switch (band) {

    case  100:
    tft.println(" WWV"); 
    break;

    case  80:
    tft.println("80 m"); 
    break;
    
    case  40:
    tft.println("40 m"); 
    break;

    case  20:
    tft.println("20 m"); 
    break;

    case  15:
    tft.println("15 m"); 
    break;

    case  10:
    tft.println("10 m"); 
    break;


    }
}


//Convierte la frecuencia a cifras dentro del array
void creaCifrasNew(){
    cifraNew[0]=((freq/100000)%10)+48;
    cifraNew[1]=((freq/10000)%10)+48;
    cifraNew[2]='.';
    cifraNew[3]=((freq/1000)%10)+48;
    cifraNew[4]=((freq/100)%10)+48;
    cifraNew[5]=((freq/10)%10)+48;
    cifraNew[6]=((freq)%10)+48;
   

    if (DEBUG1 == true){  
        for (int i=0; i<=8; i++) Serial.print(cifraNew[i]);
        Serial.println(" ");
        }
    }   


///////////////////////////////////////////////////////
//    BLOQUE DE RUTINAS DEL CONTADOR DE FRECUENCIA
///////////////////////////////////////////////////////


//Bloque del contador de frecuencia
void freqCounter (){

for (int x=0; x < 3; x++){                     // Loop thru the 3 signals to count the signals.
         if (x==0) {                            //Select VFO 
             digitalWrite(4, LOW);
             digitalWrite(6, LOW);
         }
          if (x==1) {                           //Select BFO
             digitalWrite(4, HIGH);
             digitalWrite(6, LOW);              
         }
         
         if (x==2) {                            //Select HFO
             digitalWrite(4, LOW);
             digitalWrite(6, HIGH);
         }         
         
delay (1);  //settle time

FreqCount.begin(100);

while (FreqCount.available() == 0); 
      frq = (FreqCount.read());
      FreqCount.end();
      frq = frq*10;
   
//Correction Factor, if needed.  This is added to each of the 3 frequencies read.

          if (x==0) vfo=frq+(-1950);                     
          if (x==1) bfo=frq+(-1280);
          if (x==2) hfo=(frq*8)+(-7919);

} //for (int x=0; x < 3; x++)  Final lectura de las tres frecuencias


// Identify the HFO and put the exact value read with an external counter
//-----------------------------------------------------------------------
fix_hfo = hfo/100000;
switch (fix_hfo) {
    case  188:
    hfo = 18895000;       //Banda de 3,5Mhz
    band = 100;
    break;

    case  123:
    hfo = 12395000;       //Banda de 3,5Mhz
    band = 80;
    break;

    case  158:
    hfo = 15895000;       //Banda de 7 MHZ
    band = 40;
    break;

    case  228:
    hfo = 22895000;       //Banda de 14 MHz
    band = 20;
    break;

    case  298:
    hfo = 29895000;       //Banda de 21 MHz
    band = 15;
    break;

    case  368:
    case  369:
    hfo = 36895000;       //Banda de 28 MHz
    band = 10;
    break;

    case  374:
    hfo = 37395000;       //Banda de 28,5 MHz
    band = 10;
    break;

    case  380:
    hfo = 37995000;       //Banda de 29,1 MHZ
    band = 10;
    break;

    }

//  BFO test
freqchange = lastbfo - bfo;           //see how much the frequency has change since the last measurement
freqchange = abs(freqchange);

           if (freqchange < 50){     //if the frequency hasen't changed more than 100 Hz, then leave it as is
             bfo = lastbfo;
           }
           else {lastbfo = bfo;}      //larger change detected, use this value.


//Compute the actual dial frequency, round and drop least signifigant digits 
freq = ((((hfo-(bfo+vfo))/10)+5)/10);

// use the BFO (CAR) frequency to determine USB/LSB/CW
//////////////////////////////////////////////////////////

// In the TS-520S Service manual on pg. 6, it describes the frequencies used by the BFO for the different modes.
// It states, “Frequencies are 3396.5 kHz for USB, 3393.5 kHz for LSB, and 3394.3 kHz (receive) and 3395.0 kHz (transmit) for CW.”
// You might have to tweak these values for your BFO - this is a function that the origional DG5 didn't do.
// If your Arduino's crystal is off, you might be able to fix both using the correction factor (cf) variable in the "Frequency measurement variables" section

if (bfo > 3396000){
  mode = 'U';
  }  
else if (bfo < 3393600){
  mode='L';
  }
  else{
  mode='C';
  }

//#ifdef DEBUG                                  
if (DEBUG == true){                             
Serial.print("VFO: ");                         
Serial.print (vfo); 
Serial.print (" BFO: ");                            
Serial.print (bfo);
Serial.print (" HFO: ");
Serial.print (hfo);
Serial.print (" FREQ: ");
Serial.print (freq);
Serial.print (" Modo ");
Serial.println (mode);
}


}  

