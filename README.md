# ppsspp-mando-MacOs

Puente para usar un **mando genérico USB** en **PPSSPP** en macOS cuando el emulador no lo detecta.

Lee el mando directamente con IOKit y lo convierte en las teclas que PPSSPP ya tiene asignadas por defecto. Para PPSSPP es como si estuvieras usando el teclado.

## El problema

En Windows estos mandos tienen un modo XInput (el del Xbox): presionas Start/Home y cualquier juego los reconoce. En macOS no existe ese modo.

Para leer mandos, PPSSPP usa **SDL**. La versión que trae PPSSPP 1.20.4 (SDL 2.26.5) **no detecta ningún mando en macOS 27**. Aunque macOS sí reconoce el mando y entrega sus datos, SDL lista 0 dispositivos. Agregar un mapeo (`SDL_GAMECONTROLLERCONFIG`) tampoco ayuda, porque SDL nunca llega a ver el dispositivo.

Cómo se diagnosticó:

| Prueba | Resultado |
|---|---|
| `ioreg` / `system_profiler` | macOS ve el mando: `PC/PS3/Android Gamepad`, VID `0x2563`, PID `0x0575` (chip ShanWan) |
| Lector HID en C con IOKit | Llegan botones, cruceta y sticks sin problema |
| Programa mínimo con el SDL de PPSSPP | 0 joysticks, con y sin HIDAPI |
| Framework GameController de Apple | No lo toma |

## La solución

`puente.c` es un programa pequeño en C que:

1. Abre el mando con `IOHIDManager` (filtrando por VID/PID).
2. Por cada botón o eje que cambia, envía un evento de teclado con `CGEventPost`.
3. Lleva la cuenta de cada tecla, para que dos botones que comparten tecla (L1 y L2, por ejemplo) no se pisen.
4. Si el mando se desconecta, suelta todas las teclas, muestra una notificación, reproduce un sonido y lo anota en `registro.log`.

### Asignación

Usa las teclas por defecto de PPSSPP (`controls.ini`), así que no hay que configurar nada en el emulador:

| Mando | Tecla | PSP |
|---|---|---|
| Cruceta | Flechas | Cruceta |
| Stick izquierdo | I J K L | Stick analógico |
| Equis | Z | Equis |
| Círculo | X | Círculo |
| Cuadrado | A | Cuadrado |
| Triángulo | S | Triángulo |
| L1 / L2 | Q | L |
| R1 / R2 | W | R |
| Start | Espacio | Start |
| Select | Enter | Select |

Para cambiarla, edita la tabla `button_key` de `puente.c` y vuelve a compilar.

## Uso

Requisitos: macOS, [PPSSPP](https://www.ppsspp.org/) en `/Applications/PPSSPPSDL.app` y las herramientas de línea de comandos (`xcode-select --install`).

```sh
git clone git@github.com:LOctavioDev/ppsspp-mando-MacOs.git
cd ppsspp-mando-MacOs
./compilar.sh
```

Para jugar:

1. Conecta el mando.
2. Abre **`PPSSPP-con-mando.command`** con doble clic. Arranca el puente y PPSSPP, y apaga el puente al cerrar el emulador.
3. Juega con la ventana de PPSSPP al frente, porque las teclas van a la ventana activa.

**La primera vez** macOS pide permiso de **Accesibilidad** para poder enviar teclas: Ajustes del Sistema → Privacidad y seguridad → Accesibilidad → activa **Terminal**. Luego vuelve a abrir el lanzador.

### Como app (Spotlight, Dock, Launchpad)

Si abres el `.command` desde Spotlight, macOS dice que *Siri no tiene permiso para abrir documentos en Terminal*. Para evitarlo, crea una app:

```sh
./crear-app.sh
```

Esto deja **`PPSSPP con mando.app`** en `~/Applications`, con el ícono de PPSSPP. Ábrela desde Spotlight, el Dock o Launchpad. No abre Terminal. La primera vez, el permiso de Accesibilidad hay que dárselo a **PPSSPP con mando** (en lugar de a Terminal).

Si mueves la carpeta del repositorio, vuelve a ejecutar `./crear-app.sh`.

### Avisos

| Evento | Sonido | Notificación |
|---|---|---|
| Mando conectado | Pop | "Mando conectado" |
| Mando desconectado | Basso | "Mando desconectado" |

Cada evento queda en `registro.log` con fecha y hora. Si el juego deja de responder **sin** aviso de desconexión, lo más probable es que la ventana de PPSSPP perdió el foco.

## Archivos

| Archivo | Qué es |
|---|---|
| `puente.c` | Código del puente |
| `compilar.sh` | Compila el puente. Si el compilador no coincide con el SDK más nuevo (pasa tras actualizar macOS), prueba con SDK anteriores |
| `PPSSPP-con-mando.command` | Lanzador de doble clic. Compila el puente si hace falta |
| `crear-app.sh` | Crea `PPSSPP con mando.app` en `~/Applications` para abrirlo desde Spotlight, el Dock o Launchpad |

## Otro mando

El puente busca el VID/PID `0x2563:0x0575`. Para otro mando, busca sus IDs con:

```sh
ioreg -p IOUSB -l -w0 | grep -E '"USB Product Name"|"idVendor"|"idProduct"'
```

Cambia `vid` y `pid` en `main()` de `puente.c`. Si los botones salen en otro orden, ajusta `button_key`.
