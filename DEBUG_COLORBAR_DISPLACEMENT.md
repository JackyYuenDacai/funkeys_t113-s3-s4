# Debugging TCON Colorbar Displacement - Step-by-Step Guide

## Goal
Pinpoint why TCON colorbar patterns (1-7) show displacement while DE colorbar (8) works correctly.

## Step 1: Map Pattern Register Values

### Check all colorbar modes
```bash
# In U-Boot, run these commands:
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

**What to look for:**
- Pattern selection bits (likely bits 1-3 or bits 4-6)
- Any other bits that change between modes
- Confirm colorbar 8 has bit 0 = 0 (pattern disabled)

## Step 2: Check TCON Timing Registers

### Read TCON timing/position registers
```bash
# Read all TCON registers to find timing/position registers
=> md.l 0x05461000 0x100  # Read full TCON register space (1KB)

# Focus on these key offsets:
=> md.l 0x05461014 0x10   # Basic timing registers (0x14-0x20)
=> md.l 0x05461024 0x10   # Interface registers (0x24-0x30)
=> md.l 0x05461040 0x10   # Pattern control area (0x40-0x50)
```

**What to look for:**
- Horizontal/Vertical start position registers
- Timing offset registers
- Coordinate origin registers
- Any registers that differ between colorbar 0 and colorbar 1

## Step 3: Compare DE Layer Registers

### Check DE layer configuration
```bash
# DE base address: 0x05000000
# DE layer 0 (BUF layer for framebuffer)
=> md.l 0x05001000 0x100  # DE layer 0 registers

# When colorbar 0 is active (framebuffer)
=> colorbar 0
=> md.l 0x05001000 0x100

# When colorbar 8 is active (DE colorbar)
=> colorbar 8
=> md.l 0x05001000 0x100
```

**What to look for:**
- Layer crop registers (source region)
- Layer frame registers (destination region)
- Layer position/offset registers
- Compare values between colorbar 0 and colorbar 8

## Step 4: Check TCON DSI Interface Registers

### DSI-specific registers
```bash
# DSI base: 0x05450000
=> md.l 0x05450000 0x100  # DSI registers

# Check if DSI has position/offset registers
=> md.l 0x05450000 0x200  # Extended DSI register space
```

**What to look for:**
- DSI video mode start position
- DSI timing offset
- DSI coordinate adjustment registers

## Step 5: Measure the Displacement

### Quantify the displacement
```bash
# Use colorbar patterns to measure displacement
# Colorbar patterns typically have:
# - Vertical stripes (for horizontal displacement measurement)
# - Horizontal stripes (for vertical displacement measurement)

# Measure:
# 1. Horizontal displacement (pixels)
# 2. Vertical displacement (pixels)
# 3. Is it consistent across all TCON patterns (1-7)?
# 4. Does it match any timing parameters?
```

**Expected measurements:**
- Check if displacement matches `hfront-porch`, `hback-porch`, `vfront-porch`, `vback-porch`
- Check if it matches panel timing offsets
- Check if it's a fixed offset or percentage

## Step 6: Check TCON Pattern Generation Code

### Find the driver code
```bash
# In U-Boot source, search for:
grep -r "0x05461040\|TCON.*PATTERN\|colorbar.*tcon" drivers/video/sunxi/disp2/
grep -r "tcon.*pattern\|pattern.*tcon" drivers/video/sunxi/disp2/
```

**What to look for:**
- How TCON patterns are enabled
- If there are position/offset parameters
- How pattern coordinates are calculated
- If patterns use panel timing or fixed coordinates

## Step 7: Compare with Panel Timing Parameters

### Check DTS timing values
From your DTS file:
```
lcd_hbp = <40>;      // hback-porch
lcd_ht = <1148>;     // horizontal total
lcd_hspw = <4>;      // hsync pulse width
lcd_vbp = <12>;      // vback-porch
lcd_vt = <1268>;     // vertical total
lcd_vspw = <4>;      // vsync pulse width
```

**Calculate:**
- Horizontal active start: `hbp + hspw = 40 + 4 = 44 pixels`
- Vertical active start: `vbp + vspw = 12 + 4 = 16 lines`

**Check if displacement matches these values!**

## Step 8: Check TCON Basic Timing Registers

### Read timing registers
```bash
# TCON basic timing registers (typical offsets)
=> md.l 0x05461014 0x4   # BASIC0: htotal, hactive
=> md.l 0x05461018 0x4   # BASIC1: vtotal, vactive
=> md.l 0x0546101C 0x4   # BASIC2: hsync, vsync
=> md.l 0x05461020 0x4   # BASIC3: hbp, vbp
```

**What to look for:**
- Compare register values with DTS timing parameters
- Check if TCON timing matches panel timing
- Look for any offset/adjustment fields

## Step 9: Check for TCON Coordinate Offset Registers

### Search for offset registers
```bash
# Read all TCON registers and look for:
# - Position registers
# - Offset registers
# - Start coordinate registers
# - Window/region registers

=> md.l 0x05461000 0x400  # Read full TCON register map
```

**What to look for:**
- Registers with values that might represent X/Y offsets
- Registers that change when switching colorbar modes
- Registers that might control pattern origin

## Step 10: Analyze the Root Cause

### Possible Causes

1. **TCON Pattern Origin Issue**
   - TCON patterns start at (0,0) in TCON coordinate space
   - Not accounting for panel timing offsets (hbp, vbp)
   - **Fix**: Adjust TCON pattern origin or add offset

2. **Timing Parameter Mismatch**
   - TCON timing registers don't match panel timing
   - TCON uses different timing calculation
   - **Fix**: Ensure TCON timing matches panel timing

3. **DSI Video Mode Offset**
   - DSI video mode has inherent offset
   - TCON patterns don't account for DSI timing
   - **Fix**: Adjust DSI or TCON offset registers

4. **Hardware Limitation**
   - TCON patterns are hardcoded to start at (0,0)
   - No software control over pattern position
   - **Fix**: Use DE colorbar (mode 8) instead, or accept limitation

## Step 11: Test Hypotheses

### Hypothesis 1: Displacement = Timing Offsets
```bash
# If displacement matches hbp+hspw and vbp+vspw:
# Displacement should be: X=44 pixels, Y=16 lines
# Check if this matches what you see on screen
```

### Hypothesis 2: TCON Uses Different Timing
```bash
# Compare TCON timing registers with panel timing
# Check if TCON BASIC registers match DTS values
```

### Hypothesis 3: Pattern Origin is Fixed
```bash
# Check if there are any registers that control pattern origin
# Look for X/Y start position registers in TCON
```

## Step 12: Find the Fix

### Option A: Adjust TCON Pattern Origin (if possible)
- Find TCON pattern origin/offset register
- Set it to match panel timing offsets
- May not be possible if hardware doesn't support it

### Option B: Adjust TCON Timing
- Ensure TCON timing registers match panel timing exactly
- May require recalculating timing parameters

### Option C: Use DE Colorbar Only
- Accept that TCON patterns (1-7) will always show displacement
- Use colorbar 8 (DE colorbar) for accurate testing
- Document this as expected behavior

### Option D: Software Workaround
- Modify colorbar command to use DE colorbar for modes 1-7
- Generate DE colorbar patterns that match TCON patterns
- More complex but provides accurate patterns

## Immediate Actions

### 1. Complete Register Mapping
```bash
# Run this in U-Boot to map all colorbar modes:
for i in 0 1 2 3 4 5 6 7 8; do
  echo "=== Colorbar $i ==="
  colorbar $i
  md.l 0x05461040 0x1
  md.l 0x05461014 0x10
done
```

### 2. Measure Displacement
- Use a ruler or count pixels on screen
- Measure horizontal displacement (pixels)
- Measure vertical displacement (lines)
- Compare with timing parameters

### 3. Check TCON Driver Code
- Locate TCON colorbar implementation
- Check if there are position/offset parameters
- See if patterns can be adjusted

### 4. Compare with Working System
- If you have access to Rockchip implementation
- Compare how they handle TCON patterns
- Check if they have the same displacement issue

## Expected Findings

Based on typical TCON behavior:

1. **TCON patterns start at (0,0)** - hardware limitation
2. **Displacement = timing offsets** - likely matches hbp+hspw, vbp+vspw
3. **No software control** - TCON patterns are hardcoded
4. **DE colorbar works** - because it uses the full pipeline

## Documentation

Document your findings:
- Register values for each colorbar mode
- Measured displacement values
- Timing register values
- Any offset registers found
- Conclusion: hardware limitation or fixable issue
