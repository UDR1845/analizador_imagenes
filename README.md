# Procesamiento de Imágenes con C++ y OpenMP

Implementación secuencial y paralela de un filtro de desenfoque de imágenes RGB mediante C++ y OpenMP. El programa compara los tiempos de ejecución utilizando diferentes cantidades de hilos y verifica la equivalencia de los resultados.

## Requisitos

- GCC (`g++`) con soporte para C++17 y OpenMP.
- Windows con MinGW-w64/MSYS2, o Linux con GCC.

## Compilación

Desde la carpeta del proyecto, ejecuta:

```bash
g++ -O3 -std=c++17 -fopenmp main.cpp -o procesamiento.exe
```

## Pruebas

Ejecutar con una imagen Full HD (1920 × 1080), realizando 5 repeticiones:

```bash
procesamiento.exe 1920 1080 5
```

Ejecutar con una imagen 4K UHD (3840 × 2160), realizando 5 repeticiones:

```bash
procesamiento.exe 3840 2160 5
```

### Parámetros

```text
procesamiento.exe [ancho] [alto] [repeticiones]
```

Si no se especifican argumentos, el programa utiliza por defecto una imagen de 3840 × 2160 píxeles y 5 repeticiones.

El programa muestra los tiempos de ejecución, el *speedup* y la eficiencia paralela. También verifica los resultados y genera los archivos `original.ppm` y `desenfocada.ppm`.