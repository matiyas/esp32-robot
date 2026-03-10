// ESP32 Robot E-book - Final Document
// Format: 165x228mm (No Starch Press style)

// ============ PAGE SETUP ============
#set page(
  width: 165mm,
  height: 228mm,
  margin: (
    top: 22mm,
    bottom: 22mm,
    inside: 20mm,
    outside: 16mm,
  ),
  header: context {
    if counter(page).get().first() > 6 {
      set text(size: 8pt, fill: rgb("#666666"), style: "italic")
      if calc.odd(counter(page).get().first()) {
        align(right)[Od Podstaw C do Profesjonalnego Developera Embedded]
      } else {
        align(left)[ESP32-CAM Robot Controller]
      }
      v(-2pt)
      line(length: 100%, stroke: 0.3pt + rgb("#CCCCCC"))
    }
  },
  footer: context {
    if counter(page).get().first() > 4 {
      set text(size: 9pt, fill: rgb("#666666"))
      align(center)[#counter(page).display()]
    }
  },
)

// ============ TYPOGRAPHY ============
#set text(
  font: ("Source Serif Pro", "Libertinus Serif", "Georgia", "Times New Roman"),
  size: 9.5pt,
  lang: "pl",
  hyphenate: true,
)

#set par(
  justify: true,
  leading: 0.65em,
  first-line-indent: 0.8em,
)

// ============ COLORS ============
#let accent-color = rgb("#B22222")
#let heading-color = rgb("#2F4F4F")
#let code-bg = rgb("#F7F7F7")
#let note-bg = rgb("#FFFAF0")

// ============ HEADINGS ============
#let chapter-counter = counter("chapter")

#show heading.where(level: 1): it => {
  pagebreak(weak: true)
  chapter-counter.step()
  v(25mm)
  set text(font: ("Source Sans Pro", "Helvetica Neue", "Arial"))
  block[
    #text(size: 11pt, weight: "regular", fill: accent-color, tracking: 0.1em)[
      ROZDZIAŁ #context chapter-counter.display()
    ]
    #v(3mm)
    #text(size: 20pt, weight: "bold", fill: heading-color)[#it.body]
  ]
  v(12mm)
}

#show heading.where(level: 2): it => {
  v(6mm)
  set text(font: ("Source Sans Pro", "Helvetica Neue", "Arial"), size: 13pt, weight: "bold", fill: heading-color)
  block[#it.body]
  v(3mm)
}

#show heading.where(level: 3): it => {
  v(5mm)
  set text(font: ("Source Sans Pro", "Helvetica Neue", "Arial"), size: 11pt, weight: "bold", fill: heading-color)
  block[#it.body]
  v(2mm)
}

#show heading.where(level: 4): it => {
  v(4mm)
  set text(font: ("Source Sans Pro", "Helvetica Neue", "Arial"), size: 10pt, weight: "bold", fill: heading-color)
  block[#it.body]
  v(2mm)
}

// ============ CODE BLOCKS ============
#show raw.where(block: true): it => {
  set text(font: ("Source Code Pro", "JetBrains Mono", "Fira Code", "Menlo"), size: 7.5pt)
  set par(justify: false, leading: 0.5em, first-line-indent: 0pt)
  v(2mm)
  block(
    width: 100%,
    fill: code-bg,
    stroke: (left: 2.5pt + accent-color, rest: 0.4pt + rgb("#E0E0E0")),
    inset: (left: 8pt, right: 6pt, top: 6pt, bottom: 6pt),
    radius: (right: 2pt),
  )[#it]
  v(2mm)
}

#show raw.where(block: false): it => {
  set text(font: ("Source Code Pro", "JetBrains Mono", "Menlo"), size: 8.5pt)
  box(fill: code-bg, inset: (x: 2pt, y: 0pt), radius: 1pt)[#it]
}

// ============ QUOTES / NOTES ============
#show quote: it => {
  v(2mm)
  block(
    width: 100%,
    fill: note-bg,
    stroke: (left: 2.5pt + rgb("#DAA520")),
    inset: 8pt,
    radius: (right: 2pt),
  )[
    #set text(size: 9pt)
    #it.body
  ]
  v(2mm)
}

// ============ TABLES ============
#set table(
  stroke: 0.4pt + rgb("#CCCCCC"),
  inset: 6pt,
)

#show table.cell.where(y: 0): it => {
  set text(weight: "bold", size: 8.5pt)
  it
}

// ============ LISTS ============
#set list(indent: 0.8em, marker: text(fill: accent-color, size: 8pt)[•])
#set enum(indent: 0.8em)

// ============ LINKS ============
#show link: it => {
  set text(fill: rgb("#0066CC"))
  it
}

// ============ HORIZONTAL RULE ============
#let horizontalrule = {
  v(4mm)
  line(length: 100%, stroke: 0.4pt + rgb("#DDDDDD"))
  v(4mm)
}

// ============ TITLE PAGE ============
#page(margin: (x: 25mm, y: 35mm), header: none, footer: none)[
  #v(15mm)
  #align(center)[
    #text(font: ("Source Sans Pro", "Helvetica Neue"), size: 10pt, fill: heading-color, tracking: 0.15em)[
      KOMPLEKSOWY PRZEWODNIK
    ]
    #v(6mm)
    #text(font: ("Source Sans Pro", "Helvetica Neue"), size: 24pt, weight: "bold", fill: heading-color)[
      Od Podstaw C do#linebreak()Profesjonalnego#linebreak()Developera Embedded
    ]
    #v(4mm)
    #text(font: ("Source Sans Pro", "Helvetica Neue"), size: 12pt, fill: accent-color)[
      Budowa Robota ESP32-CAM
    ]
    #v(20mm)
    #line(length: 30%, stroke: 1pt + accent-color)
    #v(20mm)
    #text(size: 10pt, fill: heading-color)[
      Tutorial wygenerowany na podstawie projektu#linebreak()
      ESP32-CAM Robot Controller
    ]
    #v(4mm)
    #text(size: 9pt, fill: rgb("#888888"))[
      Wersja 1.0
    ]
  ]
  #v(1fr)
  #align(center)[
    #text(size: 8pt, fill: rgb("#888888"))[
      Format: 165 × 228 mm
    ]
  ]
]

// ============ COPYRIGHT PAGE ============
#page(header: none, footer: none)[
  #v(1fr)
  #set text(size: 8.5pt)
  #set par(first-line-indent: 0pt, leading: 0.8em)

  *Od Podstaw C do Profesjonalnego Developera Embedded*

  _Kompleksowy Przewodnik po Budowie Robota ESP32-CAM_

  Wersja 1.0

  #v(4mm)

  Copyright © 2024

  Wszelkie prawa zastrzeżone.

  #v(4mm)

  *Wymagania wstępne:*
  Podstawowa znajomość języka C (zmienne, funkcje, pętle, tablice)

  #v(4mm)

  *Pokryte technologie:*
  - ESP-IDF v5.2 Framework
  - FreeRTOS
  - ESP32-CAM (AI-Thinker)
  - DRV8833 Motor Driver
  - SG90 Servo

  #v(8mm)

  #text(size: 7.5pt, fill: rgb("#999999"))[
    Wygenerowano przy użyciu Typst.#linebreak()
    Projekt dostępny na GitHub.
  ]
]

// ============ TABLE OF CONTENTS ============
#page(header: none)[
  #v(12mm)
  #align(center)[
    #text(font: ("Source Sans Pro", "Helvetica Neue"), size: 18pt, weight: "bold", fill: heading-color)[
      Spis Treści
    ]
  ]
  #v(8mm)

  #set text(size: 9pt)
  #set par(first-line-indent: 0pt)

  #outline(
    title: none,
    indent: 1.2em,
    depth: 2,
  )
]

#pagebreak()

// ============ RESET CHAPTER COUNTER ============
#chapter-counter.update(0)

// ============ INCLUDE CONTENT ============
// The content below is the processed e-book content

#include "content_processed.typ"
