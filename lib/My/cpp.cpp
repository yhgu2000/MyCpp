#include "cpp"

#ifdef __unix__

#include <string.h>
#include <unistd.h>

int
getenv_s(std::size_t* len, char* value, std::size_t valuesz, const char* name)
{
  auto nameLen = strlen(name);

  auto env = environ;
  while (*env) {
    if (strncmp(*env, name, nameLen) == 0 && (*env)[nameLen] == '=') {
      strncpy(value, *env + nameLen + 1, valuesz);
      *len = strlen(value);
      return 0;
    }
    ++env;
  }

  *len = 0;
  return -1;
}

#endif
