/* This file is part of the IEX2H5 project and is licensed under the MIT License.
 * 
 * Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca
 *
 * NOTE:
 *   - Implements fixed-width base76 encoding for stock symbols.
 *   - Encodes up to 8-character symbols into 50 bits using a 76-character alphabet.
 *   - The remaining 14 bits in a uint64_t are reserved for a symbol index (0–16383).
 */
