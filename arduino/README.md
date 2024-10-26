# Arduino kód

## Kompilácia s PlatformIO v editore VS Code

Stačí vytvoriť nový projekt:
- Board: Arduino Nano ATmega328
- Framework: Arduino
- pridať knižnicu FastLED (v projekte je použitá verzia 3.8.0)
- súbory main.cpp a globals.h umiestniť do priečinka src v projekte
- zvoliť správny COM port (alebo ponechať na Auto) a Uploadnuť program


## Kompilácia v Arduino IDE

- vytvoriť nový Sketch
- prekopírovať obsah súboru main.cpp do Sketch-u (do súboru s príponou .ino)
- do priečinka so Sketch-om pridať súbot globals.h
- nainštalovať knižnicu FastLED cez Library Manager
- zvoliť správny COM port, Board: Arduino Nano a Uploadnuť program