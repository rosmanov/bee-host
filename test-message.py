#!/usr/bin/env python3

import json
import struct
import sys

def main():
    text = """
Markus Kuhn [ˈmaʳkʊs kuːn] <mkuhn@acm.org> — 1999-08-20

The ASCII compatible UTF-8 encoding of ISO 10646 and Unicode
plain-text files is defined in RFC 2279 and in ISO 10646-1 Annex R.

Using Unicode/UTF-8, you can write in emails and source code things such as

Mathematics and Sciences:

  ∮ E⋅da = Q,  n → ∞, ∑ f(i) = ∏ g(i), ∀x∈ℝ: ⌈x⌉ = −⌊−x⌋, α ∧ ¬β = ¬(¬α ∨ β),

  ℕ ⊆ ℕ₀ ⊂ ℤ ⊂ ℚ ⊂ ℝ ⊂ ℂ, ⊥ < a ≠ b ≡ c ≤ d ≪ ⊤ ⇒ (A ⇔ B),

  2H₂ + O₂ ⇌ 2H₂O, R = 4.7 kΩ, ⌀ 200 mm

Linguistics and dictionaries:

  ði ıntəˈnæʃənəl fəˈnɛtık əsoʊsiˈeıʃn
  Y [ˈʏpsilɔn], Yen [jɛn], Yoga [ˈjoːgɑ]

APL:

  ((V⍳V)=⍳⍴V)/V←,V    ⌷←⍳→⍴∆∇⊃‾⍎⍕⌈

Nicer typography in plain text files:
    """
    editor = ""
    # editor = "C:\\Program Files\\Common Files\\notepad.exe"
    #editor = "C:\\Program Files\\Common Files\\notepad.exe"
    # editor = "C:\\Windows\\space notepad.exe"
    # editor = "C:\\Windows\\notepad.exe"
    # editor = "notepad.exe"
    args = [ '-c',  ':set ft=markdown', '-c', ':set tw=12345']

    response = json.dumps({"text": text, "editor": editor, "args": args, "ext": "txt"})
    sys.stdout.buffer.write(struct.pack('I', len(response)))
    # Write message itself
    sys.stdout.write(response)
    sys.stdout.flush()

    sys.exit(0)


if __name__ == '__main__':
    main()
