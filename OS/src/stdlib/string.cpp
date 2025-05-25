#include <kernel/mem/mem.h>
#include <stdlib/assert.h>
#include <stdlib/math.h>
#include <types.h>
#include <stdlib/string.h>
namespace stdlib
{

    size_t strlen(const char *src)
    {
        size_t length = 0;
        while (*src++ != '\0')
        {
            length++;
        }
        return length;
    }

    size_t strclen(const char *src, char term)
    {
        size_t length = 0;
        while (*src != '\0' && *src != term)
        {
            length++;
            src++;
        }
        return length;
    }

    const char *g_HexChars = "0123456789ABCDEF";
    static char itoaOutput[64];
    char *utoa(uint64_t val, uint8_t radix)
    {

        char buffer[64];

        for (int i = 0; i < 64; i++)
        {
            itoaOutput[i] = 0;
        }

        int pos = 0;
        do
        {
            uint64_t remainder = val % radix;
            val /= radix;
            buffer[pos++] = g_HexChars[remainder];
        } while (val > 0);

        int _pos = 0;
        while (--pos >= 0)
        {
            itoaOutput[_pos++] = buffer[pos];
        }

        return itoaOutput;
    }

    // For unsigned integers
    uint64_t atou(const char *str, uint64_t len)
    {
        uint64_t num = 0;

        for (uint64_t i = 0; i < len; i++)
        {
            num = num * 10 + (str[i] - 48);
        }

        return num;
    }

    // TODO: Doesn't work
    char *itoa(int64_t val, uint8_t radix)
    {
        for (int i = 0; i < 64; i++)
        {
            itoaOutput[i] = 0;
        }

        char buffer[64];
        bool negative = false;
        if (val < 0)
        {
            negative = true;
            val *= -1;
        }

        int pos = 0;
        do
        {
            uint64_t remainder = val % radix;
            val /= radix;
            buffer[pos++] = g_HexChars[remainder];
        } while (val > 0);

        int _pos = 0;
        if (negative)
        {
            itoaOutput[_pos++] = '-';
        }
        while (--pos >= 0)
        {
            itoaOutput[_pos++] = buffer[pos];
        }
        return itoaOutput;
    }

    // Remember to delete the value!
    char *dtoa(double number, int dec_cnt)
    {
        bool negative = false;

        if (number < 0)
        {
            negative = true;
            number = -number;
        }

        int intPart = static_cast<int>(number);
        double fractionalPart = number - intPart;

        const int bufferSize = 64;
        char *result = new char[bufferSize];
        char *current = result;

        // Convert the integer part to string
        if (intPart == 0)
        {
            *current++ = '0';
        }
        else
        {
            while (intPart != 0)
            {
                *current++ = '0' + intPart % 10;
                intPart /= 10;
            }
        }

        // Reverse the integer part
        char *start = result;
        char *end = current - 1;
        while (start < end)
        {
            char temp = *start;
            *start++ = *end;
            *end-- = temp;
        }

        // Add the decimal point if there is a fractional part
        if (fractionalPart > 0)
        {
            *current++ = '.';

            for (int i = 0; i < dec_cnt; i++)
            {
                fractionalPart *= 10;
                int digit = static_cast<int>(fractionalPart);
                *current++ = '0' + digit;
                fractionalPart -= digit;
            }
        }
        else
        {
            *current++ = '.';
            for (int i = 0; i < dec_cnt; i++)
            {
                *current++ = '0';
            }
        }

        // Add the null terminator
        *current = '\0';

        // Add negative sign if necessary
        if (negative)
        {
            char *temp = new char[current - result + 2];
            *temp = '-';
            kernel::memcpy(temp + 1, result, current - result + 1);
            delete[] result;
            result = temp;
        }

        return result;
    }

    char toUpper(char c)
    {
        if (c >= 97)
        {
            return c - 32;
        }

        return c;
    }

    bool strcmp(const char *a, const char *b, int count)
    {
        for (int i = 0; i < count; i++)
        {
            if (a[i] != b[i])
                return false;
        }

        return true;
    }

    bool strcmp(const char *a, const char *b)
    {
        size_t len = strlen(a);
        if (len != strlen(b))
            return false;
        return strcmp(a, b, len);
    }

    const char *strcat(char *dest, const char *src)
    {
        char *_dest = dest;
        dest += strlen(dest);
        strcpy(dest, src);
        return _dest;
    }

    const char *strcpy(char *_dest, const char *_src)
    {
        char *dest = _dest;
        while (*_src != '\0')
        {
            *dest = *_src;
            dest++;
            _src++;
        }
        return _dest;
    }

}