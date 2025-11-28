# Contributing to KNOMI 6-Toolhead

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

---

## 🤝 How to Contribute

### Reporting Bugs

**Before submitting:**
1. Check [existing issues](https://github.com/YOUR_USERNAME/KNOMI_6_VORON/issues)
2. Verify you're using the latest version
3. Test with a single display (if multi-display issue)

**Bug report should include:**
- Hardware version (KNOMI V1/V2)
- Firmware version
- Number of displays
- Klipper version
- Serial console logs (115200 baud)
- Steps to reproduce
- Expected vs actual behavior

**Template:**
```markdown
**Hardware:**
- KNOMI Version: V2
- Displays: 6x
- ESP32 Board: ESP32-S3-R8

**Software:**
- Firmware: v3.1.0
- Klipper: v0.12.0
- Moonraker: v0.8.0

**Issue:**
[Description]

**Steps to Reproduce:**
1. Step one
2. Step two
3. ...

**Expected:** [What should happen]
**Actual:** [What actually happens]

**Serial Logs:**
```
[Paste logs here]
```
```

---

## 💡 Suggesting Features

**Before suggesting:**
1. Check [roadmap in FEATURES.md](FEATURES.md#-roadmap)
2. Search existing feature requests
3. Consider if it benefits multi-toolhead setups

**Feature request should include:**
- Clear description
- Use case / motivation
- Proposed implementation (if applicable)
- Impact on existing features

---

## 🔧 Pull Requests

### Process

1. **Fork the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/KNOMI_6_VORON.git
   cd KNOMI_6_VORON
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow code style (see below)
   - Add documentation
   - Test thoroughly

4. **Commit with descriptive message**
   ```bash
   git commit -m "feat: Add adaptive sleep timeouts"
   ```

5. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create Pull Request**
   - Describe changes clearly
   - Reference related issues
   - Include test results

### Commit Message Convention

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
type(scope): subject

body (optional)

footer (optional)
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style (formatting, no logic change)
- `refactor`: Code restructuring (no feature/fix)
- `perf`: Performance improvement
- `test`: Add/update tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(power): Add LED sync mode for display sleep

fix(ui): Resolve progress ring artifacts in temp view

docs(readme): Update installation instructions

refactor(sleep): Simplify wake-up logic
```

---

## 🎨 Code Style

### C/C++ Guidelines

**Formatting:**
- **Indentation:** 4 spaces (no tabs)
- **Line length:** 120 characters max
- **Braces:** K&R style
  ```cpp
  if (condition) {
      // code
  } else {
      // code
  }
  ```

**Naming:**
- **Functions:** `snake_case`
- **Variables:** `snake_case`
- **Constants:** `UPPER_SNAKE_CASE`
- **Types:** `snake_case_t` (with `_t` suffix)
- **Globals:** `g_` prefix

**Example:**
```cpp
#define MAX_DISPLAYS 6

typedef enum {
    SLEEP_MODE_MANUAL,
    SLEEP_MODE_KLIPPER
} sleep_mode_t;

static uint32_t g_last_activity_ms = 0;

void display_enter_sleep(void) {
    // Implementation
}
```

**Comments:**
```cpp
// Single-line comment

/*
 * Multi-line comment
 * with proper formatting
 */

/**
 * @brief Function documentation
 * @param param Description
 * @return Description
 */
```

### Serial Output

**Format:**
```cpp
Serial.println("========================================");
Serial.println("[Category] Message");
Serial.printf("[Category] Value: %d\n", value);
Serial.println("========================================");
```

**Categories:**
- `[Display Sleep]` - Power management
- `[FS]` - Filesystem
- `[Moonraker]` - API
- `[Touch]` - Input
- `[LVGL]` - UI
- `[Init]` - Initialization
- `[Error]` - Errors

---

## 🧪 Testing

### Minimum Testing Requirements

**Before submitting PR:**

1. **Build Test**
   ```bash
   pio run -e knomiv2
   ```

2. **Single Display Test**
   - Upload firmware
   - Verify WiFi connection
   - Test sleep/wake cycle
   - Check serial logs

3. **Multi-Display Test** (if available)
   - Test with 2+ displays
   - Verify synchronized operations
   - Check retry logic

4. **Klipper Integration Test**
   - Connect to Moonraker
   - Start print → verify auto-wake
   - Complete print → verify sleep
   - Test G-code macros

### Test Checklist

```markdown
- [ ] Compiles without errors
- [ ] Compiles without warnings
- [ ] No memory leaks (check serial logs)
- [ ] WiFi connects successfully
- [ ] Display wakes on touch
- [ ] Display sleeps after timeout
- [ ] Klipper integration works
- [ ] Multi-display sync works (if applicable)
- [ ] HTTP API endpoints respond
- [ ] Serial logs are clean
- [ ] Documentation updated
```

---

## 📚 Documentation

### Required Documentation

**For new features:**
1. Update [FEATURES.md](FEATURES.md)
2. Add to [CHANGELOG.md](CHANGELOG.md)
3. Update [README.md](README.md) if needed
4. Add inline code comments
5. Create example code/config if applicable

**For bug fixes:**
1. Add to [CHANGELOG.md](CHANGELOG.md)
2. Update affected documentation
3. Add comments explaining the fix

### Documentation Style

**Markdown:**
- Use headers hierarchically (# → ## → ###)
- Add code blocks with language tags
- Include examples
- Keep paragraphs short (3-4 sentences)

**Code Comments:**
```cpp
// Short explanation for simple code

/*
 * Detailed explanation for complex code:
 * - Point 1
 * - Point 2
 * - Point 3
 */

/**
 * Function documentation (Doxygen style)
 * @brief Brief description
 * @param name Parameter description
 * @return Return value description
 */
```

---

## 🎯 Priority Areas

We especially welcome contributions in:

### High Priority
- **Bug fixes** for multi-display reliability
- **Documentation** improvements (translations, clarity)
- **Testing** on different hardware configurations
- **Performance** optimizations

### Medium Priority
- **New sleep modes** (adaptive, scheduled)
- **UI themes** and customization
- **Additional status screens**
- **Klipper macro** enhancements

### Future Features
- Deep sleep support (ESP32 power-down)
- MQTT integration
- Mobile app companion
- Camera support (time-lapse)

---

## 🚫 What We Don't Accept

- Code that breaks existing functionality
- Features that only work on single displays
- Hardcoded values without configuration options
- Undocumented changes
- Code without testing
- Commits that mix multiple unrelated changes

---

## 📞 Questions?

**Before asking:**
1. Read existing documentation
2. Search closed issues
3. Check [Troubleshooting](FEATURES.md#-tips--tricks)

**How to ask:**
- Open a [Discussion](https://github.com/YOUR_USERNAME/KNOMI_6_VORON/discussions)
- Join [Voron Discord](https://discord.gg/voron) #btt-knomi channel
- Email: [your-email@example.com]

---

## 🙏 Recognition

Contributors will be:
- Listed in [CHANGELOG.md](CHANGELOG.md)
- Credited in release notes
- Mentioned in README (for major contributions)

---

## 📄 License

By contributing, you agree that your contributions will be licensed under the same GPL-3.0 license as the project.

---

**Thank you for making KNOMI better!** 🎉
