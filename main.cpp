/*
COSC 3307 - Project: Interactive Fireworks Display
Sana Begum | Nipissing University


All logic lives in FireworksApp ( include/app.h, src/app.cpp).

Controls:
  1 / SPACE       : Launch starburst firework (center screen)
  2               : Launch fountain firework (center ground)
  3               : Launch rocket-with-trail firework
  4               : Launch cascade firework
  Arrow Up/Down   : Pitch camera
  Arrow Left/Right: Yaw camera
  A / D           : Roll camera
  W / S           : Move forward / backward
  Q               : Quit
*/

#include <app.h>

int main(void) {
    FireworksApp app;
    return app.Init();
}
