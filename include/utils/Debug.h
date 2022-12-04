//
// Created by Ylarod on 2022/12/4.
//

#ifndef OBFUSCATOR_DEBUG_H
#define OBFUSCATOR_DEBUG_H

#ifdef DEBUG_MODE

#define OBF_DEBUG(X) do { X; } while(false)
#define IS_DEBUG true

#else

#define OBF_DEBUG(X) do { } while (false)
#define IS_DEBUG false

#endif


#endif //OBFUSCATOR_DEBUG_H
