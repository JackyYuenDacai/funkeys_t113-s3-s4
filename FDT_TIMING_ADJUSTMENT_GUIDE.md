# FDT Timing Adjustment Guide

## Current Issue

You're modifying timing parameters in U-Boot's FDT, but there are a few issues:

### 1. Value Format
You're setting values as **strings** (`"40"`, `"1148"`) when they should be **integers** (`<40>`, `<1148>`).

**Current (wrong):**
```bash
=> fdt set /soc/lcd0 lcd_hbp "40"    # String format
```

**Should be:**
```bash
=> fdt set /soc/lcd0 lcd_hbp <40>    # Integer format
```

### 2. FDT Changes Need to be Applied

FDT changes in U-Boot are **in-memory only**. To take effect:
- The display driver must **re-read the FDT** (usually happens on display re-initialization)
- Or you need to **reload/restart the display**

### 3. Your Change Analysis

You changed:
- `lcd_vbp`: 12 → 20 (increased by 8)

**Original DTS values:**
```
lcd_hbp = 40
lcd_ht = 1148
lcd_hspw = 4
lcd_vbp = 12    ← You changed this to 20
lcd_vt = 1268
lcd_vspw = 4
```

**Why increase vbp by 8?**
- If displacement is 8 lines vertically, increasing vbp by 8 might compensate
- But this changes the **total vertical timing**, not just the pattern position

## Correct Way to Modify FDT

### Step 1: Set Values as Integers
```bash
# Correct format (integers, not strings)
=> fdt set /soc/lcd0 lcd_hbp <40>
=> fdt set /soc/lcd0 lcd_ht <1148>
=> fdt set /soc/lcd0 lcd_hspw <4>
=> fdt set /soc/lcd0 lcd_vbp <20>    # Your test value
=> fdt set /soc/lcd0 lcd_vt <1268>
=> fdt set /soc/lcd0 lcd_vspw <4>
```

### Step 2: Verify Changes
```bash
=> fdt print /soc/lcd0
```

**Check the format:**
- **Wrong**: `lcd_vbp = "20";` (string)
- **Right**: `lcd_vbp = <0x00000014>;` (integer, hex format)

### Step 3: Apply Changes

**Option A: Re-initialize Display**
```bash
# Disable and re-enable display to reload FDT
=> disp
# (Note: U-Boot may not have a direct command to re-init display)
```

**Option B: Reboot**
```bash
# Reboot to apply FDT changes
=> reset
```

**Option C: Check if display auto-reloads**
- Some drivers read FDT on every enable
- Try: `colorbar 0` then `colorbar 1` to see if timing changes

## Will This Fix Displacement?

### Likely Answer: **No, for these reasons:**

1. **TCON patterns are hardware-generated**
   - They use TCON's internal timing, not FDT values
   - Changing FDT won't affect TCON pattern origin

2. **FDT timing affects DE/panel timing**
   - Used for normal display operation
   - TCON patterns bypass this pipeline

3. **Changing vbp changes total timing**
   - `lcd_vt = vactive + vfp + vbp + vspw`
   - Increasing vbp by 8 changes the **total frame time**
   - This might cause other issues (sync problems, frame rate changes)

## Better Approach: Test TCON Register Directly

Instead of changing FDT, try modifying TCON registers directly:

### Step 1: Find TCON Timing Registers
```bash
# Read TCON basic timing registers
=> md.l 0x05461014 0x10  # BASIC0-BASIC3
```

### Step 2: Try Adjusting TCON Registers
```bash
# WARNING: This is experimental - may break display!
# Read current value first
=> md.l 0x05461020 0x1   # BASIC3 (likely has vbp)

# Try adjusting (example - adjust based on actual register layout)
# => mw.l 0x05461020 <new_value>
```

**Note:** This is risky - you need to know the exact register layout first!

## Recommended Next Steps

### 1. Fix FDT Format First
```bash
# Set as integers
=> fdt set /soc/lcd0 lcd_vbp <20>
=> fdt print /soc/lcd0  # Verify it shows <0x00000014> not "20"
```

### 2. Test if Changes Take Effect
```bash
# After setting FDT values correctly:
=> colorbar 0
=> colorbar 1
# Check if displacement changed
```

### 3. Measure Displacement Before/After
- Measure displacement with original vbp=12
- Measure displacement with new vbp=20
- See if it changes

### 4. Check TCON Registers
```bash
# Read TCON timing registers to see if they match FDT
=> md.l 0x05461014 0x10
# Compare with your FDT values
```

## Understanding the Timing Parameters

### What Each Parameter Means

```
lcd_hbp = 40    # Horizontal back porch (pixels after active area)
lcd_ht = 1148   # Horizontal total (active + porches + sync)
lcd_hspw = 4    # Horizontal sync pulse width
lcd_vbp = 12    # Vertical back porch (lines after active area)
lcd_vt = 1268   # Vertical total (active + porches + sync)
lcd_vspw = 4    # Vertical sync pulse width
```

### Timing Calculation
```
ht = hactive + hfp + hbp + hspw
vt = vactive + vfp + vbp + vspw

For your panel:
ht = 1080 + 28 + 40 + 4 = 1152? (but DTS says 1148)
vt = 1240 + 16 + 12 + 4 = 1272? (but DTS says 1268)
```

**Note:** The DTS values might already account for some adjustments.

## Expected Outcome

### If Changing vbp Helps:
- Displacement might shift by the amount you changed
- But this is a **workaround**, not a fix
- May cause other timing issues

### If It Doesn't Help:
- Confirms TCON patterns use their own timing
- Need to look at TCON register adjustments instead
- Or accept that TCON patterns will always show displacement

## Alternative: Use DE Colorbar

Since colorbar 8 (DE colorbar) works correctly:
- Use it for display testing instead of TCON patterns
- Or modify the colorbar command to use DE for modes 1-7

## Summary

1. **Fix FDT format**: Use `<value>` not `"value"`
2. **Test if changes take effect**: Reboot or re-init display
3. **Measure displacement**: Before and after changes
4. **Check TCON registers**: See if they match FDT values
5. **Consider alternatives**: TCON register adjustment or use DE colorbar

The displacement is likely a **hardware limitation** of TCON patterns, not something fixable via FDT timing changes.
