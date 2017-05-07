/*
 * ~= Doof sphere lights =~
 *
 * Code for the SH-055 board from Scott LED: https://scottled.en.alibaba.com/
 *
 * The great thing about this board is that it has a WS2811 chip on both sides of the board. This means that each sphere is
 * actually 2 pixels! Having 2 pixels per sphere allows for some great effects...Unfortunately they seemed to have stopped
 * manufacturing it so you will need to contact them to see if it can be ordered.
 *
 */
#include <FastLED.h>
#include <EnableInterrupt.h>
#include <Pushbutton.h>

#define VERSION "3"

// Pin definitions
#define BUTTON_RED_LED_PIN 13
#define BUTTON_RED_PRESS_PIN 12
#define BUTTON_GREEN_LED_PIN 11
#define BUTTON_GREEN_PRESS_PIN 10
#define BUTTON_BLUE_LED_PIN 9
#define BUTTON_BLUE_PRESS_PIN 8
#define DATA_PIN 6

#define NUM_LEDS 200
#define NUM_SPHERES (NUM_LEDS/2)

CRGB leds[NUM_LEDS];
CHSV spheres[NUM_SPHERES];


Pushbutton red_led_button(BUTTON_RED_PRESS_PIN);
Pushbutton green_led_button(BUTTON_GREEN_PRESS_PIN);
Pushbutton blue_led_button(BUTTON_BLUE_PRESS_PIN);

volatile boolean button_process = false;
volatile unsigned long button_event_time = 0;

// Currently using red button for feature enable
boolean feature_enabled = true;

#define LOOP_FREQUENCY_HZ 50
// #define DEBUG 1


// Our colour themes
enum colour_themes { Electric, Warm, Rainbow };
byte colour_theme;

const byte electric_hues[] = { HUE_BLUE, HUE_AQUA, HUE_PURPLE, HUE_PURPLE, HUE_BLUE, HUE_AQUA, HUE_PURPLE, HUE_PINK };
const byte warm_hues[] = { HUE_RED, HUE_ORANGE, HUE_ORANGE };


// All of our pretty patterns!
struct {
  void (*setup)(byte);
  void (*loop)(void);
  byte theme;
  byte data;
} patterns[] = {
  { star_setup, star_loop, Rainbow, 0 },
  { star_setup, star_loop, Electric, 0 },
  { star_setup, star_loop, Warm, 0 },
  { comet_setup, comet_loop, Electric, 1 },
  { comet_setup, comet_loop, Electric, 2 },
  { comet_setup, comet_loop, Electric, 3 },
  { single_hue_setup, single_hue_loop, 0, HUE_RED },
  { single_hue_setup, single_hue_loop, 0, HUE_ORANGE },
  { single_hue_setup, single_hue_loop, 0, HUE_YELLOW },
  { single_hue_setup, single_hue_loop, 0, HUE_GREEN },
  { single_hue_setup, single_hue_loop, 0, HUE_AQUA },
  { single_hue_setup, single_hue_loop, 0, HUE_BLUE },
  { single_hue_setup, single_hue_loop, 0, HUE_PURPLE },
  { single_hue_setup, single_hue_loop, 0, HUE_PINK },
  { NULL, NULL, 0, 0 } // End of list
};
byte pattern_index = 0;
byte prev_pattern_index = 255;





void setup() {
  Serial.begin(9600);
  Serial.println("Doof Light Spheres version#" VERSION " setup()...");
  
  pinMode(BUTTON_RED_LED_PIN, OUTPUT);
  pinMode(BUTTON_GREEN_LED_PIN, OUTPUT);
  pinMode(BUTTON_BLUE_LED_PIN, OUTPUT);

  digitalWrite(BUTTON_RED_LED_PIN, feature_enabled ? HIGH : LOW);
  digitalWrite(BUTTON_GREEN_LED_PIN, HIGH); // Mode change always lit up
  digitalWrite(BUTTON_BLUE_LED_PIN, HIGH); // Not used at present

  enableInterrupt(BUTTON_RED_PRESS_PIN, button_change, CHANGE);
  enableInterrupt(BUTTON_GREEN_PRESS_PIN, button_change, CHANGE);
  enableInterrupt(BUTTON_BLUE_PRESS_PIN, button_change, CHANGE);
  
  FastLED.addLeds<WS2812, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setCorrection( CRGB(255, 140, 240) );

  random16_set_seed(analogRead(0));
  randomSeed(analogRead(0));
  
  Serial.println("Setup complete!");
}


#ifdef DEBUG
unsigned long loop_printout_time = 0;
unsigned long loop_count = 0;
#endif

void loop() {
  
  unsigned long loop_start_time = millis();
  
  if (pattern_index != prev_pattern_index) {
    colour_theme = patterns[pattern_index].theme;
    patterns[pattern_index].setup(patterns[pattern_index].data);
    FastLED.show();
    prev_pattern_index = pattern_index;
  }
  patterns[pattern_index].loop();
  FastLED.show();
  
  if (button_process) {
    
#ifdef DEBUG
    Serial.println("Button processing..");
#endif
    
    if (red_led_button.getSingleDebouncedPress()) {
      button_event_time = millis();
      feature_enabled = !feature_enabled;
    }
    if (red_led_button.getSingleDebouncedRelease()) {
      button_event_time = millis();
    }
    digitalWrite(BUTTON_RED_LED_PIN, feature_enabled ? HIGH : LOW);

    if (green_led_button.getSingleDebouncedPress()) {
      button_event_time = millis();
      digitalWrite(BUTTON_GREEN_LED_PIN, LOW);
      pattern_index++;
      if (patterns[pattern_index].setup == NULL)
        pattern_index = 0;
    }
    if (green_led_button.getSingleDebouncedRelease()) {
      button_event_time = millis();
      digitalWrite(BUTTON_GREEN_LED_PIN, HIGH);
    }

    /*
    if (blue_led_button.getSingleDebouncedPress()) {
      button_event_time = millis();
      digitalWrite(BUTTON_BLUE_LED_PIN, HIGH);
    }
    if (blue_led_button.getSingleDebouncedRelease()) {
      button_event_time = millis();
      digitalWrite(BUTTON_BLUE_LED_PIN, LOW);
    }
    */
    
    if (millis() - button_event_time > 1000)
      button_process = false;
#ifdef DEBUG
      Serial.println("END Button processing.");
#endif
  }

#ifdef DEBUG
  loop_count++;
  unsigned long ms_since_last_printout = millis() - loop_printout_time;
  if (ms_since_last_printout > 1000) {
    Serial.print("Loop count: ");
    Serial.print(loop_count*1000/ms_since_last_printout);
    Serial.println(" loops/second.");
    loop_count = 0;
    loop_printout_time = millis();
  }
#endif
  
  unsigned loop_duration = millis() - loop_start_time;
  if (loop_duration < (1000/LOOP_FREQUENCY_HZ))
    delay((1000/LOOP_FREQUENCY_HZ) - loop_duration);
}

void button_change(void) {
  button_process = true;
  button_event_time = millis();
}




// Common hue management code

byte colour_theme_hue_random(void) {
  
  byte hue;
  
  switch (colour_theme) {
    
    case Electric:
      hue = electric_hues[random8(sizeof(electric_hues))];
      break;
      
    case Warm:
      hue = warm_hues[random8(sizeof(warm_hues))];
      break;
      
    case Rainbow:
      hue = random8();
      break;
      
    default:
      hue = HUE_PURPLE;
  }
 
 return hue;
} 



// All spheres a single hue
void single_hue_setup(byte hue) {
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
}

void single_hue_loop() {
}







// Comet code!
#define COMETS_NUMBER 3
#define COMET_FADE_SCALE 9    // 0-255
#define COMET_BRIGHTNESS 175
#define COMET_HUE_OFFSET 10  // Spheres always at least this far apart
#define COMET_HUE_WOBBLE 20  // How much further the hue diverges
#define COMET_EMBER_COLOUR CRGB::OrangeRed

const byte comet_cycles_per_move[] = { 9, 7, 5 }; // Comet speed. Coprime numbers so that comets never stay in sync

struct {
  byte hue;             // Colour of this comet
  byte head_index;      // Head of the comet
  byte cycle_counter;   // When counter reaches zero, we move the head
  int8_t dir;           // Direction (1 = forward, -1 = reverse)
  byte collision_glow;  // How much to glow after a collision
} comets[COMETS_NUMBER];
byte comet_number_active;

void comet_setup(byte number) {
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  comet_number_active = number;

  // Initialise all comets
  for (int i = 0; i < comet_number_active; i++) {
    comets[i].hue = colour_theme_hue_random();
    comets[i].head_index = i * NUM_SPHERES/comet_number_active; // Space evenly to start
    comets[i].cycle_counter = 1;
    comets[i].dir = 1;  // All start in the same direction and then chaos reins..
    comets[i].collision_glow = 0;
    
    leds[2*comets[i].head_index] = CHSV(comets[i].hue, 255, COMET_BRIGHTNESS);
    leds[2*comets[i].head_index + 1] = CHSV(comets[i].hue, 255, COMET_BRIGHTNESS);
    spheres[comets[i].head_index] = CHSV(comets[i].hue, 0, COMET_BRIGHTNESS);
  }  
}

void comet_loop() {
  static byte cycle = 0;
  
  // Make the tails do a colour wobble fade
  for (int i = 0; i < NUM_SPHERES; i++) {
    if (leds[2*i]) {      
      spheres[i].value = scale8(spheres[i].value, 255 - COMET_FADE_SCALE);
      if (spheres[i].value) {
        
        // Ember from any collision
        CRGB ember = COMET_EMBER_COLOUR;
        ember %= spheres[i].value; // dimm the ember of a collision
        
        // Mix in the ember glow with the comet colour
        leds[2*i] = blend(CHSV(spheres[i].hue + COMET_HUE_OFFSET/2 + scale8(cycle, COMET_HUE_WOBBLE), 255, spheres[i].value), ember, spheres[i].saturation);
        leds[2*i + 1] = blend(CHSV(spheres[i].hue - COMET_HUE_OFFSET/2 - scale8(cycle, COMET_HUE_WOBBLE), 255, spheres[i].value), ember, spheres[i].saturation);
        
      } else {
        leds[2*i] = CRGB::Black;
        leds[2*i + 1] = CRGB::Black;
      }
    }
  }

  // Move the comets
  for (int i = 0; i < comet_number_active; i++) {
    
    comets[i].cycle_counter--;    
    if (comets[i].cycle_counter == 0) { // Move the comet
      comets[i].cycle_counter = comet_cycles_per_move[i];
     
      comets[i].head_index += comets[i].dir;
      if ((comets[i].head_index == 0) || (comets[i].head_index == (NUM_SPHERES - 1))) {
        comets[i].dir *= -1; // Change direction at either end of string
        comets[i].hue = colour_theme_hue_random();
      }
    }
    
    leds[2*comets[i].head_index] = blend(CHSV(comets[i].hue + COMET_HUE_OFFSET/2 + scale8(cycle, COMET_HUE_WOBBLE), 255, max(comets[i].collision_glow, COMET_BRIGHTNESS)),
                                         COMET_EMBER_COLOUR, comets[i].collision_glow);
    leds[2*comets[i].head_index + 1] = blend(CHSV(comets[i].hue - COMET_HUE_OFFSET/2 - scale8(cycle, COMET_HUE_WOBBLE), 255, max(comets[i].collision_glow, COMET_BRIGHTNESS)),
                                             COMET_EMBER_COLOUR, comets[i].collision_glow);
    spheres[comets[i].head_index] = CHSV(comets[i].hue, comets[i].collision_glow, COMET_BRIGHTNESS); // Record original hue for fade code. Saturation used for collision glow fading..

    comets[i].collision_glow = qsub8(comets[i].collision_glow, 8); // Fade any collision glow
    
    // Handle collisions. This needs to be done in the loop to avoid comets simultaneously moving past each other
    for (int j = 0; j < comet_number_active; j ++) {
      if ( i != j && comets[i].head_index == comets[j].head_index) {
        leds[2*comets[i].head_index] = CRGB::White;
        leds[2*comets[i].head_index + 1] = CRGB::White;
        spheres[comets[i].head_index].saturation = 0;
        comets[i].collision_glow = 255;
        comets[j].collision_glow = 255;
      }
    }
  }

  cycle++; 
}


















/*
 * Star flicker effect
 */
#define HUE_WOBBLE 30
#define SATURATION_WOBBLE 64

#define SUPERNOVA_STARTDELAY 30000  // Delay before supernovas start appearing...
#define SUPERNOVA_INTERVAL 4000   // How many milliseconds between supernovas

// Stages of the supernova in sequence
#define SUPERNOVA_EXPLODE_DURATION 2000
#define SUPERNOVA_BRIGHT_DURATION  1000
#define SUPERNOVA_FADE_DURATION    3000
#define SUPERNOVA_REMNANT_DURATION  500
#define SUPERNOVA_REBORN_DURATION  1500

#define SUPERNOVA_EXPLODE_ENDTIME SUPERNOVA_EXPLODE_DURATION
#define SUPERNOVA_BRIGHT_ENDTIME (SUPERNOVA_EXPLODE_ENDTIME + SUPERNOVA_BRIGHT_DURATION)
#define SUPERNOVA_FADE_ENDTIME (SUPERNOVA_BRIGHT_ENDTIME + SUPERNOVA_FADE_DURATION)
#define SUPERNOVA_REMNANT_ENDTIME (SUPERNOVA_FADE_ENDTIME + SUPERNOVA_REMNANT_DURATION)
#define SUPERNOVA_REBORN_ENDTIME (SUPERNOVA_REMNANT_ENDTIME + SUPERNOVA_REBORN_DURATION)


enum star_state { Twinkle, Drifting, Supernova };

struct {
  byte state;
  union {
    unsigned long supernova_start_time;
    struct {
      byte original_hue;
      byte final_hue;
    } drift;
  };
} stars[NUM_SPHERES];

unsigned long last_supernova_time;
unsigned long last_drift_time;

void star_setup(byte data) {
  
  last_supernova_time = millis() + SUPERNOVA_STARTDELAY;
  last_drift_time = millis();
  
  for (byte i = 0; i < NUM_SPHERES; i++) {
    stars[i].state = Twinkle;
    stars[i].supernova_start_time = 0;
    spheres[i].hue = colour_theme_hue_random();
  }
}

// Matches 12 breaths a minute.
//    cycle time 255 cycles @ 50 cycles a second = 5 seconds per complete star_loop animation = 12 animations/minute.
void star_loop() {
  
  static byte cycle = 0;
    

  // Choose a drifting candidate
  if (cycle == 0) {
    byte i = random8(NUM_SPHERES - 1);
    if (stars[i].state == Twinkle && stars[i + 1].state == Twinkle) {
      stars[i].state = Drifting;
      stars[i + 1].state = Drifting;
      
      stars[i].drift.original_hue = spheres[i].hue;
      stars[i].drift.final_hue    = spheres[i + 1].hue;
      
      stars[i + 1].drift.original_hue = spheres[i + 1].hue;
      stars[i + 1].drift.final_hue    = spheres[i].hue;

      last_drift_time = millis();
    }
  }

  // Choose a supernova candidate
  if (feature_enabled && (millis() > last_supernova_time + SUPERNOVA_INTERVAL)) {
    byte i = random8(NUM_SPHERES);
    if (stars[i].state == Twinkle) {
      stars[i].state = Supernova;
      last_supernova_time = millis();
      stars[i].supernova_start_time = last_supernova_time;
    }
  }  
  
  for (byte i = 0; i < NUM_SPHERES; i++) {
    
    switch (stars[i].state) {
      
      case Drifting:
        spheres[i] = blend(CHSV(stars[i].drift.original_hue, 255, 255), CHSV(stars[i].drift.final_hue, 255, 255), cycle);      
        if (cycle == 255) { // End of drift..
          stars[i].state = Twinkle;
        }
        // Continue on to Twinkle case below..
        
      case Twinkle:
        star_twinkle(i, cycle);
        break;
        
      case Supernova:
        star_supernova(i);
        break;
    }
  }

  cycle++;  
}

void star_twinkle(byte star_index, byte cycle) {
  
  byte brightness = 120 + scale8(triwave8(cycle + 19 * star_index), 90);
    
  byte top_cycle = triwave8(cycle);
  leds[2*star_index] = CHSV(spheres[star_index].hue + scale8(top_cycle, HUE_WOBBLE),
                   255 - scale8(top_cycle, SATURATION_WOBBLE),
                   brightness);
  
  byte bottom_cycle = triwave8(cycle + 128);
  leds[2*star_index + 1] = CHSV(spheres[star_index].hue + scale8(bottom_cycle, HUE_WOBBLE),
                       255 - scale8(bottom_cycle, SATURATION_WOBBLE),
                       brightness);  
}




void star_supernova(byte star_index) {
  
  byte brightness;
  byte saturation;
  
  unsigned long time_since_supernova = millis() - stars[star_index].supernova_start_time;
  
  /*
   * !!!Supernova lifecycle!!!
   */
  if (time_since_supernova <= SUPERNOVA_EXPLODE_ENDTIME) {
    // Explosion phase
    spheres[star_index].hue = random8();
    brightness = 255;
    saturation = 100;
    
  } else if (time_since_supernova <= SUPERNOVA_BRIGHT_ENDTIME) {
    brightness = 255;
    saturation = 0;
    
  } else if (time_since_supernova <= SUPERNOVA_FADE_ENDTIME) {
    // Fading away
    byte fraction = (byte)(((time_since_supernova - SUPERNOVA_BRIGHT_ENDTIME) * 255) / SUPERNOVA_FADE_DURATION);
    brightness = lerp8by8(255, 15, fraction);
    saturation = 0;
    
  } else if (time_since_supernova <= SUPERNOVA_REMNANT_ENDTIME) {
    // Only a remnant remains...
    brightness = 0;
    spheres[star_index].hue = colour_theme_hue_random();
    
  } else if (time_since_supernova <= SUPERNOVA_REBORN_ENDTIME) {
    // Brightening up again
    byte fraction = (byte)(((time_since_supernova - SUPERNOVA_REMNANT_ENDTIME) * 255) / SUPERNOVA_REBORN_DURATION);
    brightness = lerp8by8(10, 200, fraction);
    saturation = 255;
    
  } else {
    // That's all folks!
    brightness = 200;
    saturation = 255;
    stars[star_index].state = Twinkle;
  }
  
  leds[2*star_index] = CHSV(spheres[star_index].hue, saturation, brightness);
  leds[2*star_index + 1] = leds[2*star_index];
}
