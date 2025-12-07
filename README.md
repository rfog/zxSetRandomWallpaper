# zxSetRandomWallpaper

(Spanish below)

Sets a **random desktop wallpaper** across all desktops and monitors from a specified folder, which must contain a collection of images.

## Usage

* Place **zxSetRandomWallpaper.exe** in any folder
* The usage is `zxSetRandomWallpaper <image_path>`
* Install the latest **Visual C++ runtimes**
* Create a task to execute the program with its parameter at the desired frequency. Eg., run each hour:
  > schtasks /Create /TN "ActualizarFondoAleatorio" /TR "C:\Ruta\Al\zxSetRandomWallpaper.exe C:\Ruta\A\Las\Imágenes" /SC HOURLY /ST 09:00

## Laughs
I spent about an hour with Copilot (via GPT5-mini) trying to get it to fix its own error on the line

> IActiveDesktop* pActiveDesktop = nullptr;

Which basically involved adjusting the order of the header file inclusions.

Gemini 3.0 fixed it in 30 seconds.

---
Establece un fondo de escritorio aleatorio en todos los escritorios y monitores a partir de una carpeta, que debe contener una colección de imágenes.

## Uso

- Pon zxSetRandomWallpaper.exe en cualquier carpeta
- El uso es zxSetRandomWallpaper <ruta_imágenes>
- Instala la última versión de los runtimes de Visual C++
- Crea una tarea que ejecute con la periodicidad deseada el programa con su parámetro. Por ejemplo, que se ejeute cada hora:
  > schtasks /Create /TN "ActualizarFondoAleatorio" /TR "C:\Ruta\Al\zxSetRandomWallpaper.exe C:\Ruta\A\Las\Imágenes" /SC HOURLY /ST 09:00

## Risas
Estuve con el Copilot (via GPT5-mini) como una hora para que solucionara su propio error en la línea

> IActiveDesktop* pActiveDesktop = nullptr;

Que básicamente era adecuar el orden de inclusión de los ficheros cabeceras.

Gemini 3.0 lo solucionó en 30 segundos.



