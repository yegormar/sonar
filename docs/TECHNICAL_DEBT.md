# Technical Debt

## Known Design Flaws or Compromises

1. **Hardcoded Calibration Constants**:
   - **Issue**: Default calibration constants are hardcoded in `sonar_main.ino` and `sonar2.ino`.
   - **Impact**: May not work optimally for all environments.
   - **Priority**: High

2. **Inconsistent Angle Sign Conventions**:
   - **Issue**: `sonar_main.ino` uses `+60 = LEFT`, while `sonar2.ino` uses `+ = LEFT`.
   - **Impact**: Potential confusion in maintenance.
   - **Priority**: Medium

3. **Duplicate Code**:
   - **Issue**: Sensor reading and feedback logic are duplicated across sketches.
   - **Impact**: Increases maintenance overhead.
   - **Priority**: Medium

4. **Limited Error Handling**:
   - **Issue**: Error handling for sensor failures is minimal.
   - **Impact**: System may behave unpredictably in edge cases.
   - **Priority**: Low

## Acceptable vs Unacceptable Debt

### Acceptable Debt
- Hardcoded calibration constants (temporary for prototyping).
- Duplicate code (temporary for experimentation).

### Unacceptable Debt
- Inconsistent angle sign conventions (must be resolved).
- Limited error handling (must be improved for production use).

## Priority Order for Addressing Debt

1. **High Priority**:
   - Resolve hardcoded calibration constants.
   - Standardize angle sign conventions.

2. **Medium Priority**:
   - Refactor duplicate code into shared modules.

3. **Low Priority**:
   - Improve error handling for sensor failures.
