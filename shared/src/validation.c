#include "validation.h"

int _check_config(t_config* config, char* keys[]) {
  int i = 0;
  while(keys[i] != NULL){
    if (!config_has_property(config, keys[i++])){
      return 0;
    }
  }
  return 1;
}