CAL_NEAR = 59.92
CAL_FAR = 171.00
CAL_DIFF_LEFT = -85.01
CAL_DIFF_RIGHT = 70.49

MAX_DIFF_MAGNITUDE = max(abs(CAL_DIFF_LEFT), abs(CAL_DIFF_RIGHT))
SYMMETRICAL_LEFT = -MAX_DIFF_MAGNITUDE
SYMMETRICAL_RIGHT = MAX_DIFF_MAGNITUDE

def mapFloat(x, in_min, in_max, out_min, out_max):
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

def test_scenario(L, R, label):
    diff = L - R
    normalizedDiff = mapFloat(diff, SYMMETRICAL_LEFT, SYMMETRICAL_RIGHT, -1.0, 1.0)
    normalizedDiff = max(-1.0, min(1.0, normalizedDiff))
    angleDeg = normalizedDiff * -60.0
    
    proximity = min(L, R)
    # Fixed dead zone = 15 degrees
    dynamicDeadZone = 15.0
    
    freq = int(mapFloat(angleDeg, -60, 60, 200, 2000))
    
    if angleDeg > dynamicDeadZone:
        led = "BLUE (LEFT)"
    elif angleDeg < -dynamicDeadZone:
        led = "RED (RIGHT)"
    else:
        led = "BOTH (CENTER)"
    
    print(f"{label}:")
    print(f"  L={L:.0f}cm  R={R:.0f}cm  →  diff={diff:+.1f}  angle={angleDeg:+.1f}°  dead_zone={dynamicDeadZone:.0f}°")
    print(f"  LED: {led}   Freq: {freq}Hz")
    print()

print("=" * 60)
print("SONAR MAPPING VALIDATION TOOL (with FIXED 15° dead zone)")
print(f"CAL_NEAR={CAL_NEAR}, CAL_FAR={CAL_FAR}")
print(f"SYM_LEFT={SYMMETRICAL_LEFT}, SYM_RIGHT={SYMMETRICAL_RIGHT}")
print("=" * 60)
print()

# Test person on left center side
test_scenario(50, 100, "LEFT side (L=50, R=100)")

# Test person on right center side
test_scenario(100, 50, "RIGHT side (R=50, L=100)")

# Test person in center
test_scenario(60, 60, "CENTER (L=60, R=60)")
test_scenario(100, 100, "CENTER far (L=100, R=100)")

# Test near threshold
test_scenario(30, 30, "VERY CLOSE center (L=30, R=30)")

# Test left but one sensor sees wall (should still work)
test_scenario(30, 170, "LEFT close, far right reading (L=30, R=170)")
test_scenario(170, 30, "RIGHT close, far left reading (L=170, R=30)")
