# TCON Register Addresses for T113 (sun8iw20p1)

## Correct Base Addresses

From `sun8iw20p1-soc-system.dtsi`:
```
disp: disp@0x5000000 {
    reg = <0x0 0x05000000 0x0 0x3fffff>,    /* de0 */
          <0x0 0x05460000 0x0 0xfff>,        /* display_if_top */
          <0x0 0x05461000 0x0 0xfff>,        /* tcon-lcd0 */
          <0x0 0x05470000 0x0 0xfff>,        /* tcon-tv */
          <0x0 0x05450000 0x0 0x1fff>;       /* dsi0 */
```

**Key Addresses:**
- **TCON-LCD0**: `0x05461000` (this is what you need!)
- **DE (Display Engine)**: `0x05000000`
- **DSI0**: `0x05450000`
- **Display IF Top**: `0x05460000`

## What You Were Reading

`0x06000000` is the **DE memory region** (framebuffer memory), not registers. That's why you saw code/data instead of register values.

## Reading TCON Registers

### TCON Control Registers (offset from 0x05461000)

Typical TCON register offsets (exact offsets depend on T113 TCON version):

```
TCON_CTL_REG          = 0x0000  // TCON Control Register
TCON_IO_POL_REG       = 0x0004  // IO Polarity Register (may control colorbar)
TCON_INT_REG          = 0x0008  // Interrupt Register
TCON_LVDS_IF_REG      = 0x000C  // LVDS Interface Register
TCON_DSI_IF_REG       = 0x0010  // DSI Interface Register
TCON_TCON0_BASIC0_REG = 0x0014  // Basic Timing 0
TCON_TCON0_BASIC1_REG = 0x0018  // Basic Timing 1
TCON_TCON0_BASIC2_REG = 0x001C  // Basic Timing 2
TCON_TCON0_BASIC3_REG = 0x0020  // Basic Timing 3
TCON_TCON0_HV_IF_REG  = 0x0024  // HV Interface Register
TCON_TCON0_CPU_IF_REG = 0x0028  // CPU Interface Register
TCON_TCON0_IO_TRI_REG = 0x002C  // IO Tri-state Register
TCON_TCON0_IO_POL_REG = 0x0030  // IO Polarity Register (colorbar control here!)
TCON_TCON0_IO_CTL_REG = 0x0034  // IO Control Register
TCON_TCON0_IO_TAT_REG = 0x0038  // IO Test/AT Register
TCON_TCON0_IO_CU_REG  = 0x003C  // IO CU Register
TCON_TCON0_TTL0_REG   = 0x0040  // TTL 0 Register
TCON_TCON0_TTL1_REG   = 0x0044  // TTL 1 Register
TCON_TCON0_TTL2_REG   = 0x0048  // TTL 2 Register
TCON_TCON0_TTL3_REG   = 0x004C  // TTL 3 Register
TCON_TCON0_TTL4_REG   = 0x0050  // TTL 4 Register
TCON_TCON0_LVDS0_REG  = 0x0054  // LVDS 0 Register
TCON_TCON0_LVDS1_REG  = 0x0058  // LVDS 1 Register
TCON_TCON0_LVDS2_REG  = 0x005C  // LVDS 2 Register
TCON_TCON0_LVDS3_REG  = 0x0060  // LVDS 3 Register
TCON_TCON0_LVDS_CLK_REG = 0x0064  // LVDS Clock Register
TCON_TCON0_IO_POL_REG2 = 0x0068  // IO Polarity Register 2 (may have colorbar bits)
```

### Colorbar Control Bits

The colorbar pattern selection is typically in:
- `TCON_TCON0_IO_POL_REG` (0x05461030) - bits for pattern selection
- Or a dedicated colorbar register

**Typical bit fields:**
- Bit 0-2: Pattern mode (0-7)
- Bit 3: Enable colorbar/test pattern
- Bit 4-7: Pattern type

## Commands to Read TCON Registers

### Read TCON Control Register
```bash
=> md.l 0x05461000 0x40  # Read first 64 registers (256 bytes)
```

### Read IO Polarity Register (likely has colorbar bits)
```bash
=> md.l 0x05461030 0x1   # Read IO_POL_REG
```

### Read All TCON Registers (first 1KB)
```bash
=> md.l 0x05461000 0x100  # Read 1KB of TCON registers
```

## Reading DE Registers

### DE Base Address
```bash
=> md.l 0x05000000 0x100  # Read DE registers
```

### DE Layer Registers (for colorbar 8)
DE layers are typically at:
- Layer 0: `0x05001000`
- Layer 1: `0x05002000`
- Layer 2: `0x05003000`
- etc.

## Understanding the Register Values

### Typical Register Format
- **0x00000000**: All zeros (disabled/reset state)
- **0xFFFFFFFF**: All ones (all features enabled)
- **0x00000001**: Bit 0 set (pattern enabled)
- **0x00000007**: Bits 0-2 set (pattern mode 7)

### What to Look For

When `colorbar 1` is active, you should see:
- TCON_TCON0_IO_POL_REG: `0x0000000X` (where X is pattern mode)
- TCON_CTL_REG: May have enable bits set

When `colorbar 8` is active:
- DE layer registers will have colorbar configuration
- TCON registers may show normal operation (not test pattern mode)

## Debugging Colorbar Displacement

### Check TCON Timing Registers
```bash
# Basic timing registers
=> md.l 0x05461014 0x10  # Read BASIC0-BASIC3 (timing parameters)
```

### Check DE Layer Configuration
```bash
# DE layer 0 (BUF layer for framebuffer)
=> md.l 0x05001000 0x100  # Read DE layer 0 registers
```

### Compare colorbar 1 vs colorbar 8

**Before colorbar 1:**
```bash
=> md.l 0x05461030 0x1   # Save IO_POL_REG value
```

**After colorbar 1:**
```bash
=> md.l 0x05461030 0x1   # Check if it changed
```

**After colorbar 8:**
```bash
=> md.l 0x05461030 0x1   # Should be different from colorbar 1
```

## Expected Behavior

**Colorbar 1-7 (TCON patterns):**
- TCON_TCON0_IO_POL_REG will have pattern bits set
- TCON generates pattern internally
- Bypasses DE layer settings
- May show displacement due to TCON's internal coordinate system

**Colorbar 8 (DE colorbar):**
- DE layer registers configured for colorbar
- TCON_TCON0_IO_POL_REG in normal mode (no test pattern)
- Uses DE layer crop/frame settings
- Should align correctly

## Notes

1. **Register offsets may vary** - T113 might have different TCON register layout than older Allwinner SoCs
2. **Check U-Boot source** - Look for `TCON_*_REG` definitions in the driver code
3. **Memory protection** - Some registers may be read-only or protected
4. **Register documentation** - Allwinner T113 datasheet would have exact register definitions

## Next Steps

1. Read TCON registers at `0x05461000`
2. Compare register values between colorbar modes
3. Look for pattern selection bits
4. Check if TCON timing registers match panel timing
