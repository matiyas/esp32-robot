// No Starch Press style template for ESP32 Robot E-book
// Page size: 165x228mm (similar to trade paperback)

#let defined-fonts = (
  // Primary fonts - will fall back if not available
  serif: ("Source Serif Pro", "Libertinus Serif", "Linux Libertine", "Georgia", "Times New Roman"),
  sans: ("Source Sans Pro", "Libertinus Sans", "Helvetica Neue", "Arial"),
  mono: ("Source Code Pro", "JetBrains Mono", "Fira Code", "Menlo", "Monaco", "Consolas"),
)

// Colors
#let colors = (
  primary: rgb("#B22222"),      // Dark red for accents
  secondary: rgb("#2F4F4F"),    // Dark slate for headings
  code-bg: rgb("#F5F5F5"),      // Light gray for code
  code-border: rgb("#E0E0E0"),  // Border for code blocks
  link: rgb("#0066CC"),         // Blue for links
  note-bg: rgb("#FFF8DC"),      // Cream for notes
  note-border: rgb("#DAA520"),  // Gold border for notes
)

// Chapter counter
#let chapter-counter = counter("chapter")

// Main document setup
#let book(
  title: "",
  subtitle: "",
  author: "",
  version: "",
  body,
) = {
  // Page setup
  set page(
    paper: "custom",
    width: 165mm,
    height: 228mm,
    margin: (
      top: 25mm,
      bottom: 25mm,
      inside: 22mm,  // Larger for binding
      outside: 18mm,
    ),
    header: context {
      if counter(page).get().first() > 4 {
        let chapter-title = query(selector(heading.where(level: 1)).before(here()))
        if chapter-title.len() > 0 {
          let title-text = chapter-title.last().body
          set text(size: 9pt, fill: colors.secondary, style: "italic")
          if calc.odd(counter(page).get().first()) {
            align(right)[#title-text]
          } else {
            align(left)[Od Podstaw C do Profesjonalnego Developera Embedded]
          }
          v(-3pt)
          line(length: 100%, stroke: 0.5pt + colors.secondary)
        }
      }
    },
    footer: context {
      if counter(page).get().first() > 2 {
        set text(size: 9pt, fill: colors.secondary)
        let page-num = counter(page).get().first()
        if calc.odd(page-num) {
          align(right)[#page-num]
        } else {
          align(left)[#page-num]
        }
      }
    },
  )

  // Typography
  set text(
    font: defined-fonts.serif,
    size: 10pt,
    lang: "pl",
    hyphenate: true,
  )

  set par(
    justify: true,
    leading: 0.7em,
    first-line-indent: 1em,
  )

  // Headings
  set heading(numbering: none)

  show heading.where(level: 1): it => {
    pagebreak(weak: true)
    chapter-counter.step()
    v(30mm)
    set text(font: defined-fonts.sans, fill: colors.secondary)
    block[
      #text(size: 14pt, weight: "regular", fill: colors.primary)[ROZDZIAŁ #context chapter-counter.display()]
      #v(5mm)
      #text(size: 24pt, weight: "bold")[#it.body]
    ]
    v(15mm)
  }

  show heading.where(level: 2): it => {
    v(8mm)
    set text(font: defined-fonts.sans, size: 14pt, weight: "bold", fill: colors.secondary)
    block[#it.body]
    v(4mm)
  }

  show heading.where(level: 3): it => {
    v(6mm)
    set text(font: defined-fonts.sans, size: 12pt, weight: "bold", fill: colors.secondary)
    block[#it.body]
    v(3mm)
  }

  show heading.where(level: 4): it => {
    v(4mm)
    set text(font: defined-fonts.sans, size: 11pt, weight: "bold", fill: colors.secondary)
    block[#it.body]
    v(2mm)
  }

  // Code blocks
  show raw.where(block: true): it => {
    set text(font: defined-fonts.mono, size: 8.5pt)
    set par(justify: false, leading: 0.55em, first-line-indent: 0pt)
    v(3mm)
    block(
      width: 100%,
      fill: colors.code-bg,
      stroke: (left: 3pt + colors.primary, rest: 0.5pt + colors.code-border),
      inset: (left: 10pt, right: 8pt, top: 8pt, bottom: 8pt),
      radius: (right: 3pt),
    )[#it]
    v(3mm)
  }

  // Inline code
  show raw.where(block: false): it => {
    set text(font: defined-fonts.mono, size: 9pt)
    box(
      fill: colors.code-bg,
      inset: (x: 3pt, y: 0pt),
      radius: 2pt,
    )[#it]
  }

  // Links
  show link: it => {
    set text(fill: colors.link)
    underline(it)
  }

  // Block quotes (notes/tips)
  show quote: it => {
    v(3mm)
    block(
      width: 100%,
      fill: colors.note-bg,
      stroke: (left: 3pt + colors.note-border),
      inset: 10pt,
      radius: (right: 3pt),
    )[
      #set text(size: 9.5pt, style: "italic")
      #it.body
    ]
    v(3mm)
  }

  // Tables
  show table: it => {
    set text(size: 9pt)
    v(3mm)
    block(width: 100%)[#it]
    v(3mm)
  }

  show table.cell.where(y: 0): strong

  // Lists
  set list(indent: 1em, marker: text(fill: colors.primary)[•])
  set enum(indent: 1em)

  // Horizontal rules
  show line: it => {
    v(5mm)
    line(length: 100%, stroke: 0.5pt + colors.code-border)
    v(5mm)
  }

  // ============ TITLE PAGE ============
  page(
    margin: (x: 25mm, y: 30mm),
    header: none,
    footer: none,
  )[
    #v(20mm)
    #align(center)[
      #text(font: defined-fonts.sans, size: 11pt, fill: colors.secondary, tracking: 0.2em)[
        KOMPLEKSOWY PRZEWODNIK
      ]
      #v(8mm)
      #text(font: defined-fonts.sans, size: 28pt, weight: "bold", fill: colors.secondary)[
        #title
      ]
      #v(5mm)
      #text(font: defined-fonts.sans, size: 14pt, fill: colors.primary)[
        #subtitle
      ]
      #v(25mm)
      #line(length: 40%, stroke: 1pt + colors.primary)
      #v(25mm)
      #text(font: defined-fonts.serif, size: 11pt, fill: colors.secondary)[
        #author
      ]
      #v(5mm)
      #text(font: defined-fonts.serif, size: 10pt, fill: colors.secondary)[
        Wersja #version
      ]
    ]
    #v(1fr)
    #align(center)[
      #text(font: defined-fonts.sans, size: 9pt, fill: colors.secondary)[
        Tutorial wygenerowany na podstawie projektu ESP32-CAM Robot Controller
      ]
    ]
  ]

  // ============ COPYRIGHT PAGE ============
  page(
    header: none,
    footer: none,
  )[
    #v(1fr)
    #set text(size: 9pt)
    #set par(first-line-indent: 0pt)

    *Od Podstaw C do Profesjonalnego Developera Embedded*

    Wersja #version

    #v(5mm)

    Copyright © 2024

    Wszelkie prawa zastrzeżone. Żadna część tej publikacji nie może być
    powielana ani rozpowszechniana bez pisemnej zgody autora.

    #v(5mm)

    *Wymagania wstępne:*
    Podstawowa znajomość języka C (zmienne, funkcje, pętle, tablice)

    #v(5mm)

    Projekt dostępny na GitHub.

    #v(10mm)

    #text(size: 8pt, fill: colors.secondary)[
      Wygenerowano przy użyciu Typst.
    ]
  ]

  // ============ TABLE OF CONTENTS ============
  page(
    header: none,
  )[
    #v(15mm)
    #align(center)[
      #text(font: defined-fonts.sans, size: 20pt, weight: "bold", fill: colors.secondary)[
        Spis Treści
      ]
    ]
    #v(10mm)

    #set text(size: 10pt)
    #set par(first-line-indent: 0pt)

    #outline(
      title: none,
      indent: 1.5em,
      depth: 2,
    )
  ]

  pagebreak()

  // ============ MAIN CONTENT ============
  body
}

// Helper function for notes/tips
#let note(body) = {
  block(
    width: 100%,
    fill: colors.note-bg,
    stroke: (left: 3pt + colors.note-border),
    inset: 10pt,
    radius: (right: 3pt),
  )[
    #set text(size: 9.5pt)
    #strong[Uwaga:] #body
  ]
}

// Helper for terminal/shell commands
#let terminal(body) = {
  block(
    width: 100%,
    fill: rgb("#1E1E1E"),
    stroke: 0.5pt + rgb("#333333"),
    inset: 10pt,
    radius: 3pt,
  )[
    #set text(font: defined-fonts.mono, size: 8.5pt, fill: rgb("#D4D4D4"))
    #body
  ]
}
