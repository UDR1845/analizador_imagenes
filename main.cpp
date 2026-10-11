
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <omp.h>

using PixelData = std::vector<std::uint8_t>;
using Clock = std::chrono::steady_clock;

PixelData generarImagen(int ancho, int alto) {
    PixelData imagen(
        static_cast<std::size_t>(ancho) * alto * 3
    );

    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x) {
            const std::size_t i =
                (static_cast<std::size_t>(y) * ancho + x) * 3;

            imagen[i]     = (x * 7 + y * 3) % 256;
            imagen[i + 1] = (x * 2 + y * 5) % 256;
            imagen[i + 2] = (x + y * 9) % 256;
        }
    }

    return imagen;
}

void guardarPPM(
    const std::string& nombre,
    const PixelData& imagen,
    int ancho,
    int alto
) {
    std::ofstream archivo(nombre, std::ios::binary);

    if (!archivo) {
        throw std::runtime_error(
            "No se pudo crear el archivo: " + nombre
        );
    }

    archivo << "P6\n" << ancho << ' ' << alto << "\n255\n";

    archivo.write(
        reinterpret_cast<const char*>(imagen.data()),
        static_cast<std::streamsize>(imagen.size())
    );
}

void filtroSecuencial(
    const PixelData& entrada,
    PixelData& salida,
    int ancho,
    int alto
) {
    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x) {
            int sumaR = 0;
            int sumaG = 0;
            int sumaB = 0;
            int contador = 0;

            for (int dy = -1; dy <= 1; ++dy) {
                const int ny = y + dy;

                if (ny < 0 || ny >= alto) continue;

                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = x + dx;

                    if (nx < 0 || nx >= ancho) continue;

                    const std::size_t i =
                        (static_cast<std::size_t>(ny) * ancho + nx) * 3;

                    sumaR += entrada[i];
                    sumaG += entrada[i + 1];
                    sumaB += entrada[i + 2];
                    ++contador;
                }
            }

            const std::size_t o =
                (static_cast<std::size_t>(y) * ancho + x) * 3;

            salida[o]     = sumaR / contador;
            salida[o + 1] = sumaG / contador;
            salida[o + 2] = sumaB / contador;
        }
    }
}

void filtroParalelo(
    const PixelData& entrada,
    PixelData& salida,
    int ancho,
    int alto
) {
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x) {
            int sumaR = 0;
            int sumaG = 0;
            int sumaB = 0;
            int contador = 0;

            for (int dy = -1; dy <= 1; ++dy) {
                const int ny = y + dy;

                if (ny < 0 || ny >= alto) continue;

                for (int dx = -1; dx <= 1; ++dx) {
                    const int nx = x + dx;

                    if (nx < 0 || nx >= ancho) continue;

                    const std::size_t i =
                        (static_cast<std::size_t>(ny) * ancho + nx) * 3;

                    sumaR += entrada[i];
                    sumaG += entrada[i + 1];
                    sumaB += entrada[i + 2];
                    ++contador;
                }
            }

            const std::size_t o =
                (static_cast<std::size_t>(y) * ancho + x) * 3;

            salida[o]     = sumaR / contador;
            salida[o + 1] = sumaG / contador;
            salida[o + 2] = sumaB / contador;
        }
    }
}

template <typename Funcion>
double medirMinimo(
    Funcion funcion,
    int repeticiones
) {
    double minimo = 1e100;

    for (int i = 0; i < repeticiones; ++i) {
        const auto inicio = Clock::now();
        funcion();
        const auto fin = Clock::now();

        const double segundos =
            std::chrono::duration<double>(fin - inicio).count();

        minimo = std::min(minimo, segundos);
    }

    return minimo;
}

int main(int argc, char* argv[]) {
    try {
        const int ancho = argc > 1 ? std::stoi(argv[1]) : 3840;
        const int alto = argc > 2 ? std::stoi(argv[2]) : 2160;
        const int repeticiones = argc > 3 ? std::stoi(argv[3]) : 5;

        if (ancho < 1 || alto < 1 || repeticiones < 1) {
            throw std::invalid_argument(
                "Las dimensiones y repeticiones deben ser positivas."
            );
        }

        const std::size_t bytes =
            static_cast<std::size_t>(ancho) * alto * 3;

        if (bytes > PixelData{}.max_size()) {
            throw std::length_error("La imagen es demasiado grande.");
        }

        const PixelData entrada = generarImagen(ancho, alto);

        PixelData salidaSecuencial(bytes);
        PixelData salidaParalela(bytes);

        // Calentamiento y verificación inicial.
        filtroSecuencial(
            entrada, salidaSecuencial, ancho, alto
        );

        filtroParalelo(
            entrada, salidaParalela, ancho, alto
        );

        if (salidaSecuencial != salidaParalela) {
            throw std::runtime_error(
                "Los resultados iniciales no coinciden."
            );
        }

        // Las asignaciones de memoria quedan fuera de la medición.
        const double tiempoSecuencial = medirMinimo(
            [&]() {
                filtroSecuencial(
                    entrada, salidaSecuencial, ancho, alto
                );
            },
            repeticiones
        );

        std::cout << std::fixed << std::setprecision(6);

        std::cout << "Imagen: " << ancho << " x " << alto << '\n';
        std::cout << "Pixeles: "
                  << static_cast<std::size_t>(ancho) * alto << '\n';
        std::cout << "Repeticiones: " << repeticiones << '\n';
        std::cout << "Tiempo secuencial: "
                  << tiempoSecuencial << " s\n\n";

        std::cout
            << "Hilos\tTiempo(s)\tSpeedup\t\tEficiencia(%)\n";

        const int maxHilos = omp_get_num_procs();

        for (int hilos = 1; hilos <= maxHilos; hilos *= 2) {
            omp_set_dynamic(0);
            omp_set_num_threads(hilos);

            const double tiempoParalelo = medirMinimo(
                [&]() {
                    filtroParalelo(
                        entrada, salidaParalela, ancho, alto
                    );
                },
                repeticiones
            );

            if (salidaSecuencial != salidaParalela) {
                throw std::runtime_error(
                    "Los resultados difieren con " +
                    std::to_string(hilos) + " hilos."
                );
            }

            const double speedup =
                tiempoSecuencial / tiempoParalelo;

            const double eficiencia =
                100.0 * speedup / hilos;

            std::cout << hilos << '\t'
                      << tiempoParalelo << '\t'
                      << speedup << '\t'
                      << eficiencia << '\n';
        }

        // Se guardan las imágenes para inspeccionar el resultado.
        guardarPPM("original.ppm", entrada, ancho, alto);
        guardarPPM(
            "desenfocada.ppm",
            salidaParalela,
            ancho,
            alto
        );

        std::cout << "\nVerificacion: correcta.\n";
        std::cout << "Archivos: original.ppm y desenfocada.ppm\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
