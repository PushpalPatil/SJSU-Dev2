#include "newlib/newlib.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>

#include "L0_LowLevel/ram.hpp"
#include "L1_Drivers/uart.hpp"
#include "utility/macros.hpp"

#if defined(HOST_TEST)
int HostWrite(int c)
{
  int payload = c;
  return static_cast<int>(write(1, &payload, 1));
}
int HostRead()
{
  int result = 0;
  read(1, &result, 1);
  return result;
}
Stdout out = HostWrite;
Stdin in   = HostRead;
#else
int FirmwareStdOut(int data)
{
  uart0.Send(static_cast<uint8_t>(data));
  return 1;
}
int FirmwareStdIn()
{
  return static_cast<int>(uart0.Receive());
}
Stdout out = FirmwareStdOut;
Stdin in   = FirmwareStdIn;
#endif

extern "C"
{
  // Dummy implementation of isatty
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _isatty([[maybe_unused]] int file)
  {
    return 1;
  }
  // Dummy implementation of exit with return code placed into
  // Arm register r3
  // NOLINTNEXTLINE(readability-identifier-naming)
  void _exit([[maybe_unused]] int rc)
  {
    while (1)
    {
      continue;
    }
  }
  // Dummy implementation of getpid
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _getpid()
  {
    return 1;
  }
  // Dummy implementation of kill
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _kill(int, int)
  {
    return -1;
  }
  // Dummy implementation of fstat, makes the assumption that the "device"
  // representing, in this case STDIN, STDOUT, and STDERR as character devices.
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _fstat([[maybe_unused]] int file, struct stat * status)
  {
    status->st_mode = S_IFCHR;
    return 0;
  }
  // Implementation of sbrk for allocating and deallocating space for malloc.
  // NOLINTNEXTLINE(readability-identifier-naming)
  void * _sbrk(int increment)
  {
    void * previous_heap_position = static_cast<void *>(heap_position);
    // Check that by allocating this space, we do not exceed the heap area.
    if ((heap_position + increment) > &heap_end)
    {
      previous_heap_position = nullptr;
    }
    heap_position += increment;
    return previous_heap_position;
  }
  // Dummy implementation of close
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _close([[maybe_unused]] int file)
  {
    return -1;
  }
  // Minimum implementation of _write using UART0 putchar
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _write([[maybe_unused]] int file, char * ptr, int length)
  {
    for (int i = 0; i < length; i++)
    {
      // TODO(#81): either make this inline, or swap with function that can
      //   take a buffer and length.
      out(ptr[i]);
    }
    return length;
  }
  // Dummy implementation of _lseek
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _lseek([[maybe_unused]] int file, [[maybe_unused]] int ptr,
             [[maybe_unused]] int dir)
  {
    return 0;
  }
  // Minimum implementation of _read using UART0 getchar
  // NOLINTNEXTLINE(readability-identifier-naming)
  int _read(FILE * file, char * ptr, int length)
  {
    if (file == STDIN_FILENO)
    {
      length = 1;
      // TODO(#81): either make this inline
      *ptr = static_cast<char>(in());
      if (*ptr == '\r')
      {
        out('\r');
        *ptr = '\n';
      }
      out(*ptr);
    }
    return length;
  }
  // Needed by third party printf library
  void _putchar(char character)  // NOLINT
  {
    out(character);
  }

  // Overload default libnano putchar() with a more optimal version that does
  // not use dynamic memory
  int putchar(int character)  // NOLINT
  {
    return out(character);
  }

  // Overload default libnano puts() with a more optimal version that does
  // not use dynamic memory
  int puts(const char * str)  // NOLINT
  {
    int i;
    for (i = 0; str[i] != '\0'; i++)
    {
      out(str[i]);
    }
    out('\n');
    return i;
  }

  // =============================
  // Backtrace Utility Functions
  // =============================
  void * stack_trace[config::kBacktraceDepth] = { nullptr };
  size_t stack_depth                          = 0;

  void __cyg_profile_func_enter(void *, void * call_site)  // NOLINT
  {
    stack_trace[stack_depth++] = call_site;
  }

  void __cyg_profile_func_exit(void *, void *)  // NOLINT
  {
    stack_depth--;
  }

  // using FILE = FF_FILE;
  struct FF_FILE {};
  // clang-format off
  int ff_mkdir(...)       { return 0; }  // NOLINT
  int ff_chdir(...)       { return 0; }  // NOLINT
  int ff_rmdir(...)       { return 0; }  // NOLINT
  int ff_getcwd(...)      { return 0; }  // NOLINT
  FILE * ff_fopen(...)    { return nullptr; }  // NOLINT
  int ff_fclose(...)      { return 0; }  // NOLINT
  int ff_fwrite(...)      { return 0; }  // NOLINT
  int ff_fread(...)       { return 0; }  // NOLINT
  int ff_fputc(...)       { return 0; }  // NOLINT
  int ff_fgetc(...)       { return 0; }  // NOLINT
  char * ff_fgets(...)    { return nullptr; }  // NOLINT
  int ff_fprintf(...)     { return 0; }  // NOLINT
  int ff_fseek(...)       { return 0; }  // NOLINT
  int ff_ftell(...)       { return 0; }  // NOLINT
  int ff_seteof(...)      { return 0; }  // NOLINT
  int ff_rewind(...)      { return 0; }  // NOLINT
  int ff_truncate(...)    { return 0; }  // NOLINT
  int ff_errno(...)       { return 0; }  // NOLINT
  int ff_feof(...)        { return 0; }  // NOLINT
  int ff_rename(...)      { return 0; }  // NOLINT
  int ff_remove(...)      { return 0; }  // NOLINT
  int ff_stat(...)        { return 0; }  // NOLINT
  int ff_filelength(...)  { return 0; }  // NOLINT
  int ff_findfirst(...)   { return 0; }  // NOLINT
  int ff_findnext(...)    { return 0; }  // NOLINT
  // clang-format on

  FILE * fopen(const char * filename, const char * mode)  // NOLINT
  {
    return ff_fopen(filename, mode);
  }
  int fclose(FILE * stream)
  {
    return ff_fclose(stream);
  }
  size_t fwrite(const void * buffer, size_t size, size_t count,
                FILE * stream)  // NOLINT
  {
    return ff_fwrite(buffer, size, count, stream);
  }
  size_t fread(void * buffer, size_t size, size_t count,
               FILE * stream)  // NOLINT
  {
    return ff_fread(buffer, size, count, stream);
  }
  int fgetc(FILE * stream)  // NOLINT
  {
    return ff_fgetc(stream);
  }
  char * fgets(char * str, int count, FILE * stream)  // NOLINT
  {
    return ff_fgets(str, count, stream);
  }
  int fputc(int ch, FILE * stream)  // NOLINT
  {
    return ff_fputc(ch, stream);
  }
  int fputs(const char * str, FILE * stream)  // NOLINT
  {
    size_t i;
    for (i = 0; str[i] == '\0'; i++)
    {
      fputc(str[i], stream);
    }
    return i;
  }
  int fseek(FILE * stream, long offset, int origin)  // NOLINT
  {
    return ff_fseek(stream, offset, origin);
  }
  long ftell(FILE * stream)  // NOLINT
  {
    return ff_ftell(stream);
  }
  void rewind(FILE * stream)  // NOLINT
  {
    ff_rewind(stream);
  }
  int feof(FILE * stream)  // NOLINT
  {
    return ff_feof(stream);
  }
  int rename(const char * old_filename, const char * new_filename)  // NOLINT
  {
    return ff_rename(old_filename, new_filename);
  }
  int remove(const char * fname)  // NOLINT
  {
    return ff_remove(fname);
  }
}

void ** GetStackTrace()
{
  return stack_trace;
}

size_t GetStackDepth()
{
  return stack_depth;
}
