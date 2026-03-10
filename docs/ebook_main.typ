// Main document for ESP32 Robot E-book
// Uses the No Starch Press style template

#import "ebook_template.typ": *

// Define horizontalrule that pandoc generates
#let horizontalrule = line(length: 100%, stroke: 0.5pt + rgb("#CCCCCC"))

#show: book.with(
  title: "Od Podstaw C do Profesjonalnego Developera Embedded",
  subtitle: "Kompleksowy Przewodnik po Budowie Robota ESP32-CAM",
  author: "Tutorial ESP32-CAM Robot Controller",
  version: "1.0",
)

// Reset chapter counter before content
#chapter-counter.update(0)

// ========================================
// CZĘŚĆ I: FUNDAMENTY
// ========================================

= Rozdział 1: Zaawansowane Koncepty Języka C

== 1.1 Wprowadzenie

Zanim przejdziemy do programowania mikrokontrolerów, musimy ugruntować wiedzę z zaawansowanych aspektów języka C. W tym rozdziale poznasz techniki, które są powszechnie używane w programowaniu embedded:

- *Struktury* -- grupowanie powiązanych danych
- *Wskaźniki* -- bezpośredni dostęp do pamięci
- *Dyrektywy preprocesora* -- konfiguracja kompilacji
- *Słowo kluczowe `static`* -- enkapsulacja w C

== 1.2 Struktury (struct)

=== 1.2.1 Podstawy struktur

Struktura to sposób na grupowanie powiązanych zmiennych różnych typów pod jedną nazwą. W programowaniu embedded struktury są wszechobecne -- używamy ich do konfiguracji peryferiów, przechowywania stanu urządzeń i przekazywania złożonych danych.

```c
// Podstawowa deklaracja struktury
struct motor_pins {
    int in1;  // Pin kierunku 1
    int in2;  // Pin kierunku 2
};
```

=== 1.2.2 typedef struct -- Tworzenie nowych typów

W praktyce niemal zawsze używamy `typedef` do tworzenia aliasu dla struktury. Dzięki temu nie musimy pisać słowa `struct` przy każdym użyciu:

```c
// Sposób 1: typedef po deklaracji struktury
struct motor_pins {
    int in1;
    int in2;
};
typedef struct motor_pins motor_pins_t;

// Sposób 2: typedef w jednej linii (preferowany w embedded)
typedef struct {
    int in1;
    int in2;
} motor_pins_t;
```

#quote[
  *Konwencja nazewnictwa:* W programowaniu embedded często dodajemy sufiks `_t` do nazw typów (np. `motor_pins_t`, `gpio_num_t`). Jest to konwencja pochodząca z POSIX, która ułatwia rozpoznawanie typów w kodzie.
]

=== 1.2.3 Designated Initializers (C99)

C99 wprowadził "designated initializers" -- sposób inicjalizacji struktur z nazwami pól:

```c
// Stary sposób (C89) - kolejność ma znaczenie!
motor_control_config_t config = {
    {12, 13},  // left_motor
    {14, 15},  // right_motor
    2,         // enable_pin
    1000,      // pwm_frequency_hz
    500,       // ramp_duration_ms
    25         // ramp_steps
};

// Nowy sposób (C99) - czytelniejszy i bezpieczniejszy
motor_control_config_t config = {
    .left_motor = {.in1 = 12, .in2 = 13},
    .right_motor = {.in1 = 14, .in2 = 15},
    .enable_pin = 2,
    .pwm_frequency_hz = 1000,
    .ramp_duration_ms = 500,
    .ramp_steps = 25
};
```

== 1.3 Wskaźniki zaawansowane

=== 1.3.1 Wskaźniki do struktur

Wskaźniki do struktur są kluczowe w embedded -- pozwalają przekazywać duże struktury bez kopiowania:

```c
// Przekazywanie przez wartość - KOPIUJE całą strukturę!
void bad_init(motor_control_config_t config);

// Przekazywanie przez wskaźnik - tylko 4/8 bajtów
void good_init(const motor_control_config_t *config);

// Użycie:
motor_control_config_t cfg = { ... };
good_init(&cfg);  // Przekazujemy adres
```

=== 1.3.2 Operator strzałki (`->`)

Do dostępu do pól struktury przez wskaźnik używamy operatora `->`:

```c
void print_config(const motor_control_config_t *config) {
    // Te dwie linie są równoważne:
    printf("PWM freq: %d\n", (*config).pwm_frequency_hz);
    printf("PWM freq: %d\n", config->pwm_frequency_hz);  // Preferowane!
}
```

=== 1.3.3 Słowa kluczowe const i volatile

```c
// const - wartość nie może być zmieniona
const int MAX_SPEED = 100;

// Wskaźnik do stałej (dane read-only)
const char *message = "Hello";

// Stały wskaźnik (adres nie może się zmienić)
char *const buffer = malloc(100);

// volatile - wartość może zmienić się "z zewnątrz"
// Używane dla rejestrów sprzętowych i zmiennych ISR
volatile uint32_t *gpio_register = (volatile uint32_t *)0x3FF44004;
```

#quote[
  *Ważne:* Słowo `volatile` informuje kompilator, że nie może optymalizować dostępu do tej zmiennej. Jest to krytyczne dla rejestrów sprzętowych i zmiennych modyfikowanych w przerwaniach (ISR).
]

== 1.4 Dyrektywy preprocesora

=== 1.4.1 Include guards

Include guards zapobiegają wielokrotnemu włączeniu tego samego nagłówka:

```c
// Plik: motor_control.h
#ifndef MOTOR_CONTROL_H      // Jeśli NIE zdefiniowano
#define MOTOR_CONTROL_H      // Zdefiniuj makro

// Tutaj zawartość nagłówka...

#endif /* MOTOR_CONTROL_H */ // Koniec warunku
```

=== 1.4.2 Makra z parametrami

```c
// Proste makro
#define MAX_MOTORS 4

// Makro z parametrami
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Makro wieloliniowe
#define ESP_ERROR_CHECK(x) do { \
    esp_err_t err = (x); \
    if (err != ESP_OK) { \
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(err)); \
        abort(); \
    } \
} while(0)
```

=== 1.4.3 Kompilacja warunkowa

```c
// Kompilacja zależna od platformy
#ifdef ESP32
    #include "esp_log.h"
    #define LOG_INFO(msg) ESP_LOGI(TAG, msg)
#else
    #include <stdio.h>
    #define LOG_INFO(msg) printf("[INFO] %s\n", msg)
#endif

// Tryb debug
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) // Nic nie robi
#endif
```

== 1.5 Słowo kluczowe static

=== 1.5.1 Zmienne statyczne lokalne

Zmienna statyczna lokalna zachowuje wartość między wywołaniami funkcji:

```c
int get_next_id(void) {
    static int counter = 0;  // Inicjalizacja tylko raz!
    return ++counter;
}

// Użycie:
printf("%d\n", get_next_id());  // 1
printf("%d\n", get_next_id());  // 2
printf("%d\n", get_next_id());  // 3
```

=== 1.5.2 Enkapsulacja w C

W C używamy `static` do tworzenia "prywatnych" funkcji i zmiennych modułu:

```c
// motor_control.c

// Prywatna zmienna - widoczna tylko w tym pliku
static bool initialized = false;

// Prywatna funkcja - nie eksportowana w .h
static void set_motor_direction(int pin1, int pin2, int dir) {
    // Implementacja...
}

// Publiczna funkcja - zadeklarowana w .h
esp_err_t motor_move_forward(uint32_t duration_ms) {
    if (!initialized) return ESP_ERR_INVALID_STATE;
    set_motor_direction(PIN1, PIN2, FORWARD);
    return ESP_OK;
}
```

== 1.6 Ćwiczenia

=== Ćwiczenie 1.1: Struktura konfiguracji LED

*Zadanie:* Zdefiniuj strukturę `led_config_t` z polami: `gpio_num`, `active_low` (bool), `blink_period_ms`.

*Rozwiązanie:*

```c
typedef struct {
    gpio_num_t gpio_num;      // Numer pinu GPIO
    bool active_low;          // true jeśli LED świeci przy LOW
    uint32_t blink_period_ms; // Okres migania w ms
} led_config_t;

// Użycie z designated initializer:
led_config_t led = {
    .gpio_num = GPIO_NUM_2,
    .active_low = false,
    .blink_period_ms = 500
};
```

=== Ćwiczenie 1.2: Makro bezpiecznego dzielenia

*Zadanie:* Napisz makro `SAFE_DIV(a, b)` które zwraca 0 gdy `b == 0`.

*Rozwiązanie:*

```c
#define SAFE_DIV(a, b) ((b) != 0 ? (a) / (b) : 0)

// Użycie:
int result = SAFE_DIV(100, 0);  // Zwróci 0, nie crash
```

#pagebreak()

// ========================================
// Continue with remaining chapters...
// Due to size, the full content would be included from the pandoc-generated file
// ========================================

#include "content_processed.typ"
