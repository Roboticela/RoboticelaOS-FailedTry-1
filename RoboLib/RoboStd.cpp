#include <stddef.h>
#include <stdint.h>

extern "C" void* memcpy(void*, const void*, size_t);
extern "C" void* memset(void*, int, size_t);

extern "C" size_t strlen(const char* text) {
    size_t length = 0;
    if (text == nullptr) {
        return 0;
    }
    while (text[length] != 0) {
        length += 1;
    }
    return length;
}

extern "C" int strcmp(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return left == right ? 0 : (left == nullptr ? -1 : 1);
    }
    while (*left != 0 && *left == *right) {
        left += 1;
        right += 1;
    }
    return static_cast<unsigned char>(*left) - static_cast<unsigned char>(*right);
}

extern "C" int memcmp(const void* left, const void* right, size_t count) {
    const uint8_t* a = static_cast<const uint8_t*>(left);
    const uint8_t* b = static_cast<const uint8_t*>(right);
    for (size_t index = 0; index < count; ++index) {
        if (a[index] != b[index]) {
            return static_cast<int>(a[index]) - static_cast<int>(b[index]);
        }
    }
    return 0;
}

extern "C" void abort(void) {
    for (;;) {
    }
}

extern "C" int abs(int value) {
    return value < 0 ? -value : value;
}

extern "C" int atoi(const char* text) {
    if (text == nullptr) {
        return 0;
    }
    int sign = 1;
    while (*text == ' ' || *text == '\t') {
        text += 1;
    }
    if (*text == '-') {
        sign = -1;
        text += 1;
    } else if (*text == '+') {
        text += 1;
    }
    int value = 0;
    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        text += 1;
    }
    return value * sign;
}

extern "C" double floor(double value) {
    if (value >= 0.0) {
        return static_cast<double>(static_cast<long long>(value));
    }
    double whole = static_cast<double>(static_cast<long long>(value));
    return whole == value ? value : whole - 1.0;
}

extern "C" double ceil(double value) {
    double floored = floor(value);
    return floored == value ? value : floored + 1.0;
}

extern "C" float floorf(float value) {
    return static_cast<float>(floor(static_cast<double>(value)));
}

extern "C" float ceilf(float value) {
    return static_cast<float>(ceil(static_cast<double>(value)));
}

extern "C" double fabs(double value) {
    return value < 0.0 ? -value : value;
}

extern "C" float fabsf(float value) {
    return value < 0.0f ? -value : value;
}

extern "C" double sqrt(double value) {
    if (value <= 0.0) {
        return 0.0;
    }
    double guess = value;
    for (int step = 0; step < 16; ++step) {
        guess = (guess + value / guess) * 0.5;
    }
    return guess;
}

extern "C" float sqrtf(float value) {
    return static_cast<float>(sqrt(static_cast<double>(value)));
}

extern "C" double fmod(double value, double divisor) {
    if (divisor == 0.0) {
        return 0.0;
    }
    return value - divisor * floor(value / divisor);
}

extern "C" double cos(double value) {
    const double pi = 3.141592653589793;
    while (value > pi) {
        value -= 2.0 * pi;
    }
    while (value < -pi) {
        value += 2.0 * pi;
    }
    double term = 1.0;
    double sum = 1.0;
    double square = value * value;
    for (int n = 1; n < 10; ++n) {
        term *= -square / static_cast<double>((2 * n - 1) * (2 * n));
        sum += term;
    }
    return sum;
}

extern "C" double acos(double value) {
    if (value < -1.0) {
        value = -1.0;
    }
    if (value > 1.0) {
        value = 1.0;
    }
    int negative = value < 0.0;
    value = fabs(value);
    double result = -0.0187293;
    result = result * value + 0.0742610;
    result = result * value - 0.2121144;
    result = result * value + 1.5707288;
    result *= sqrt(1.0 - value);
    return negative ? 3.141592653589793 - result : result;
}

extern "C" double pow(double base, double exponent) {
    if (exponent == 0.0) {
        return 1.0;
    }
    if (base == 0.0) {
        return 0.0;
    }
    if (exponent == static_cast<double>(static_cast<int>(exponent)) && exponent > -32.0 && exponent < 32.0) {
        int power = static_cast<int>(exponent);
        if (power < 0) {
            base = 1.0 / base;
            power = -power;
        }
        double result = 1.0;
        while (power > 0) {
            if ((power & 1) != 0) {
                result *= base;
            }
            base *= base;
            power >>= 1;
        }
        return result;
    }
    if (base < 0.0) {
        return 0.0;
    }
    int shift = 0;
    double reduced = base;
    while (reduced >= 2.0) {
        reduced *= 0.5;
        shift += 1;
    }
    while (reduced < 1.0) {
        reduced *= 2.0;
        shift -= 1;
    }
    const double ratio = (reduced - 1.0) / (reduced + 1.0);
    const double ratioSquare = ratio * ratio;
    double logTerm = ratio;
    double logSum = 0.0;
    for (int step = 0; step < 16; ++step) {
        logSum += logTerm / static_cast<double>(2 * step + 1);
        logTerm *= ratioSquare;
    }
    const double naturalLog = 2.0 * logSum + static_cast<double>(shift) * 0.6931471805599453;
    const double scaled = exponent * naturalLog;
    const double whole = floor(scaled + (scaled >= 0.0 ? 0.5 : -0.5));
    const double remainder = scaled - whole;
    double term = 1.0;
    double exponential = 1.0;
    for (int step = 1; step < 20; ++step) {
        term *= remainder / static_cast<double>(step);
        exponential += term;
    }
    int power = static_cast<int>(whole);
    double scale = 1.0;
    double baseE = 2.718281828459045;
    if (power < 0) {
        baseE = 1.0 / baseE;
        power = -power;
    }
    while (power > 0) {
        if ((power & 1) != 0) {
            scale *= baseE;
        }
        baseE *= baseE;
        power >>= 1;
    }
    return exponential * scale;
}

extern "C" long lround(double value) {
    return static_cast<long>(value >= 0.0 ? floor(value + 0.5) : ceil(value - 0.5));
}

extern "C" long lroundf(float value) {
    return lround(static_cast<double>(value));
}

namespace {

void WriteSigned(char*& cursor, char* end, long long value) {
    if (cursor >= end) {
        return;
    }
    if (value < 0) {
        *cursor++ = '-';
        value = -value;
    }
    char digits[32];
    int count = 0;
    do {
        digits[count++] = static_cast<char>('0' + (value % 10));
        value /= 10;
    } while (value > 0 && count < 32);
    while (count > 0 && cursor < end) {
        *cursor++ = digits[--count];
    }
}

void WriteUnsigned(char*& cursor, char* end, unsigned long long value, int base, int width, char fill, bool upper) {
    char digits[32];
    int count = 0;
    const char* alphabet = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
        digits[count++] = alphabet[value % static_cast<unsigned>(base)];
        value /= static_cast<unsigned>(base);
    } while (value > 0 && count < 32);
    while (count < width && cursor < end) {
        *cursor++ = fill;
        width -= 1;
    }
    while (count > 0 && cursor < end) {
        *cursor++ = digits[--count];
    }
}

}  // namespace

extern "C" int vsnprintf(char* buffer, size_t capacity, const char* format, __builtin_va_list args) {
    if (buffer == nullptr || capacity == 0 || format == nullptr) {
        return 0;
    }
    char* cursor = buffer;
    char* end = buffer + capacity - 1;
    while (*format != 0 && cursor < end) {
        if (*format != '%') {
            *cursor++ = *format++;
            continue;
        }
        format += 1;
        char fill = ' ';
        int width = 0;
        int precision = -1;
        if (*format == '0') {
            fill = '0';
            format += 1;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format += 1;
        }
        if (*format == '.') {
            format += 1;
            precision = 0;
            while (*format >= '0' && *format <= '9') {
                precision = precision * 10 + (*format - '0');
                format += 1;
            }
        }
        if (*format == 's') {
            const char* text = __builtin_va_arg(args, const char*);
            if (text == nullptr) {
                text = "";
            }
            while (*text != 0 && cursor < end) {
                *cursor++ = *text++;
            }
        } else if (*format == 'd' || *format == 'i') {
            WriteSigned(cursor, end, __builtin_va_arg(args, int));
        } else if (*format == 'u') {
            WriteUnsigned(cursor, end, __builtin_va_arg(args, unsigned), 10, width, fill, false);
        } else if (*format == 'x' || *format == 'X') {
            WriteUnsigned(cursor, end, __builtin_va_arg(args, unsigned), 16, width, fill, *format == 'X');
        } else if (*format == 'f') {
            double value = __builtin_va_arg(args, double);
            if (precision < 0) {
                precision = 6;
            }
            if (value < 0.0) {
                if (cursor < end) {
                    *cursor++ = '-';
                }
                value = -value;
            }
            double scale = 1.0;
            for (int digit = 0; digit < precision; ++digit) {
                scale *= 10.0;
            }
            double rounded = floor(value * scale + 0.5) / scale;
            WriteSigned(cursor, end, static_cast<long long>(floor(rounded)));
            if (precision > 0 && cursor < end) {
                *cursor++ = '.';
                double fraction = rounded - floor(rounded);
                for (int digit = 0; digit < precision && cursor < end; ++digit) {
                    fraction *= 10.0;
                    int piece = static_cast<int>(fraction);
                    *cursor++ = static_cast<char>('0' + piece);
                    fraction -= piece;
                }
            }
        } else if (*format == 'c') {
            if (cursor < end) {
                *cursor++ = static_cast<char>(__builtin_va_arg(args, int));
            }
        } else if (*format != 0) {
            if (cursor < end) {
                *cursor++ = *format;
            }
        }
        if (*format != 0) {
            format += 1;
        }
    }
    *cursor = 0;
    return static_cast<int>(cursor - buffer);
}

extern "C" int snprintf(char* buffer, size_t capacity, const char* format, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, format);
    int written = vsnprintf(buffer, capacity, format, args);
    __builtin_va_end(args);
    return written;
}

namespace std {

int snprintf(char* buffer, size_t capacity, const char* format, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, format);
    int written = ::vsnprintf(buffer, capacity, format, args);
    __builtin_va_end(args);
    return written;
}

}  // namespace std
