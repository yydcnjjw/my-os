#include <stdarg.h>
#include <stdint.h>

#include <string.h>
extern double pow(double x, double y);

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s) {
    int i = 0;

    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

#define ZEROPAD 0x01 /* pad with zero */
#define SIGN 0x02    /* unsigned/signed long */
#define PLUS 0x04    /* show plus */
#define SPACE 0x08   /* space if plus */
#define LEFT 0x10    /* left justified */
#define SPECIAL 0x20 /* 0x */
#define SMALL 0x40   /* use 'abcdef' instead of 'ABCDEF' */

#define do_div(n, base)                                                        \
    ({                                                                         \
        uint32_t __base = (base);                                              \
        uint32_t __rem;                                                        \
        __rem = ((uint64_t)(n)) % __base;                                      \
        (n) = ((uint64_t)(n)) / __base;                                        \
        __rem;                                                                 \
    })

static char *number(char *str, unsigned long long num, uint8_t base, int size,
                    int precision, unsigned int type) {
    if (type & LEFT)
        type &= ~ZEROPAD;

    if (base < 2 || base > 36)
        return 0;

    char c = (type & ZEROPAD) ? '0' : ' ';
    char sign = 0;
    if (type & SIGN && (signed long long)num < 0) {
        sign = '-';
        num = -(signed long long)num;
    } else {
        if (type & PLUS)
            sign = '+';
        else if (type & SPACE)
            sign = ' ';
    }

    if (sign)
        size--;

    if (type & SPECIAL) {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }

    int i = 0;
    char tmp[36] = {0};
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (type & SMALL)
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";

    if (num < base)
        tmp[i++] = digits[num];
    else if (base != 10) {
        int mask = base - 1;
        int shift = base == 16 ? 4 : 3;

        do {
            tmp[i++] = digits[((unsigned char)num) & mask];
            num >>= shift;
        } while (num);
    } else {
        do {
            tmp[i++] = digits[do_div(num, base)];
        } while (num);
    }

    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type & (ZEROPAD + LEFT)))
        while (size-- > 0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (type & SPECIAL) {
        if (base == 8)
            *str++ = '0';
        else if (base == 16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;
    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
}

static char *format_float(char *str, double num, int size, int precision,
                          unsigned int type) {
    if (type & LEFT)
        type &= ~ZEROPAD;
    char c = (type & ZEROPAD) ? '0' : ' ';

    char sign = 0;
    if (type & SIGN && num < 0) {
        sign = '-';
        num = -num;
    } else {
        if (type & PLUS)
            sign = '+';
        else if (type & SPACE)
            sign = ' ';
    }

    if (sign)
        size--;

    int ipart = (int)num;
    double fpart = num - (double)ipart;

    int i = 0;
    char tmp[36] = {0};
    const char *digits = "0123456789";

    if (precision == -1) {
        precision = 6;
    }

    fpart = fpart * pow(10, precision);
    do {
        tmp[i++] = digits[do_div(fpart, 10)];
    } while (fpart);

    tmp[i++] = '.';
    
    do {
        tmp[i++] = digits[do_div(ipart, 10)];
    } while (ipart);

    size -= i;

    if (!(type & (ZEROPAD + LEFT)))
        while (size-- > 0)
            *str++ = ' ';

    if (sign)
        *str++ = sign;

    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
}

int vsprintf(char *buf, const char *fmt, va_list args) {
    char *s;
    int *ip;
    char *str;
    unsigned int flags = 0; /* flags to number() */

    for (str = buf; *fmt; ++fmt) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        /* process flags */
        flags = 0;
    repeat:
        ++fmt; /* this also skips first '%' */
        switch (*fmt) {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        /* get field width */
        /* width of output field */
        int field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*') {
            /* it's the next argument */
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        int precision = -1; /* min. # of digits for integers; max
                          number of chars for from string */
        if (*fmt == '.') {
            ++fmt;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*') {
                /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        /* int qualifier = -1; */
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            /* qualifier = *fmt; */
            ++fmt;
        }

        int len;
        int i;
        switch (*fmt) {
        case 'c':
            if (!(flags & LEFT))
                while (--field_width > 0)
                    *str++ = ' ';
            *str++ = (unsigned char)va_arg(args, int);
            while (--field_width > 0)
                *str++ = ' ';
            break;

        case 's':
            s = va_arg(args, char *);
            len = strlen(s);
            if (precision < 0)
                precision = len;
            else if (len > precision)
                len = precision;

            if (!(flags & LEFT))
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; ++i)
                *str++ = *s++;
            while (len < field_width--)
                *str++ = ' ';
            break;

        case 'o':
            str = number(str, va_arg(args, unsigned long), 8, field_width,
                         precision, flags);
            break;

        case 'p':
            if (field_width == -1) {
                field_width = 16;
            }
            flags |= ZEROPAD;
            str = number(str, (unsigned long)va_arg(args, void *), 16,
                         field_width, precision, flags);
            break;

        case 'x':
            flags |= SMALL;
            __attribute__((fallthrough));
        case 'X':
            str = number(str, va_arg(args, unsigned long), 16, field_width,
                         precision, flags);
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
            __attribute__((fallthrough));
        case 'u':
            str = number(str, va_arg(args, unsigned long), 10, field_width,
                         precision, flags);
            break;

        case 'f':
            str = format_float(str, va_arg(args, double), field_width,
                               precision, flags);
            break;
        case 'n':
            ip = va_arg(args, int *);
            *ip = (str - buf);
            break;

        default:
            if (*fmt != '%')
                *str++ = '%';
            if (*fmt)
                *str++ = *fmt;
            else
                --fmt;
            break;
        }
    }
    *str = '\0';
    return str - buf;
}
