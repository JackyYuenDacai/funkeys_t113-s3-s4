# Colorbar Register Analysis

## Key Finding

**Register at offset 0x40 (0x05461040) controls TCON colorbar patterns!**

### Register Changes

**Colorbar 0 (framebuffer picture):**
```
05461040: 81000000
```
- Bit 0 = 0 (pattern disabled)
- Bit 24 = 1 (some control bit)

**Colorbar 1 (TCON pattern 1):**
```
05461040: 81000001
```
- Bit 0 = 1 (pattern enabled)
- Bit 24 = 1 (control bit)

## Register Bit Analysis

### Register 0x05461040 (TCON Pattern Control)

**Bit Fields (likely):**
- **Bit 0**: Pattern enable (0=disabled, 1=enabled)
- **Bits 1-3**: Pattern mode selection (0-7)
- **Bit 24**: Some control/enable bit
- **Other bits**: Additional control/configuration

### Expected Values for Different Modes

Based on the pattern, here's what you should see:

```
colorbar 0: 0x81000000  (bit 0 = 0, pattern disabled)
colorbar 1: 0x81000001  (bit 0 = 1, bits 1-3 = 0b000 = mode 0)
colorbar 2: 0x81000003  (bit 0 = 1, bits 1-3 = 0b001 = mode 1)
colorbar 3: 0x81000005  (bit 0 = 1, bits 1-3 = 0b010 = mode 2)
colorbar 4: 0x81000007  (bit 0 = 1, bits 1-3 = 0b011 = mode 3)
colorbar 5: 0x81000009  (bit 0 = 1, bits 1-3 = 0b100 = mode 4)
colorbar 6: 0x8100000B  (bit 0 = 1, bits 1-3 = 0b101 = mode 5)
colorbar 7: 0x8100000D  (bit 0 = 1, bits 1-3 = 0b110 = mode 6)
colorbar 8: 0x81000000  (bit 0 = 0, DE colorbar, not TCON pattern)
```

**Note:** The exact bit mapping might be different. Check by reading the register for each colorbar mode.

## Testing Commands

### Check All Colorbar Modes

```bash
# Colorbar 0
=> colorbar 0
=> md.l 0x05461040 0x1

# Colorbar 1
=> colorbar 1
=> md.l 0x05461040 0x1

# Colorbar 2
=> colorbar 2
=> md.l 0x05461040 0x1

# ... continue for modes 3-8
```

### Quick Check Script (in U-Boot)

```bash
# Check pattern register for all modes
=> colorbar 0; md.l 0x05461040 0x1
=> colorbar 1; md.l 0x05461040 0x1
=> colorbar 2; md.l 0x05461040 0x1
=> colorbar 3; md.l 0x05461040 0x1
=> colorbar 4; md.l 0x05461040 0x1
=> colorbar 5; md.l 0x05461040 0x1
=> colorbar 6; md.l 0x05461040 0x1
=> colorbar 7; md.l 0x05461040 0x1
=> colorbar 8; md.l 0x05461040 0x1
```

## Other Important Registers

### Register 0x05461000 (TCON Control)
```
05461000: 80000000
```
- Bit 31 = 1 (TCON enabled)
- This stays constant

### Register 0x05461060 (Timing/Control)
```
05461060: 10400005  (colorbar 0)
05461060: 10400005  (colorbar 1) - unchanged
```
- This register doesn't change between colorbar modes
- Likely contains timing or interface configuration

## Why Colorbar 1-7 Show Displacement

### The Problem

When TCON pattern mode is enabled (bit 0 = 1 in register 0x40):
1. **TCON generates pattern internally** - bypasses DE layer
2. **Uses TCON's internal coordinate system** - not aligned with DE layer settings
3. **Doesn't respect DE layer crop/frame** - TCON patterns start at (0,0) in TCON's view
4. **Timing may not match panel offsets** - TCON patterns use raw timing, not adjusted for panel

### Why Colorbar 8 Works

When colorbar 8 is active:
- Register 0x05461040: `0x81000000` (bit 0 = 0, pattern disabled)
- **DE generates the colorbar** - uses DE layer composition
- **Respects DE layer settings** - crop, frame, coordinate system
- **Properly aligned** - uses the same pipeline as normal rendering

## Understanding the Displacement

The displacement you see with colorbar 1-7 is because:

1. **TCON patterns are hardware-generated** - they don't go through the DE layer pipeline
2. **TCON uses its own origin** - typically (0,0) in TCON's coordinate space
3. **No layer transformation** - TCON patterns bypass DE layer crop/frame adjustments
4. **Timing differences** - TCON patterns may use different timing calculations than DE

This is **expected behavior** for TCON test patterns - they're designed for hardware testing, not for display alignment verification.

## Next Steps

1. **Read register 0x40 for all colorbar modes** to map the pattern selection bits
2. **Check DE layer registers** when colorbar 8 is active to see how DE generates it
3. **Compare timing registers** between colorbar 1 and colorbar 8
4. **Check if there's a way to adjust TCON pattern origin** (unlikely, but worth checking)

## Register Summary

| Register | Offset | Purpose | Changes with colorbar? |
|----------|--------|---------|----------------------|
| 0x05461000 | 0x00 | TCON Control | No (always 0x80000000) |
| 0x05461004 | 0x04 | TCON Config | No |
| 0x05461040 | 0x40 | **Pattern Control** | **YES** (bit 0 = pattern enable) |
| 0x05461060 | 0x60 | Timing/Control | No |

The key register is **0x05461040** - this controls TCON test pattern generation!
