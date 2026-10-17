// Puente mando -> teclado para PPSSPP en macOS.
// Lee el mando ShanWan "PC/PS3/Android Gamepad" (2563:0575) por IOKit y envía
// las teclas que PPSSPP usa por defecto (controls.ini).
#include <IOKit/hid/IOHIDManager.h>
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Teclas de macOS (kVK_*)
enum { K_A = 0, K_S = 1, K_Z = 6, K_X = 7, K_Q = 12, K_W = 13, K_ENTER = 36, K_L = 37,
       K_J = 38, K_K = 40, K_I = 34, K_SPACE = 49, K_LEFT = 123, K_RIGHT = 124, K_DOWN = 125, K_UP = 126 };

// Botón HID (1..13) -> tecla. Distribución estilo PS3.
static const int button_key[14] = {
  -1,
  K_S,      // 1  Triángulo
  K_X,      // 2  Círculo
  K_Z,      // 3  Equis
  K_A,      // 4  Cuadrado
  K_Q,      // 5  L1 -> L
  K_W,      // 6  R1 -> R
  K_Q,      // 7  L2 -> L
  K_W,      // 8  R2 -> R
  K_ENTER,  // 9  Select
  K_SPACE,  // 10 Start
  -1, -1, -1 // 11 L3, 12 R3, 13 Home
};

static int pressed[128]; // contador por tecla: varias fuentes pueden mantener la misma tecla

static void key(int k, int down) {
  if (k < 0) return;
  int before = pressed[k] > 0;
  pressed[k] += down ? 1 : -1;
  if (pressed[k] < 0) pressed[k] = 0;
  int after = pressed[k] > 0;
  if (before == after) return;
  CGEventRef e = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)k, after);
  CGEventPost(kCGHIDEventTap, e);
  CFRelease(e);
}

// Fuentes direccionales con estado: cruceta y stick izquierdo
static int hat_dirs = 0, stick_x = 0, stick_y = 0; // bits: 1 arriba, 2 derecha, 4 abajo, 8 izquierda

static void set_dirs(int *state, int nuevo, const int keys[4]) {
  for (int i = 0; i < 4; i++) {
    int b = 1 << i;
    if ((*state & b) != (nuevo & b)) key(keys[i], nuevo & b);
  }
  *state = nuevo;
}

static const int arrows[4] = { K_UP, K_RIGHT, K_DOWN, K_LEFT };
static const int analog[4] = { K_I, K_L, K_K, K_J };

static void on_value(void *ctx, IOReturn r, void *sender, IOHIDValueRef v) {
  IOHIDElementRef e = IOHIDValueGetElement(v);
  uint32_t page = IOHIDElementGetUsagePage(e), usage = IOHIDElementGetUsage(e);
  long val = (long)IOHIDValueGetIntegerValue(v);

  if (page == kHIDPage_Button && usage >= 1 && usage <= 13) {
    key(button_key[usage], val != 0);
  } else if (page == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_Hatswitch) {
    static const int hat[8] = { 1, 1|2, 2, 2|4, 4, 4|8, 8, 8|1 };
    set_dirs(&hat_dirs, (val >= 0 && val < 8) ? hat[val] : 0, arrows);
  } else if (page == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_X) {
    set_dirs(&stick_x, val < 64 ? 8 : val > 192 ? 2 : 0, analog);
  } else if (page == kHIDPage_GenericDesktop && usage == kHIDUsage_GD_Y) {
    set_dirs(&stick_y, val < 64 ? 1 : val > 192 ? 4 : 0, analog);
  }
}

// Notificación de macOS + línea con hora en ~/ppsspp-mando/registro.log
static void avisar(const char *msg, const char *sonido) {
  char hora[32], cmd[256], ruta[512];
  time_t t = time(NULL);
  strftime(hora, sizeof hora, "%Y-%m-%d %H:%M:%S", localtime(&t));
  printf("[%s] %s\n", hora, msg); fflush(stdout);
  snprintf(ruta, sizeof ruta, "%s/ppsspp-mando/registro.log", getenv("HOME"));
  FILE *f = fopen(ruta, "a");
  if (f) { fprintf(f, "[%s] %s\n", hora, msg); fclose(f); }
  snprintf(cmd, sizeof cmd, "osascript -e 'display notification \"%s\" with title \"PPSSPP\"' >/dev/null 2>&1 &", msg);
  system(cmd);
  // Sonido aparte, para que se oiga aunque las notificaciones estén bloqueadas
  snprintf(cmd, sizeof cmd, "afplay /System/Library/Sounds/%s.aiff >/dev/null 2>&1 &", sonido);
  system(cmd);
}

static void on_attach(void *ctx, IOReturn r, void *sender, IOHIDDeviceRef d) { avisar("Mando conectado", "Pop"); }
static void on_detach(void *ctx, IOReturn r, void *sender, IOHIDDeviceRef d) {
  avisar("Mando desconectado", "Basso");
  for (int k = 0; k < 128; k++) if (pressed[k]) { pressed[k] = 1; key(k, 0); } // soltar todo
  hat_dirs = stick_x = stick_y = 0;
}

int main(void) {
  const void *opt_k[] = { kAXTrustedCheckOptionPrompt }, *opt_v[] = { kCFBooleanTrue };
  CFDictionaryRef opts = CFDictionaryCreate(NULL, opt_k, opt_v, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  if (!AXIsProcessTrustedWithOptions(opts))
    printf("AVISO: falta permiso de Accesibilidad para enviar teclas (Ajustes > Privacidad y seguridad > Accesibilidad).\n");

  IOHIDManagerRef m = IOHIDManagerCreate(kCFAllocatorDefault, 0);
  int vid = 0x2563, pid = 0x0575;
  CFNumberRef cv = CFNumberCreate(NULL, kCFNumberIntType, &vid), cp = CFNumberCreate(NULL, kCFNumberIntType, &pid);
  const void *k[] = { CFSTR(kIOHIDVendorIDKey), CFSTR(kIOHIDProductIDKey) }, *vals[] = { cv, cp };
  CFDictionaryRef match = CFDictionaryCreate(NULL, k, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  IOHIDManagerSetDeviceMatching(m, match);
  IOHIDManagerRegisterDeviceMatchingCallback(m, on_attach, NULL);
  IOHIDManagerRegisterDeviceRemovalCallback(m, on_detach, NULL);
  IOHIDManagerRegisterInputValueCallback(m, on_value, NULL);
  IOHIDManagerScheduleWithRunLoop(m, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
  if (IOHIDManagerOpen(m, 0) != kIOReturnSuccess) { printf("No se pudo abrir el gestor HID\n"); return 1; }
  printf("Puente activo. Ctrl+C para salir.\n"); fflush(stdout);
  CFRunLoopRun();
}
