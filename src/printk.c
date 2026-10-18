// Source: https://github.com/mit-pdos/xv6-riscv/blob/06aad25c735fd3159bdfae5680be4eab7b1668b2/kernel/printk.c


#include "printk.h"
#include <stdarg.h>
#include "cpu.h"
#include "tty.h"
#include "vga.h"

static char digits[] = "0123456789abcdef";

static void
printint(int xx, int base, int sign)
{
  char buf[20];
  int i;
  unsigned int x;

  if (sign && ((sign = (xx < 0))))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    term_putchar(buf[i]);
}

static void
printptr(uint32_t x)
{
  term_putchar('0');
  term_putchar('x');
  // // for (i = 0; i < (sizeof(uint32_t) * 2); i++, x <<= 4)
  // //   term_putchar(digits[x >> (sizeof(uint32_t) * 4 - 4)]);
  // for (int i = 1; i <= 8; i++) {
  //   uint32_t index = x >> (32-4*i) & 0xF;
  //   // printint(index, 16, 0);
  //   // term_putchar('.');
  //   term_putchar(digits[index]);
  // }
  printint(x, 16, 0);
}

// Print to the console.
int
printk(char *fmt, ...)
{
  va_list ap;
  int i, cx, c2 = 0;
  char *s;

  va_start(ap, fmt);
  for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
    if (cx != '%') {
      term_putchar((char) cx);
      continue;
    }
    i++;
    int c0 = fmt[i + 0] & 0xff;
    int c1 = 0;
    if (c0)
      c1 = fmt[i + 1] & 0xff;
    if (c1)
      c2 = fmt[i + 2] & 0xff;
    if (c0 == 'd') {
      printint(va_arg(ap, int), 10, 1);
    } else if (c0 == 'l' && c1 == 'd') {
      printint(va_arg(ap, uint64_t), 10, 1);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') {
      printint(va_arg(ap, uint64_t), 10, 1);
      i += 2;
    } else if (c0 == 'u') {
      printint(va_arg(ap, uint32_t), 10, 0);
    } else if (c0 == 'l' && c1 == 'u') {
      printint(va_arg(ap, uint64_t), 10, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'u') {
      printint(va_arg(ap, uint64_t), 10, 0);
      i += 2;
    } else if (c0 == 'x') {
      printint(va_arg(ap, uint32_t), 16, 0);
    } else if (c0 == 'l' && c1 == 'x') {
      printint(va_arg(ap, uint32_t), 16, 0);
      i += 1;
    } else if (c0 == 'l' && c1 == 'l' && c2 == 'x') {
      printint(va_arg(ap, uint32_t), 16, 0);
      i += 2;
    } else if (c0 == 'p') {
      printptr(va_arg(ap, uint32_t));
    } else if (c0 == 'c') {
      term_putchar(va_arg(ap, uint32_t));
    } else if (c0 == 's') {
      if ((s = va_arg(ap, char *)) == 0)
        s = "(null)";
      for (; *s; s++)
        term_putchar(*s);
    } else if (c0 == '%') {
      term_putchar('%');
    } else if (c0 == 0) {
      break;
    } else {
      // Print unknown % sequence to draw attention.
      term_putchar('%');
      term_putchar((char) c0);
    }
  }
  va_end(ap);

  return 0;
}

void
panic(char *str) {
  int_off();
  // term_clear(0);
  term_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
  printk("kernel panic:\n\r");
  printk("%s", str);
  for (;;)
    ;
}